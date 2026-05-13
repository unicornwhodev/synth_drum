#include <JuceHeader.h>

#include "../Engine/DrumSynthVoice.h"
#include "../Engine/FactoryPresets.h"
#include "../Engine/FxProcessors.h"
#include "../../../Shared/ProductionQa.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
struct RenderJob
{
    juce::String instrument;
    juce::String family;
    juce::String category;
    juce::String subcategory;
    juce::String articulation;
    juce::String key;
    juce::String tempoBpm;
    juce::String durationSeconds;
    juce::String velocityLayer;
    juce::String roundRobin;
    juce::String take;
    juce::String finalRelativePath;
    juce::String presetProfile;
    juce::String fxProfile;
    // Optional Phase 7 columns (empty string = not specified)
    juce::String kitFamily;       // "Classique"|"Acoustique"|"Ambient"|"Cinematique"|"Moderne"
    juce::String kitName;         // specific KitPreset name to seed from
    juce::String reverbPredelay;  // float ms override
    juce::String eqEnable;        // "1"|"true" to enable global EQ
};

struct Event
{
    int sample = 0;
    float velocity = 0.8f;
};

struct VoiceSlot
{
    std::unique_ptr<mds::DrumVoice> voice;
    int startedAt = -1;
};

struct FxSettings
{
    float compThresholdDb  = -18.0f;
    float compRatio        = 2.2f;
    float compAttackMs     = 10.0f;
    float compReleaseMs    = 120.0f;
    float compMix          = 0.10f;
    float satDrive         = 1.18f;
    float satMix           = 0.03f;
    float transientAttack  = 0.16f;
    float transientSustain = -0.02f;
    float transientMix     = 0.12f;
    float stereoWidth      = 0.04f;
    // Reverb (DattorroPlateReverb)
    float reverbSize       = 0.18f;
    float reverbDamping    = 0.46f;
    float reverbWidth      = 0.80f;
    float reverbWet        = 0.0f;
    float reverbPredelay   = 10.0f;
    // EQ (ParametricEQ3Band)
    bool  eqEnable         = false;
    float eqLowFreq        = 120.0f;
    float eqLowGain        = 0.0f;
    float eqMidFreq        = 1000.0f;
    float eqMidGain        = 0.0f;
    float eqMidQ           = 1.0f;
    float eqHighFreq       = 5000.0f;
    float eqHighGain       = 0.0f;
    // Output
    float targetPeak       = 0.93f;
};

constexpr double kSampleRate = 48000.0;

float clamp01(const float value) { return juce::jlimit(0.0f, 1.0f, value); }

juce::String slug(juce::String value)
{
    value = value.toLowerCase();
    value = value.replaceCharacter(' ', '_').replaceCharacter('-', '_').replaceCharacter('/', '_');
    value = value.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_");
    while (value.contains("__"))
        value = value.replace("__", "_");
    return value.trimCharactersAtStart("_").trimCharactersAtEnd("_");
}

bool containsAny(const juce::String& haystack, std::initializer_list<const char*> needles)
{
    for (const auto* needle : needles)
        if (haystack.contains(needle))
            return true;
    return false;
}

juce::StringArray parseCsvLine(const juce::String& line)
{
    juce::StringArray parts;
    juce::String current;
    bool inQuotes = false;
    for (int i = 0; i < line.length(); ++i)
    {
        const auto ch = line[i];
        if (ch == '"')
        {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"')
            {
                current << '"';
                ++i;
            }
            else
            {
                inQuotes = !inQuotes;
            }
        }
        else if (ch == ',' && !inQuotes)
        {
            parts.add(current);
            current.clear();
        }
        else
        {
            current << ch;
        }
    }
    parts.add(current);
    return parts;
}

int csvColumn(const juce::StringArray& header, const juce::String& name)
{
    const auto index = header.indexOf(name);
    if (index < 0)
        throw std::runtime_error(("Missing CSV column: " + name).toStdString());
    return index;
}

int parseTokenIndex(const juce::String& token, const juce::String& prefix)
{
    if (!token.startsWithIgnoreCase(prefix))
        return 1;
    const auto value = token.fromFirstOccurrenceOf(prefix, false, false).getIntValue();
    return value > 0 ? value : 1;
}

float velocityFromLayer(const juce::String& layer)
{
    switch (parseTokenIndex(layer, "v"))
    {
        case 1: return 0.28f;
        case 2: return 0.42f;
        case 3: return 0.58f;
        case 4: return 0.72f;
        case 5: return 0.86f;
        case 6: return 1.00f;
        default: return 0.72f;
    }
}

FxSettings fxProfileFor(const RenderJob& job)
{
    const auto key = slug(job.fxProfile);
    FxSettings fx;
    if (key == "accentplayable")
    {
        fx.compThresholdDb=-16.0f; fx.compRatio=2.9f; fx.compAttackMs=6.0f; fx.compReleaseMs=90.0f;  fx.compMix=0.18f;
        fx.satDrive=1.32f; fx.satMix=0.05f;
        fx.transientAttack=0.25f; fx.transientSustain=-0.03f; fx.transientMix=0.16f;
        fx.stereoWidth=0.05f;
        fx.reverbSize=0.18f; fx.reverbDamping=0.44f; fx.reverbWet=0.01f; fx.reverbPredelay=6.0f;
        fx.eqEnable=true; fx.eqLowFreq=80.0f; fx.eqLowGain=1.5f; fx.eqHighFreq=8000.0f; fx.eqHighGain=0.8f;
    }
    else if (key == "texturedplayable")
    {
        fx.compThresholdDb=-17.5f; fx.compRatio=2.7f; fx.compAttackMs=10.0f; fx.compReleaseMs=120.0f; fx.compMix=0.18f;
        fx.satDrive=1.55f; fx.satMix=0.10f;
        fx.transientAttack=0.14f; fx.transientSustain=0.04f; fx.transientMix=0.12f;
        fx.stereoWidth=0.10f;
        fx.reverbSize=0.28f; fx.reverbDamping=0.50f; fx.reverbWet=0.05f; fx.reverbPredelay=12.0f;
        fx.eqEnable=true; fx.eqMidFreq=800.0f; fx.eqMidGain=1.0f; fx.eqMidQ=0.8f;
    }
    else if (key == "designedtexture")
    {
        fx.compThresholdDb=-19.5f; fx.compRatio=3.0f; fx.compAttackMs=12.0f; fx.compReleaseMs=160.0f; fx.compMix=0.26f;
        fx.satDrive=1.90f; fx.satMix=0.18f;
        fx.transientAttack=0.08f; fx.transientSustain=0.10f; fx.transientMix=0.14f;
        fx.stereoWidth=0.20f;
        fx.reverbSize=0.74f; fx.reverbDamping=0.58f; fx.reverbWet=0.16f; fx.reverbPredelay=20.0f;
        fx.eqEnable=true; fx.eqLowFreq=120.0f; fx.eqLowGain=2.0f; fx.eqHighFreq=6000.0f; fx.eqHighGain=-1.5f;
        fx.targetPeak=0.90f;
    }
    // Apply per-job overrides from optional CSV columns
    if (job.reverbPredelay.isNotEmpty())
        fx.reverbPredelay = juce::jlimit(0.0f, 100.0f, job.reverbPredelay.getFloatValue());
    if (job.eqEnable == "1" || job.eqEnable.equalsIgnoreCase("true"))
        fx.eqEnable = true;
    return fx;
}

FxSettings fxFromGlobalSettings(const mds::GlobalFxSettings& global)
{
    FxSettings fx;
    fx.compThresholdDb = global.compThreshold;
    fx.compRatio = global.compRatio;
    fx.compAttackMs = global.compAttack;
    fx.compReleaseMs = global.compRelease;
    fx.compMix = global.compMix;
    fx.satDrive = global.satDrive;
    fx.satMix = global.satMix;
    fx.transientAttack = global.transientAttack;
    fx.transientSustain = global.transientSustain;
    fx.transientMix = global.transientMix;
    fx.reverbSize = global.reverbSize;
    fx.reverbDamping = global.reverbDamping;
    fx.reverbWidth = global.reverbWidth;
    fx.reverbWet = global.reverbMix;
    fx.reverbPredelay = global.reverbPredelay;
    fx.eqEnable = global.eqEnable;
    fx.eqLowFreq = global.eqLowFreq;
    fx.eqLowGain = global.eqLowGain;
    fx.eqMidFreq = global.eqMidFreq;
    fx.eqMidGain = global.eqMidGain;
    fx.eqMidQ = global.eqMidQ;
    fx.eqHighFreq = global.eqHighFreq;
    fx.eqHighGain = global.eqHighGain;
    fx.targetPeak = juce::Decibels::decibelsToGain(global.limiterEnable ? global.limiterThreshold : -0.6f);
    return fx;
}

bool isTextureJob(const RenderJob& job)
{
    const auto text = slug(job.family + "_" + job.subcategory + "_" + job.articulation + "_" + job.fxProfile);
    return containsAny(text, { "wash", "sweep", "noise", "dark", "glitch", "sizzle", "shimmer", "resonant", "tail", "wide", "splash", "designedtexture", "texturedplayable" });
}

int padIndexFromName(const juce::String& name)
{
    const auto wanted = slug(name);
    if (wanted == "kick_a" || wanted == "kicka") return 0;
    if (wanted == "kick_b" || wanted == "kickb") return 1;
    if (wanted == "snare" || wanted == "snare_a" || wanted == "snarea") return 2;
    if (wanted == "snare_b" || wanted == "snareb") return 3;
    if (wanted == "clap") return 3;
    if (wanted == "hat_closed" || wanted == "hatclosed") return 4;
    if (wanted == "hat_open" || wanted == "hatopen") return 5;
    if (wanted == "perc_1" || wanted == "perc1") return 6;
    if (wanted == "perc_2" || wanted == "perc2") return 7;
    if (wanted == "tom_low" || wanted == "tomlow") return 8;
    if (wanted == "tom_high" || wanted == "tomhigh") return 9;
    if (wanted == "crash") return 10;
    if (wanted == "ride") return 10;
    if (wanted == "fx") return 11;
    return -1;
}

mds::PadSettings seedSettingsForPad(const int padIndex, const RenderJob& job)
{
    // If a specific kit name is given, try to find it in the factory preset list
    if (job.kitName.isNotEmpty())
    {
        const auto wanted = slug(job.kitName);
        for (const auto& kit : mds::getFactoryPresets())
            if (slug(juce::String(kit.name.c_str())) == wanted)
                return kit.pads[static_cast<std::size_t>(padIndex)];
    }
    // If a kit family is given, use applyTargetMatrix to a default-built pad
    if (job.kitFamily.isNotEmpty())
    {
        using KF = mds::KitFamily;
        const auto fam = slug(job.kitFamily);
        KF family = KF::Classique;
        if (fam == "acoustique") family = KF::Acoustique;
        else if (fam == "ambient")     family = KF::Ambient;
        else if (fam == "cinematique") family = KF::Cinematique;
        else if (fam == "moderne")     family = KF::Moderne;

        // Use the first factory kit matching that family as seed
        auto s = mds::getDefaultPadSettings(padIndex);
        // Build a minimal single-pad kit and apply the matrix nudge
        mds::KitPreset tmpKit;
        for (int i = 0; i < mds::kNumPads; ++i)
            tmpKit.pads[static_cast<std::size_t>(i)] = mds::getDefaultPadSettings(i);
        tmpKit.pads[static_cast<std::size_t>(padIndex)] = s;
        mds::applyTargetMatrix(tmpKit, family);
        return tmpKit.pads[static_cast<std::size_t>(padIndex)];
    }
    return mds::getDefaultPadSettings(padIndex);
}

void applyPresetMacro(const RenderJob& job, mds::PadSettings& s)
{
    const auto preset = slug(job.presetProfile);
    if (preset == "expressive")
    {
        s.noiseAmount = clamp01(s.noiseAmount + 0.10f);
        s.clickAmount = clamp01(s.clickAmount + 0.06f);
        s.decaySeconds = juce::jlimit(0.02f, 2.0f, s.decaySeconds * 1.08f);
    }
    else if (preset == "dark")
    {
        s.cutoffHz = juce::jlimit(250.0f, 18000.0f, s.cutoffHz * 0.68f);
        s.decaySeconds = juce::jlimit(0.02f, 2.5f, s.decaySeconds * 1.22f);
        s.drive = juce::jlimit(0.8f, 3.5f, s.drive + 0.10f);
    }
    else if (preset == "cinematic")
    {
        s.decaySeconds = juce::jlimit(0.02f, 3.0f, s.decaySeconds * 1.35f);
        s.noiseAmount = clamp01(s.noiseAmount + 0.06f);
        s.pan = juce::jlimit(-1.0f, 1.0f, s.pan * 1.15f);
    }
}

void adaptSettingsForJob(const RenderJob& job, mds::PadSettings& s)
{
    const auto text = slug(job.subcategory + "_" + job.articulation + "_" + job.family + "_" + job.instrument);

    if (containsAny(text, { "accent", "punch", "impact", "bright" }))
    {
        s.level = juce::jlimit(0.05f, 1.2f, s.level * 1.08f);
        s.clickAmount = clamp01(s.clickAmount + 0.10f);
        s.drive = juce::jlimit(0.8f, 3.5f, s.drive + 0.08f);
    }
    if (containsAny(text, { "soft", "ghost" }))
    {
        s.level = juce::jlimit(0.04f, 1.2f, s.level * 0.78f);
        s.clickAmount = clamp01(s.clickAmount * 0.65f);
        s.noiseAmount = clamp01(s.noiseAmount * 0.82f);
    }
    if (containsAny(text, { "short", "tight", "tick" }))
    {
        s.decaySeconds = juce::jlimit(0.02f, 2.0f, s.decaySeconds * 0.55f);
    }
    if (containsAny(text, { "tail", "open", "wash", "sizzle", "shimmer", "resonant" }))
    {
        s.decaySeconds = juce::jlimit(0.02f, 3.0f, s.decaySeconds * 1.55f);
        s.cutoffHz = juce::jlimit(250.0f, 18000.0f, s.cutoffHz * 1.08f);
    }
    if (containsAny(text, { "low", "sub", "dark" }))
    {
        s.baseFrequencyHz = juce::jmax(28.0f, s.baseFrequencyHz * 0.82f);
        s.cutoffHz = juce::jlimit(180.0f, 16000.0f, s.cutoffHz * 0.72f);
        s.decaySeconds = juce::jlimit(0.02f, 3.0f, s.decaySeconds * 1.18f);
    }
    if (containsAny(text, { "noise", "dirty", "glitch" }))
    {
        s.noiseAmount = clamp01(s.noiseAmount + 0.14f);
        s.drive = juce::jlimit(0.8f, 3.5f, s.drive + 0.12f);
    }
    if (containsAny(text, { "wide", "splash" }))
    {
        s.pan = juce::jlimit(-1.0f, 1.0f, s.pan * 1.30f);
        s.decaySeconds = juce::jlimit(0.02f, 3.0f, s.decaySeconds * 1.20f);
    }
    if (containsAny(text, { "burst", "fill", "roll", "sweep" }))
    {
        s.decaySeconds = juce::jlimit(0.02f, 2.5f, s.decaySeconds * 0.95f);
        s.clickAmount = clamp01(s.clickAmount + 0.04f);
    }
}

void applyVariation(const RenderJob& job, mds::PadSettings& s)
{
    const auto rr = parseTokenIndex(job.roundRobin, "rr");
    const auto take = parseTokenIndex(job.take, "t");
    s.tuneSemitones = juce::jlimit(-24.0f, 24.0f, s.tuneSemitones + 0.10f * static_cast<float>(rr - 2));
    s.pan = juce::jlimit(-1.0f, 1.0f, s.pan + 0.012f * static_cast<float>(take - 6));
    s.decaySeconds = juce::jlimit(0.02f, 3.0f, s.decaySeconds * (1.0f + 0.02f * static_cast<float>((take - 1) % 3 - 1)));
    s.clickAmount = clamp01(s.clickAmount + 0.01f * static_cast<float>((rr + take) % 3 - 1));
    s.noiseAmount = clamp01(s.noiseAmount + 0.015f * static_cast<float>((take + 1) % 3 - 1));
}

void addEvent(std::vector<Event>& events, const int totalSamples, const double timeSeconds, const float velocity)
{
    const auto sample = juce::jlimit(0, juce::jmax(0, totalSamples - 1), static_cast<int>(std::round(timeSeconds * kSampleRate)));
    events.push_back({ sample, juce::jlimit(0.05f, 1.0f, velocity) });
}

std::vector<Event> buildEvents(const RenderJob& job, const float durationSeconds)
{
    const auto totalSamples = static_cast<int>(std::ceil(durationSeconds * kSampleRate));
    const auto baseVelocity = velocityFromLayer(job.velocityLayer);
    const auto text = slug(job.subcategory + "_" + job.articulation + "_" + job.instrument);
    juce::Random random(job.finalRelativePath.hashCode());
    auto jitterSeconds = [&random](const double msRange)
    {
        return (random.nextDouble() - 0.5) * (msRange / 1000.0);
    };

    std::vector<Event> events;

    if (containsAny(text, { "roll" }))
    {
        const int count = text.contains("hat") ? 6 : 5;
        for (int i = 0; i < count; ++i)
            addEvent(events, totalSamples, 0.018 * i + jitterSeconds(4.0), baseVelocity * (0.56f + 0.08f * static_cast<float>(i)));
    }
    else if (containsAny(text, { "burst" }))
    {
        for (int i = 0; i < 4; ++i)
            addEvent(events, totalSamples, 0.012 * i + jitterSeconds(3.0), baseVelocity * (0.72f + 0.05f * static_cast<float>(3 - i)));
    }
    else if (containsAny(text, { "fill" }))
    {
        const double pattern[] = { 0.0, 0.070, 0.140, 0.225 };
        for (int i = 0; i < 4; ++i)
            addEvent(events, totalSamples, pattern[i] + jitterSeconds(6.0), baseVelocity * (0.72f + 0.08f * static_cast<float>(i)));
    }
    else if (containsAny(text, { "wide" }))
    {
        const double pattern[] = { 0.0, 0.011, 0.024, 0.041 };
        for (int i = 0; i < 4; ++i)
            addEvent(events, totalSamples, pattern[i] + jitterSeconds(2.0), baseVelocity * (0.82f - 0.10f * static_cast<float>(i > 1)));
    }
    else if (containsAny(text, { "sweep" }))
    {
        const double pattern[] = { 0.0, 0.110, 0.240, 0.390 };
        for (int i = 0; i < 4; ++i)
            addEvent(events, totalSamples, pattern[i] + jitterSeconds(8.0), baseVelocity * (0.58f + 0.10f * static_cast<float>(i)));
    }
    else if (containsAny(text, { "wash" }))
    {
        addEvent(events, totalSamples, 0.0, baseVelocity);
        addEvent(events, totalSamples, 0.055 + jitterSeconds(6.0), baseVelocity * 0.48f);
    }
    else if (containsAny(text, { "glitch" }))
    {
        const double pattern[] = { 0.0, 0.031, 0.089, 0.157 };
        for (int i = 0; i < 4; ++i)
            addEvent(events, totalSamples, pattern[i] + jitterSeconds(10.0), baseVelocity * (0.55f + 0.10f * static_cast<float>((i + 1) % 3)));
    }
    else if (containsAny(text, { "dirty" }))
    {
        addEvent(events, totalSamples, 0.0, baseVelocity);
        addEvent(events, totalSamples, 0.014 + jitterSeconds(3.0), baseVelocity * 0.42f);
    }
    else if (containsAny(text, { "ghost" }))
    {
        addEvent(events, totalSamples, 0.0, baseVelocity * 0.62f);
        addEvent(events, totalSamples, 0.048 + jitterSeconds(4.0), baseVelocity * 0.34f);
    }
    else if (containsAny(text, { "noise" }))
    {
        addEvent(events, totalSamples, 0.0, baseVelocity * 0.82f);
        addEvent(events, totalSamples, 0.072 + jitterSeconds(5.0), baseVelocity * 0.28f);
    }
    else if (containsAny(text, { "splash" }))
    {
        addEvent(events, totalSamples, 0.0, baseVelocity);
        addEvent(events, totalSamples, 0.043 + jitterSeconds(4.0), baseVelocity * 0.36f);
    }
    else
    {
        addEvent(events, totalSamples, 0.0, baseVelocity);
    }

    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.sample < b.sample; });
    return events;
}

void renderTimeline(juce::AudioBuffer<float>& buffer, const std::vector<Event>& events, const mds::PadSettings& settings)
{
    std::vector<VoiceSlot> voices(24);
    std::size_t eventIndex = 0;
    int nextSteal = 0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        while (eventIndex < events.size() && events[eventIndex].sample == sample)
        {
            int slotIndex = -1;
            for (int i = 0; i < static_cast<int>(voices.size()); ++i)
            {
                const auto& slotVoice = voices[static_cast<std::size_t>(i)].voice;
                if (slotVoice == nullptr || !slotVoice->isActive())
                {
                    slotIndex = i;
                    break;
                }
            }
            if (slotIndex < 0)
            {
                slotIndex = nextSteal;
                nextSteal = (nextSteal + 1) % static_cast<int>(voices.size());
            }

            auto& slot = voices[static_cast<std::size_t>(slotIndex)];
            slot.startedAt = sample;
            if (slot.voice == nullptr)
                slot.voice = mds::createVoiceForModel(settings.voiceModel);

            if (slot.voice != nullptr)
                slot.voice->start(settings, events[eventIndex].velocity, kSampleRate);
            ++eventIndex;
        }

        for (auto& slot : voices)
            if (slot.voice != nullptr && slot.voice->isActive())
                slot.voice->render(buffer, sample, 1);
    }
}

void applyTransient(juce::AudioBuffer<float>& buffer, const FxSettings& fx)
{
    if (fx.transientMix <= 0.0001f)
        return;

    std::array<float, 2> fastEnv = { 0.0f, 0.0f };
    std::array<float, 2> slowEnv = { 0.0f, 0.0f };
    const auto fastCoeff = std::exp(-1.0f / (0.0015f * static_cast<float>(kSampleRate)));
    const auto slowCoeff = std::exp(-1.0f / (0.040f * static_cast<float>(kSampleRate)));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& fast = fastEnv[static_cast<std::size_t>(juce::jlimit(0, 1, ch))];
        auto& slow = slowEnv[static_cast<std::size_t>(juce::jlimit(0, 1, ch))];
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto dry = data[i];
            const auto absSample = std::abs(dry);
            fast = fastCoeff * fast + (1.0f - fastCoeff) * absSample;
            slow = slowCoeff * slow + (1.0f - slowCoeff) * absSample;
            const auto transient = fast - slow;
            const auto gain = juce::jlimit(0.25f, 4.0f, 1.0f + fx.transientAttack * juce::jmax(0.0f, transient) * 8.0f + fx.transientSustain * juce::jmax(0.0f, -transient) * 5.0f);
            data[i] = dry + (dry * gain - dry) * fx.transientMix;
        }
    }
}

void applySaturator(juce::AudioBuffer<float>& buffer, const FxSettings& fx)
{
    if (fx.satMix <= 0.0001f)
        return;
    const auto norm = 1.0f / std::max(0.0001f, std::tanh(fx.satDrive));
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto dry = data[i];
            const auto wet = std::tanh(dry * fx.satDrive) * norm;
            data[i] = dry + (wet - dry) * fx.satMix;
        }
    }
}

void applyCompressor(juce::AudioBuffer<float>& buffer, const FxSettings& fx)
{
    if (fx.compMix <= 0.0001f)
        return;
    juce::AudioBuffer<float> dry;
    dry.makeCopyOf(buffer);
    juce::dsp::Compressor<float> compressor;
    juce::dsp::ProcessSpec spec { kSampleRate, static_cast<juce::uint32>(buffer.getNumSamples()), static_cast<juce::uint32>(buffer.getNumChannels()) };
    compressor.prepare(spec);
    compressor.setThreshold(fx.compThresholdDb);
    compressor.setRatio(fx.compRatio);
    compressor.setAttack(fx.compAttackMs);
    compressor.setRelease(fx.compReleaseMs);
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        auto* dryData = dry.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            wet[i] = dryData[i] + (wet[i] - dryData[i]) * fx.compMix;
    }
}

void applyStereoWidth(juce::AudioBuffer<float>& buffer, const FxSettings& fx)
{
    if (buffer.getNumChannels() < 2 || fx.stereoWidth <= 0.0001f)
        return;
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    const auto width = 1.0f + fx.stereoWidth;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto mid = 0.5f * (left[i] + right[i]);
        const auto side = 0.5f * (left[i] - right[i]) * width;
        left[i] = mid + side;
        right[i] = mid - side;
    }
}

void applyReverb(juce::AudioBuffer<float>& buffer, const FxSettings& fx)
{
    if (fx.reverbWet <= 0.0001f)
        return;
    mds::fx::DattorroPlateReverb reverb;
    reverb.prepare(kSampleRate, buffer.getNumSamples());
    mds::fx::DattorroPlateReverb::Params rp;
    rp.decay      = clamp01(fx.reverbSize);
    rp.damping    = clamp01(fx.reverbDamping);
    rp.width      = clamp01(fx.reverbWidth);
    rp.mix        = clamp01(fx.reverbWet);
    rp.preDelayMs = fx.reverbPredelay;
    const auto numSamples = buffer.getNumSamples();
    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getNumChannels() >= 2 ? buffer.getWritePointer(1) : nullptr;
    reverb.process(left, right, numSamples, rp);
}

void applyEQ(juce::AudioBuffer<float>& buffer, const FxSettings& fx)
{
    if (!fx.eqEnable)
        return;
    mds::fx::ParametricEQ3Band eq;
    eq.prepare(kSampleRate);
    mds::fx::ParametricEQ3Band::Params ep;
    ep.lowFreq    = fx.eqLowFreq;
    ep.lowGainDb  = fx.eqLowGain;
    ep.midFreq    = fx.eqMidFreq;
    ep.midGainDb  = fx.eqMidGain;
    ep.midQ       = fx.eqMidQ;
    ep.highFreq   = fx.eqHighFreq;
    ep.highGainDb = fx.eqHighGain;
    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getNumChannels() >= 2 ? buffer.getWritePointer(1) : nullptr;
    eq.process(left, right, buffer.getNumSamples(), ep);
}

void applyChorus(juce::AudioBuffer<float>& buffer, const mds::GlobalFxSettings& fx)
{
    if (!fx.chorusEnable || fx.chorusMix <= 0.0001f)
        return;

    mds::fx::StereoChorus chorus;
    chorus.prepare(kSampleRate, buffer.getNumSamples());
    mds::fx::StereoChorus::Params cp;
    cp.rateHz = fx.chorusRate;
    cp.depth = fx.chorusDepth;
    cp.mix = fx.chorusMix;

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() >= 2 ? buffer.getWritePointer(1) : nullptr;
    chorus.process(left, right, buffer.getNumSamples(), cp);
}

void applyDelay(juce::AudioBuffer<float>& buffer, const mds::GlobalFxSettings& fx)
{
    if (!fx.delayEnable || fx.delayMix <= 0.0001f)
        return;

    mds::fx::StereoDelay delay;
    delay.prepare(kSampleRate, buffer.getNumSamples());
    mds::fx::StereoDelay::Params dp;
    dp.timeMs = fx.delayTime;
    dp.feedback = fx.delayFeedback;
    dp.mix = fx.delayMix;
    dp.syncToBpm = fx.delaySync;
    dp.noteDiv = fx.delayNoteDiv;
    dp.bpm = 120.0f;

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() >= 2 ? buffer.getWritePointer(1) : nullptr;
    delay.process(left, right, buffer.getNumSamples(), dp);
}

void applyLimiter(juce::AudioBuffer<float>& buffer, const mds::GlobalFxSettings& fx)
{
    if (!fx.limiterEnable)
        return;

    mds::fx::OutputLimiter limiter;
    limiter.prepare(kSampleRate);
    mds::fx::OutputLimiter::Params lp;
    lp.thresholdDb = fx.limiterThreshold;
    lp.releaseMs = fx.limiterRelease;

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() >= 2 ? buffer.getWritePointer(1) : nullptr;
    limiter.process(left, right, buffer.getNumSamples(), lp);
}

void applyDcHighPass(juce::AudioBuffer<float>& buffer)
{
    const auto alpha = std::exp(-2.0f * juce::MathConstants<float>::pi * 16.0f / static_cast<float>(kSampleRate));
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        float previousInput = 0.0f;
        float previousOutput = 0.0f;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto input = data[i];
            const auto output = alpha * (previousOutput + input - previousInput);
            data[i] = output;
            previousInput = input;
            previousOutput = output;
        }
    }
}

void applyAuxBusSafety(juce::AudioBuffer<float>& buffer)
{
    constexpr float kAuxTrimDb = -2.5f;
    const float trim = juce::Decibels::decibelsToGain(kAuxTrimDb);
    const float normalizer = 1.0f / std::max(0.0001f, std::tanh(1.15f));

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float trimmed = data[i] * trim;
            const float protectedSample = std::tanh(trimmed * 1.15f) * normalizer * 0.90f;
            data[i] = std::abs(trimmed) > 0.80f ? protectedSample : trimmed;
        }
    }
}

void applyMasterFxChain(juce::AudioBuffer<float>& buffer, const mds::GlobalFxSettings& fx)
{
    const auto renderFx = fxFromGlobalSettings(fx);
    applyTransient(buffer, renderFx);
    applySaturator(buffer, renderFx);
    applyCompressor(buffer, renderFx);
    applyEQ(buffer, renderFx);
    applyChorus(buffer, fx);
    applyDelay(buffer, fx);
    applyReverb(buffer, renderFx);
    applyStereoWidth(buffer, renderFx);
    buffer.applyGain(juce::Decibels::decibelsToGain(fx.outputGainDb));
    applyLimiter(buffer, fx);
    applyDcHighPass(buffer);
}

void trimAndProtect(juce::AudioBuffer<float>& buffer, const RenderJob& job, const FxSettings& fx)
{
    int first = 0;
    int last = buffer.getNumSamples() - 1;
    auto magnitudeAt = [&buffer](const int sample)
    {
        float mag = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            mag = juce::jmax(mag, std::abs(buffer.getSample(ch, sample)));
        return mag;
    };

    while (first < buffer.getNumSamples() && magnitudeAt(first) < 0.0007f) ++first;
    while (last > first && magnitudeAt(last) < 0.0007f) --last;
    first = juce::jmax(0, first - 48);
    last = juce::jmin(buffer.getNumSamples() - 1, last + (isTextureJob(job) ? 1800 : 900));

    const auto newLength = juce::jmax(1, last - first + 1);
    juce::AudioBuffer<float> trimmed(buffer.getNumChannels(), newLength);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        trimmed.copyFrom(ch, 0, buffer, ch, first, newLength);
    buffer.makeCopyOf(trimmed);

    const auto fadeIn = juce::jmin(48, buffer.getNumSamples() / 6);
    const auto fadeOut = juce::jmin(isTextureJob(job) ? 640 : 160, buffer.getNumSamples() / 3);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        if (fadeIn > 0)
            buffer.applyGainRamp(ch, 0, fadeIn, 0.0f, 1.0f);
        if (fadeOut > 0)
            buffer.applyGainRamp(ch, buffer.getNumSamples() - fadeOut, fadeOut, 1.0f, 0.0f);
    }

    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    if (peak > 0.0001f)
        buffer.applyGain(fx.targetPeak / peak);
}

bool writeWav(const juce::File& file, juce::AudioBuffer<float>& buffer)
{
    file.getParentDirectory().createDirectory();
    juce::WavAudioFormat format;
    std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());
    if (stream == nullptr)
        return false;
    if (auto* writer = format.createWriterFor(stream.get(), kSampleRate, static_cast<unsigned int>(buffer.getNumChannels()), 24, {}, 0))
    {
        stream.release();
        std::unique_ptr<juce::AudioFormatWriter> holder(writer);
        return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }
    return false;
}

std::vector<RenderJob> readManifestCsv(const juce::File& file)
{
    juce::StringArray lines;
    lines.addLines(file.loadFileAsString());
    if (lines.size() < 2)
        throw std::runtime_error("Manifest CSV is empty.");

    const auto header = parseCsvLine(lines[0]);
    const auto idxInstrument = csvColumn(header, "Instrument");
    const auto idxFamily = csvColumn(header, "Family");
    const auto idxCategory = csvColumn(header, "Category");
    const auto idxSubcategory = csvColumn(header, "Subcategory");
    const auto idxArticulation = csvColumn(header, "Articulation");
    const auto idxKey = csvColumn(header, "Key");
    const auto idxTempo = csvColumn(header, "Tempo BPM");
    const auto idxDuration = csvColumn(header, "Duration (s)");
    const auto idxVelocity = csvColumn(header, "Velocity Layer");
    const auto idxRr = csvColumn(header, "Round Robin");
    const auto idxTake = csvColumn(header, "Take");
    const auto idxPath = csvColumn(header, "Final Relative Path");
    const auto idxPreset = csvColumn(header, "Preset Profile");
    const auto idxFx = csvColumn(header, "FX Profile");
    // Optional Phase 7 columns — gracefully absent in older CSVs
    const auto idxKitFamily    = header.indexOf("Kit Family");
    const auto idxKitName      = header.indexOf("Kit Name");
    const auto idxReverbPre    = header.indexOf("Reverb Predelay");
    const auto idxEqEn         = header.indexOf("EQ Enable");

    auto optCol = [&](const juce::StringArray& cols, int idx) -> juce::String
    {
        if (idx < 0 || idx >= static_cast<int>(cols.size())) return {};
        return cols[static_cast<std::size_t>(idx)];
    };

    std::vector<RenderJob> jobs;
    for (int lineIndex = 1; lineIndex < lines.size(); ++lineIndex)
    {
        const auto cols = parseCsvLine(lines[lineIndex]);
        if (static_cast<int>(cols.size()) < static_cast<int>(header.size()))
            continue;
        RenderJob j;
        j.instrument      = cols[static_cast<std::size_t>(idxInstrument)];
        j.family          = cols[static_cast<std::size_t>(idxFamily)];
        j.category        = cols[static_cast<std::size_t>(idxCategory)];
        j.subcategory     = cols[static_cast<std::size_t>(idxSubcategory)];
        j.articulation    = cols[static_cast<std::size_t>(idxArticulation)];
        j.key             = cols[static_cast<std::size_t>(idxKey)];
        j.tempoBpm        = cols[static_cast<std::size_t>(idxTempo)];
        j.durationSeconds = cols[static_cast<std::size_t>(idxDuration)];
        j.velocityLayer   = cols[static_cast<std::size_t>(idxVelocity)];
        j.roundRobin      = cols[static_cast<std::size_t>(idxRr)];
        j.take            = cols[static_cast<std::size_t>(idxTake)];
        j.finalRelativePath = cols[static_cast<std::size_t>(idxPath)];
        j.presetProfile   = cols[static_cast<std::size_t>(idxPreset)];
        j.fxProfile       = cols[static_cast<std::size_t>(idxFx)];
        j.kitFamily       = optCol(cols, idxKitFamily);
        j.kitName         = optCol(cols, idxKitName);
        j.reverbPredelay  = optCol(cols, idxReverbPre);
        j.eqEnable        = optCol(cols, idxEqEn);
        jobs.push_back(std::move(j));
    }
    return jobs;
}

struct AudioMetrics
{
    float peakDb = -100.0f;
    float rmsDb = -100.0f;
    float hfRatio = 0.0f;
    float tailMs = 0.0f;
    float stereoWidth = 0.0f;
};

float toDb(const float linear)
{
    return linear > 0.0000001f ? 20.0f * std::log10(linear) : -100.0f;
}

AudioMetrics measureBuffer(const juce::AudioBuffer<float>& buffer)
{
    AudioMetrics m;
    if (buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0)
        return m;

    float peak = 0.0f;
    double sumSquares = 0.0;
    double diffSquares = 0.0;
    double totalSquares = 0.0;
    int tailIndex = 0;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float v = buffer.getSample(ch, sample);
            peak = std::max(peak, std::abs(v));
            mono += v;
            sumSquares += static_cast<double>(v) * static_cast<double>(v);
        }

        mono /= static_cast<float>(buffer.getNumChannels());
        totalSquares += static_cast<double>(mono) * static_cast<double>(mono);
        if (sample > 0)
        {
            const float diff = mono - (buffer.getNumChannels() > 0
                ? 0.5f * (buffer.getSample(0, sample - 1)
                        + buffer.getSample(juce::jmin(1, buffer.getNumChannels() - 1), sample - 1))
                : 0.0f);
            diffSquares += static_cast<double>(diff) * static_cast<double>(diff);
        }
        if (std::abs(mono) > 0.0012f)
            tailIndex = sample;
    }

    m.peakDb = toDb(peak);
    const auto sampleCount = static_cast<double>(buffer.getNumChannels() * buffer.getNumSamples());
    m.rmsDb = toDb(static_cast<float>(std::sqrt(sumSquares / std::max(1.0, sampleCount))));
    m.hfRatio = clamp01(static_cast<float>(std::sqrt(diffSquares / std::max(1.0, totalSquares + 1.0e-9))));
    m.tailMs = static_cast<float>(1000.0 * static_cast<double>(tailIndex) / kSampleRate);

    if (buffer.getNumChannels() >= 2)
    {
        double side = 0.0;
        double mid = 0.0;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float l = buffer.getSample(0, i);
            const float r = buffer.getSample(1, i);
            const float mSample = 0.5f * (l + r);
            const float sSample = 0.5f * (l - r);
            mid += static_cast<double>(mSample) * static_cast<double>(mSample);
            side += static_cast<double>(sSample) * static_cast<double>(sSample);
        }
        m.stereoWidth = clamp01(static_cast<float>(std::sqrt(side / std::max(1.0, mid + side))));
    }

    return m;
}

float measureWindowRmsDb(const juce::AudioBuffer<float>& buffer, const int startSample)
{
    if (buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= startSample)
        return -100.0f;

    double sumSquares = 0.0;
    int count = 0;
    for (int sample = juce::jmax(0, startSample); sample < buffer.getNumSamples(); ++sample)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float v = buffer.getSample(ch, sample);
            sumSquares += static_cast<double>(v) * static_cast<double>(v);
            ++count;
        }
    }

    return toDb(static_cast<float>(std::sqrt(sumSquares / std::max(1, count))));
}

juce::AudioBuffer<float> renderSingleHit(const mds::PadSettings& settings,
                                         const float velocity,
                                         const double durationSeconds)
{
    const int renderSamples = std::max(1, static_cast<int>(kSampleRate * durationSeconds));
    juce::AudioBuffer<float> buffer(2, renderSamples);
    buffer.clear();
    auto voice = mds::createVoiceForModel(settings.voiceModel);
    if (voice != nullptr)
    {
        voice->start(settings, velocity, kSampleRate);
        voice->render(buffer, 0, renderSamples);
    }
    return buffer;
}

juce::AudioBuffer<float> renderGroupHits(const mds::KitPreset& kit,
                                         std::initializer_list<int> pads,
                                         const float velocity,
                                         const double durationSeconds)
{
    const int renderSamples = std::max(1, static_cast<int>(kSampleRate * durationSeconds));
    juce::AudioBuffer<float> buffer(2, renderSamples);
    buffer.clear();

    for (int pad : pads)
    {
        auto hit = renderSingleHit(kit.pads[static_cast<std::size_t>(pad)], velocity, durationSeconds);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addFrom(ch, 0, hit, ch, 0, renderSamples);
    }

    return buffer;
}

struct ScheduledPadEvent
{
    int sample = 0;
    int padIndex = 0;
    float velocity = 0.8f;
};

juce::AudioBuffer<float> renderScheduledPads(const mds::KitPreset& kit,
                                             std::vector<ScheduledPadEvent> events,
                                             const double durationSeconds)
{
    struct ActiveSlot
    {
        std::unique_ptr<mds::DrumVoice> voice;
        int chokeGroup = 0;
        int startedAt = -1;
    };

    const int renderSamples = std::max(1, static_cast<int>(kSampleRate * durationSeconds));
    juce::AudioBuffer<float> buffer(2, renderSamples);
    buffer.clear();

    std::sort(events.begin(), events.end(), [](const ScheduledPadEvent& a, const ScheduledPadEvent& b)
    {
        return a.sample < b.sample;
    });

    std::vector<ActiveSlot> voices(24);
    std::size_t eventIndex = 0;
    int nextSteal = 0;

    for (int sample = 0; sample < renderSamples; ++sample)
    {
        while (eventIndex < events.size() && events[eventIndex].sample == sample)
        {
            const auto& event = events[eventIndex];
            if (event.padIndex >= 0 && event.padIndex < mds::kNumPads)
            {
                const int chokeGroup = mds::getPadInfo(event.padIndex).chokeGroup;
                if (chokeGroup > 0)
                {
                    for (auto& slot : voices)
                    {
                        if (slot.voice != nullptr && slot.chokeGroup == chokeGroup)
                        {
                            slot.voice.reset();
                            slot.chokeGroup = 0;
                            slot.startedAt = -1;
                        }
                    }
                }

                int slotIndex = -1;
                for (int i = 0; i < static_cast<int>(voices.size()); ++i)
                {
                    if (voices[static_cast<std::size_t>(i)].voice == nullptr
                        || !voices[static_cast<std::size_t>(i)].voice->isActive())
                    {
                        slotIndex = i;
                        break;
                    }
                }
                if (slotIndex < 0)
                {
                    slotIndex = nextSteal;
                    nextSteal = (nextSteal + 1) % static_cast<int>(voices.size());
                }

                auto& slot = voices[static_cast<std::size_t>(slotIndex)];
                const auto& settings = kit.pads[static_cast<std::size_t>(event.padIndex)];
                slot.voice = mds::createVoiceForModel(settings.voiceModel);
                slot.chokeGroup = chokeGroup;
                slot.startedAt = sample;
                if (slot.voice != nullptr)
                    slot.voice->start(settings, event.velocity, kSampleRate);
            }

            ++eventIndex;
        }

        for (auto& slot : voices)
        {
            if (slot.voice != nullptr && slot.voice->isActive())
                slot.voice->render(buffer, sample, 1);
            else if (slot.voice != nullptr && !slot.voice->isActive())
                slot.voice.reset();
        }
    }

    return buffer;
}

std::unique_ptr<juce::XmlElement> writeKitPresetXml(const mds::KitPreset& kit)
{
    auto root = std::make_unique<juce::XmlElement>("KitPreset");
    root->setAttribute("name", kit.name);
    root->setAttribute("preset_version", mds::kPresetVersion);
    root->setAttribute("family_label", kit.familyLabel);
    root->setAttribute("mix_role", kit.mixRole);
    root->setAttribute("description", kit.description);
    root->setAttribute("output_profile", kit.outputProfile);
    root->setAttribute("nominal_peak_db", kit.nominalPeakDb);

    juce::StringArray tagValues;
    for (const auto& tag : kit.tags)
        tagValues.add(juce::String(tag));
    root->setAttribute("tags", tagValues.joinIntoString(";"));

    auto* fx = root->createNewChildElement("GlobalFx");
    fx->setAttribute("output_gain", kit.fx.outputGainDb);
    fx->setAttribute("macro_punch", kit.fx.macroPunch);
    fx->setAttribute("macro_weight", kit.fx.macroWeight);
    fx->setAttribute("macro_air", kit.fx.macroAir);
    fx->setAttribute("macro_dirt", kit.fx.macroDirt);
    fx->setAttribute("comp_threshold", kit.fx.compThreshold);
    fx->setAttribute("comp_ratio", kit.fx.compRatio);
    fx->setAttribute("comp_attack", kit.fx.compAttack);
    fx->setAttribute("comp_release", kit.fx.compRelease);
    fx->setAttribute("comp_makeup", kit.fx.compMakeup);
    fx->setAttribute("comp_mix", kit.fx.compMix);
    fx->setAttribute("sat_drive", kit.fx.satDrive);
    fx->setAttribute("sat_mix", kit.fx.satMix);
    fx->setAttribute("transient_attack", kit.fx.transientAttack);
    fx->setAttribute("transient_sustain", kit.fx.transientSustain);
    fx->setAttribute("transient_mix", kit.fx.transientMix);
    fx->setAttribute("reverb_size", kit.fx.reverbSize);
    fx->setAttribute("reverb_damping", kit.fx.reverbDamping);
    fx->setAttribute("reverb_width", kit.fx.reverbWidth);
    fx->setAttribute("reverb_mix", kit.fx.reverbMix);
    fx->setAttribute("reverb_predelay", kit.fx.reverbPredelay);
    fx->setAttribute("eq_low_freq", kit.fx.eqLowFreq);
    fx->setAttribute("eq_low_gain", kit.fx.eqLowGain);
    fx->setAttribute("eq_mid_freq", kit.fx.eqMidFreq);
    fx->setAttribute("eq_mid_gain", kit.fx.eqMidGain);
    fx->setAttribute("eq_mid_q", kit.fx.eqMidQ);
    fx->setAttribute("eq_high_freq", kit.fx.eqHighFreq);
    fx->setAttribute("eq_high_gain", kit.fx.eqHighGain);
    fx->setAttribute("fx_eq_en", kit.fx.eqEnable ? 1 : 0);
    fx->setAttribute("chorus_rate", kit.fx.chorusRate);
    fx->setAttribute("chorus_depth", kit.fx.chorusDepth);
    fx->setAttribute("chorus_mix", kit.fx.chorusMix);
    fx->setAttribute("fx_chorus_en", kit.fx.chorusEnable ? 1 : 0);
    fx->setAttribute("delay_time", kit.fx.delayTime);
    fx->setAttribute("delay_feedback", kit.fx.delayFeedback);
    fx->setAttribute("delay_mix", kit.fx.delayMix);
    fx->setAttribute("delay_sync", kit.fx.delaySync ? 1 : 0);
    fx->setAttribute("delay_note_div", kit.fx.delayNoteDiv);
    fx->setAttribute("fx_delay_en", kit.fx.delayEnable ? 1 : 0);
    fx->setAttribute("limiter_threshold", kit.fx.limiterThreshold);
    fx->setAttribute("limiter_release", kit.fx.limiterRelease);
    fx->setAttribute("fx_limiter_en", kit.fx.limiterEnable ? 1 : 0);

    for (int pad = 0; pad < mds::kNumPads; ++pad)
    {
        const auto& s = kit.pads[static_cast<std::size_t>(pad)];
        auto* padXml = root->createNewChildElement("Pad");
        padXml->setAttribute("index", pad);
        padXml->setAttribute("level", s.level);
        padXml->setAttribute("tune", s.tuneSemitones);
        padXml->setAttribute("decay", s.decaySeconds);
        padXml->setAttribute("attack", s.attackSeconds);
        padXml->setAttribute("pitch_drop", s.pitchDropSemitones);
        padXml->setAttribute("pitch_decay", s.pitchDecaySeconds);
        padXml->setAttribute("noise", s.noiseAmount);
        padXml->setAttribute("click", s.clickAmount);
        padXml->setAttribute("drive", s.drive);
        padXml->setAttribute("cutoff", s.cutoffHz);
        padXml->setAttribute("pan", s.pan);
        padXml->setAttribute("base_freq", s.baseFrequencyHz);
        padXml->setAttribute("voice_model", static_cast<int>(s.voiceModel));
        padXml->setAttribute("output", kit.outputBuses[static_cast<std::size_t>(pad)]);
    }

    return root;
}

mds::KitPreset readKitPresetXml(const juce::XmlElement& xml, const mds::KitPreset& fallback)
{
    auto kit = fallback;
    kit.name = xml.getStringAttribute("name", juce::String(kit.name)).toStdString();
    kit.familyLabel = xml.getStringAttribute("family_label", juce::String(kit.familyLabel)).toStdString();
    kit.mixRole = xml.getStringAttribute("mix_role", juce::String(kit.mixRole)).toStdString();
    kit.description = xml.getStringAttribute("description", juce::String(kit.description)).toStdString();
    kit.outputProfile = xml.getStringAttribute("output_profile", juce::String(kit.outputProfile)).toStdString();
    kit.nominalPeakDb = static_cast<float>(xml.getDoubleAttribute("nominal_peak_db", kit.nominalPeakDb));
    {
        kit.tags.clear();
        const auto tagText = xml.getStringAttribute("tags");
        const auto parsedTags = juce::StringArray::fromTokens(tagText, ";,", "\"");
        for (const auto& tag : parsedTags)
        {
            const auto trimmed = tag.trim();
            if (trimmed.isNotEmpty())
                kit.tags.push_back(trimmed.toStdString());
        }
    }

    if (const auto* fx = xml.getChildByName("GlobalFx"))
    {
        auto readFloat = [fx](const char* attr, float fallbackValue)
        {
            return static_cast<float>(fx->getDoubleAttribute(attr, fallbackValue));
        };

        kit.fx.outputGainDb = readFloat("output_gain", kit.fx.outputGainDb);
        kit.fx.macroPunch = readFloat("macro_punch", kit.fx.macroPunch);
        kit.fx.macroWeight = readFloat("macro_weight", kit.fx.macroWeight);
        kit.fx.macroAir = readFloat("macro_air", kit.fx.macroAir);
        kit.fx.macroDirt = readFloat("macro_dirt", kit.fx.macroDirt);
        kit.fx.compThreshold = readFloat("comp_threshold", kit.fx.compThreshold);
        kit.fx.compRatio = readFloat("comp_ratio", kit.fx.compRatio);
        kit.fx.compAttack = readFloat("comp_attack", kit.fx.compAttack);
        kit.fx.compRelease = readFloat("comp_release", kit.fx.compRelease);
        kit.fx.compMakeup = readFloat("comp_makeup", kit.fx.compMakeup);
        kit.fx.compMix = readFloat("comp_mix", kit.fx.compMix);
        kit.fx.satDrive = readFloat("sat_drive", kit.fx.satDrive);
        kit.fx.satMix = readFloat("sat_mix", kit.fx.satMix);
        kit.fx.transientAttack = readFloat("transient_attack", kit.fx.transientAttack);
        kit.fx.transientSustain = readFloat("transient_sustain", kit.fx.transientSustain);
        kit.fx.transientMix = readFloat("transient_mix", kit.fx.transientMix);
        kit.fx.reverbSize = readFloat("reverb_size", kit.fx.reverbSize);
        kit.fx.reverbDamping = readFloat("reverb_damping", kit.fx.reverbDamping);
        kit.fx.reverbWidth = readFloat("reverb_width", kit.fx.reverbWidth);
        kit.fx.reverbMix = readFloat("reverb_mix", kit.fx.reverbMix);
        kit.fx.reverbPredelay = readFloat("reverb_predelay", kit.fx.reverbPredelay);
        kit.fx.eqLowFreq = readFloat("eq_low_freq", kit.fx.eqLowFreq);
        kit.fx.eqLowGain = readFloat("eq_low_gain", kit.fx.eqLowGain);
        kit.fx.eqMidFreq = readFloat("eq_mid_freq", kit.fx.eqMidFreq);
        kit.fx.eqMidGain = readFloat("eq_mid_gain", kit.fx.eqMidGain);
        kit.fx.eqMidQ = readFloat("eq_mid_q", kit.fx.eqMidQ);
        kit.fx.eqHighFreq = readFloat("eq_high_freq", kit.fx.eqHighFreq);
        kit.fx.eqHighGain = readFloat("eq_high_gain", kit.fx.eqHighGain);
        kit.fx.eqEnable = fx->getIntAttribute("fx_eq_en", kit.fx.eqEnable ? 1 : 0) != 0;
        kit.fx.chorusRate = readFloat("chorus_rate", kit.fx.chorusRate);
        kit.fx.chorusDepth = readFloat("chorus_depth", kit.fx.chorusDepth);
        kit.fx.chorusMix = readFloat("chorus_mix", kit.fx.chorusMix);
        kit.fx.chorusEnable = fx->getIntAttribute("fx_chorus_en", kit.fx.chorusEnable ? 1 : 0) != 0;
        kit.fx.delayTime = readFloat("delay_time", kit.fx.delayTime);
        kit.fx.delayFeedback = readFloat("delay_feedback", kit.fx.delayFeedback);
        kit.fx.delayMix = readFloat("delay_mix", kit.fx.delayMix);
        kit.fx.delaySync = fx->getIntAttribute("delay_sync", kit.fx.delaySync ? 1 : 0) != 0;
        kit.fx.delayNoteDiv = fx->getIntAttribute("delay_note_div", kit.fx.delayNoteDiv);
        kit.fx.delayEnable = fx->getIntAttribute("fx_delay_en", kit.fx.delayEnable ? 1 : 0) != 0;
        kit.fx.limiterThreshold = readFloat("limiter_threshold", kit.fx.limiterThreshold);
        kit.fx.limiterRelease = readFloat("limiter_release", kit.fx.limiterRelease);
        kit.fx.limiterEnable = fx->getIntAttribute("fx_limiter_en", kit.fx.limiterEnable ? 1 : 0) != 0;
    }

    for (auto* padXml = xml.getChildByName("Pad"); padXml != nullptr; padXml = padXml->getNextElementWithTagName("Pad"))
    {
        const int padIndex = padXml->getIntAttribute("index", -1);
        if (padIndex < 0 || padIndex >= mds::kNumPads)
            continue;

        auto& pad = kit.pads[static_cast<std::size_t>(padIndex)];
        pad.level = static_cast<float>(padXml->getDoubleAttribute("level", pad.level));
        pad.tuneSemitones = static_cast<float>(padXml->getDoubleAttribute("tune", pad.tuneSemitones));
        pad.decaySeconds = static_cast<float>(padXml->getDoubleAttribute("decay", pad.decaySeconds));
        pad.attackSeconds = static_cast<float>(padXml->getDoubleAttribute("attack", pad.attackSeconds));
        pad.pitchDropSemitones = static_cast<float>(padXml->getDoubleAttribute("pitch_drop", pad.pitchDropSemitones));
        pad.pitchDecaySeconds = static_cast<float>(padXml->getDoubleAttribute("pitch_decay", pad.pitchDecaySeconds));
        pad.noiseAmount = static_cast<float>(padXml->getDoubleAttribute("noise", pad.noiseAmount));
        pad.clickAmount = static_cast<float>(padXml->getDoubleAttribute("click", pad.clickAmount));
        pad.drive = static_cast<float>(padXml->getDoubleAttribute("drive", pad.drive));
        pad.cutoffHz = static_cast<float>(padXml->getDoubleAttribute("cutoff", pad.cutoffHz));
        pad.pan = static_cast<float>(padXml->getDoubleAttribute("pan", pad.pan));
        pad.baseFrequencyHz = static_cast<float>(padXml->getDoubleAttribute("base_freq", pad.baseFrequencyHz));
        pad.voiceModel = static_cast<mds::PadVoiceModel>(padXml->getIntAttribute("voice_model", static_cast<int>(pad.voiceModel)));
        kit.outputBuses[static_cast<std::size_t>(padIndex)] = juce::jlimit(0, mds::kNumPads,
            padXml->getIntAttribute("output", kit.outputBuses[static_cast<std::size_t>(padIndex)]));
    }

    return kit;
}

bool kitsAreEquivalent(const mds::KitPreset& a, const mds::KitPreset& b)
{
    auto closeEnough = [](const float lhs, const float rhs)
    {
        return std::abs(lhs - rhs) <= 1.0e-4f;
    };

    if (a.name != b.name
        || a.familyLabel != b.familyLabel
        || a.mixRole != b.mixRole
        || a.description != b.description
        || a.outputProfile != b.outputProfile
        || !closeEnough(a.nominalPeakDb, b.nominalPeakDb)
        || a.tags != b.tags)
        return false;

    for (int pad = 0; pad < mds::kNumPads; ++pad)
    {
        const auto& lhs = a.pads[static_cast<std::size_t>(pad)];
        const auto& rhs = b.pads[static_cast<std::size_t>(pad)];
        if (!closeEnough(lhs.level, rhs.level)
            || !closeEnough(lhs.tuneSemitones, rhs.tuneSemitones)
            || !closeEnough(lhs.decaySeconds, rhs.decaySeconds)
            || !closeEnough(lhs.attackSeconds, rhs.attackSeconds)
            || !closeEnough(lhs.pitchDropSemitones, rhs.pitchDropSemitones)
            || !closeEnough(lhs.pitchDecaySeconds, rhs.pitchDecaySeconds)
            || !closeEnough(lhs.noiseAmount, rhs.noiseAmount)
            || !closeEnough(lhs.clickAmount, rhs.clickAmount)
            || !closeEnough(lhs.drive, rhs.drive)
            || !closeEnough(lhs.cutoffHz, rhs.cutoffHz)
            || !closeEnough(lhs.pan, rhs.pan)
            || !closeEnough(lhs.baseFrequencyHz, rhs.baseFrequencyHz)
            || lhs.voiceModel != rhs.voiceModel
            || a.outputBuses[static_cast<std::size_t>(pad)] != b.outputBuses[static_cast<std::size_t>(pad)])
        {
            return false;
        }
    }

    return closeEnough(a.fx.outputGainDb, b.fx.outputGainDb)
        && closeEnough(a.fx.compThreshold, b.fx.compThreshold)
        && closeEnough(a.fx.compRatio, b.fx.compRatio)
        && closeEnough(a.fx.compAttack, b.fx.compAttack)
        && closeEnough(a.fx.compRelease, b.fx.compRelease)
        && closeEnough(a.fx.compMix, b.fx.compMix)
        && closeEnough(a.fx.satDrive, b.fx.satDrive)
        && closeEnough(a.fx.satMix, b.fx.satMix)
        && closeEnough(a.fx.transientAttack, b.fx.transientAttack)
        && closeEnough(a.fx.transientSustain, b.fx.transientSustain)
        && closeEnough(a.fx.transientMix, b.fx.transientMix)
        && closeEnough(a.fx.reverbSize, b.fx.reverbSize)
        && closeEnough(a.fx.reverbDamping, b.fx.reverbDamping)
        && closeEnough(a.fx.reverbWidth, b.fx.reverbWidth)
        && closeEnough(a.fx.reverbMix, b.fx.reverbMix)
        && closeEnough(a.fx.reverbPredelay, b.fx.reverbPredelay)
        && closeEnough(a.fx.eqLowFreq, b.fx.eqLowFreq)
        && closeEnough(a.fx.eqLowGain, b.fx.eqLowGain)
        && closeEnough(a.fx.eqMidFreq, b.fx.eqMidFreq)
        && closeEnough(a.fx.eqMidGain, b.fx.eqMidGain)
        && closeEnough(a.fx.eqMidQ, b.fx.eqMidQ)
        && closeEnough(a.fx.eqHighFreq, b.fx.eqHighFreq)
        && closeEnough(a.fx.eqHighGain, b.fx.eqHighGain)
        && closeEnough(a.fx.chorusRate, b.fx.chorusRate)
        && closeEnough(a.fx.chorusDepth, b.fx.chorusDepth)
        && closeEnough(a.fx.chorusMix, b.fx.chorusMix)
        && closeEnough(a.fx.delayTime, b.fx.delayTime)
        && closeEnough(a.fx.delayFeedback, b.fx.delayFeedback)
        && closeEnough(a.fx.delayMix, b.fx.delayMix)
        && closeEnough(a.fx.limiterThreshold, b.fx.limiterThreshold)
        && closeEnough(a.fx.limiterRelease, b.fx.limiterRelease)
        && a.fx.eqEnable == b.fx.eqEnable
        && a.fx.chorusEnable == b.fx.chorusEnable
        && a.fx.delaySync == b.fx.delaySync
        && a.fx.delayEnable == b.fx.delayEnable
        && a.fx.delayNoteDiv == b.fx.delayNoteDiv
        && a.fx.limiterEnable == b.fx.limiterEnable;
}

const mds::KitPreset* findFactoryKitByName(const std::string& name)
{
    for (const auto& kit : mds::getFactoryPresets())
        if (kit.name == name)
            return &kit;
    return nullptr;
}

juce::File getQaDirectory()
{
    auto dir = juce::File::getCurrentWorkingDirectory().getChildFile("qa");
    dir.createDirectory();
    return dir;
}

juce::String csvEscape(const juce::String& text)
{
    auto escaped = text;
    escaped = escaped.replace("\"", "\"\"");
    return "\"" + escaped + "\"";
}

bool hasCompleteMetadata(const mds::KitPreset& kit)
{
    return !kit.familyLabel.empty()
        && !kit.mixRole.empty()
        && !kit.description.empty()
        && !kit.outputProfile.empty()
        && !kit.tags.empty()
        && std::isfinite(kit.nominalPeakDb);
}

juce::String twoDigit(const int value)
{
    return value < 10 ? "0" + juce::String(value) : juce::String(value);
}

float peakMagnitude(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        peak = std::max(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    return peak;
}

bool bufferIsFinite(const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (!std::isfinite(data[i]))
                return false;
    }
    return true;
}

void addBufferTo(juce::AudioBuffer<float>& destination,
                 const juce::AudioBuffer<float>& source,
                 const float gain = 1.0f)
{
    const auto samples = std::min(destination.getNumSamples(), source.getNumSamples());
    const auto channels = std::min(destination.getNumChannels(), source.getNumChannels());
    for (int ch = 0; ch < channels; ++ch)
        destination.addFrom(ch, 0, source, ch, 0, samples, gain);
}

void scaleBufferToPeak(juce::AudioBuffer<float>& buffer,
                       const float targetPeak,
                       const bool allowMakeup)
{
    const auto peak = peakMagnitude(buffer);
    if (peak <= 0.000001f)
        return;

    if (allowMakeup || peak > targetPeak)
        buffer.applyGain(targetPeak / peak);
}

const mds::KitPreset& requireFactoryKit(const char* name)
{
    if (const auto* kit = findFactoryKitByName(name))
        return *kit;

    throw std::runtime_error((std::string("Missing factory kit for drum release suite: ") + name).c_str());
}

int sampleAtBeat(const int barIndex, const double beatInBar, const double beatSeconds)
{
    constexpr double kBeatsPerBar = 4.0;
    const auto seconds = (static_cast<double>(barIndex) * kBeatsPerBar + beatInBar) * beatSeconds;
    return static_cast<int>(std::round(seconds * kSampleRate));
}

enum class DrumReleaseStem : int
{
    Core = 0,
    HatsCymbals,
    PercussionToms,
    Fx,
    Count
};

constexpr int kDrumReleaseStemCount = static_cast<int>(DrumReleaseStem::Count);

juce::String drumReleaseStemName(const int stemIndex)
{
    switch (static_cast<DrumReleaseStem>(stemIndex))
    {
        case DrumReleaseStem::Core:           return "core_kick_snare";
        case DrumReleaseStem::HatsCymbals:    return "hats_cymbals";
        case DrumReleaseStem::PercussionToms: return "percussion_toms";
        case DrumReleaseStem::Fx:             return "fx";
        case DrumReleaseStem::Count:          break;
    }
    return "unknown";
}

int drumReleaseStemForPad(const int padIndex)
{
    switch (padIndex)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            return static_cast<int>(DrumReleaseStem::Core);
        case 4:
        case 5:
        case 10:
            return static_cast<int>(DrumReleaseStem::HatsCymbals);
        case 6:
        case 7:
        case 8:
        case 9:
            return static_cast<int>(DrumReleaseStem::PercussionToms);
        case 11:
            return static_cast<int>(DrumReleaseStem::Fx);
        default:
            return static_cast<int>(DrumReleaseStem::Fx);
    }
}

struct DrumReleaseEvent
{
    const mds::KitPreset* kit = nullptr;
    int sample = 0;
    int padIndex = 0;
    float velocity = 0.8f;
};

std::vector<DrumReleaseEvent> makeDrumReleaseEvents()
{
    constexpr double kBpm = 90.0;
    const auto beatSeconds = 60.0 / kBpm;

    const auto& standard = requireFactoryKit("Classique Standard");
    const auto& trap = requireFactoryKit("Moderne Trap");
    const auto& cinematic = requireFactoryKit("Cinematique Percussion");
    const auto& electro = requireFactoryKit("Moderne Electro");

    std::vector<DrumReleaseEvent> events;
    auto add = [&events, beatSeconds](const mds::KitPreset& kit,
                                      const int bar,
                                      const double beat,
                                      const int pad,
                                      const float velocity)
    {
        events.push_back({ &kit, sampleAtBeat(bar, beat, beatSeconds), pad, velocity });
    };

    for (int bar = 0; bar < 2; ++bar)
    {
        add(standard, bar, 0.00, 0, 0.92f);
        add(standard, bar, 1.50, 1, 0.52f);
        add(standard, bar, 2.00, 0, 0.76f);
        add(standard, bar, 1.00, 2, 0.84f);
        add(standard, bar, 3.00, 2, 0.88f);
        add(standard, bar, 3.00, 3, 0.48f);
        for (int step = 0; step < 8; ++step)
            add(standard, bar, static_cast<double>(step) * 0.5, 4, step % 2 == 0 ? 0.45f : 0.34f);
        add(standard, bar, 3.50, 5, 0.42f);
    }

    for (int bar = 2; bar < 4; ++bar)
    {
        add(trap, bar, 0.00, 0, 0.90f);
        add(trap, bar, 0.75, 1, 0.54f);
        add(trap, bar, 2.00, 0, 0.78f);
        add(trap, bar, 2.50, 1, 0.48f);
        add(trap, bar, 1.00, 2, 0.86f);
        add(trap, bar, 3.00, 2, 0.88f);
        for (int step = 0; step < 16; ++step)
            add(trap, bar, static_cast<double>(step) * 0.25, 4, (step % 4 == 0) ? 0.60f : 0.38f);
        add(trap, bar, 1.75, 5, 0.46f);
        add(trap, bar, 1.875, 4, 0.72f);
        add(trap, bar, 3.50, 5, 0.42f);
        add(trap, bar, 3.625, 4, 0.70f);
    }

    for (int bar = 4; bar < 6; ++bar)
    {
        add(cinematic, bar, 0.00, 0, 0.94f);
        add(cinematic, bar, 0.00, 10, bar == 4 ? 0.72f : 0.48f);
        add(cinematic, bar, 1.00, 6, 0.84f);
        add(cinematic, bar, 1.50, 7, 0.78f);
        add(cinematic, bar, 2.00, 8, 0.82f);
        add(cinematic, bar, 2.50, 9, 0.78f);
        add(cinematic, bar, 3.00, 2, 0.78f);
        add(cinematic, bar, 3.50, 11, 0.58f);
    }

    for (int bar = 6; bar < 8; ++bar)
    {
        add(electro, bar, 0.00, 0, 0.86f);
        add(electro, bar, 0.50, 11, 0.50f);
        add(electro, bar, 1.00, 3, 0.78f);
        add(electro, bar, 2.00, 0, 0.72f);
        add(electro, bar, 3.00, 2, 0.82f);
        add(electro, bar, 3.50, 11, 0.56f);
        for (int step = 0; step < 8; ++step)
            add(electro, bar, static_cast<double>(step) * 0.5, 4, 0.46f);
        add(electro, bar, 2.75, 6, 0.58f);
        add(electro, bar, 3.25, 7, 0.54f);
    }

    return events;
}

juce::AudioBuffer<float> renderReleaseEvents(const std::vector<DrumReleaseEvent>& releaseEvents,
                                             const mds::KitPreset& kit,
                                             const int totalSamples,
                                             const int stemFilter)
{
    std::vector<ScheduledPadEvent> scheduled;
    for (const auto& event : releaseEvents)
    {
        if (event.kit != &kit)
            continue;
        if (stemFilter >= 0 && drumReleaseStemForPad(event.padIndex) != stemFilter)
            continue;
        scheduled.push_back({ event.sample, event.padIndex, event.velocity });
    }

    juce::AudioBuffer<float> buffer(2, totalSamples);
    buffer.clear();
    if (scheduled.empty())
        return buffer;

    buffer = renderScheduledPads(kit, scheduled, static_cast<double>(totalSamples) / kSampleRate);
    applyMasterFxChain(buffer, kit.fx);
    return buffer;
}

struct DrumReleaseReportRow
{
    juce::String file;
    juce::String type;
    juce::String kit;
    juce::String target;
    AudioMetrics metrics;
    bool finite = true;
    juce::String status;
    juce::String notes;
};

DrumReleaseReportRow makeDrumReleaseReportRow(const juce::File& file,
                                              const juce::String& type,
                                              const juce::String& kit,
                                              const juce::String& target,
                                              const juce::AudioBuffer<float>& buffer,
                                              const juce::String& notes,
                                              const bool requireMainHeadroom)
{
    DrumReleaseReportRow row;
    row.file = file.getRelativePathFrom(juce::File::getCurrentWorkingDirectory());
    row.type = type;
    row.kit = kit;
    row.target = target;
    row.metrics = measureBuffer(buffer);
    row.finite = bufferIsFinite(buffer);
    row.notes = notes;

    const bool audible = row.metrics.peakDb >= musique::qa::kMinimumAudiblePeakDb;
    const bool clipped = row.metrics.peakDb > musique::qa::kMaximumPeakDb + musique::qa::kClippingToleranceDb;
    const bool headroomOk = !requireMainHeadroom || row.metrics.peakDb <= -1.0f;
    row.status = (row.finite && audible && !clipped && headroomOk) ? "PASS" : "FAIL";
    return row;
}

void writeDrumReleaseReport(const juce::File& reportFile,
                            const std::vector<DrumReleaseReportRow>& rows)
{
    juce::StringArray lines;
    lines.add("file,type,kit,target,peak_db,rms_db,crest_db,tail_ms,stereo_width,finite,status,notes");
    for (const auto& row : rows)
    {
        const auto crestDb = row.metrics.peakDb - row.metrics.rmsDb;
        lines.add(csvEscape(row.file) + ","
            + csvEscape(row.type) + ","
            + csvEscape(row.kit) + ","
            + csvEscape(row.target) + ","
            + juce::String(row.metrics.peakDb, 2) + ","
            + juce::String(row.metrics.rmsDb, 2) + ","
            + juce::String(crestDb, 2) + ","
            + juce::String(row.metrics.tailMs, 2) + ","
            + juce::String(row.metrics.stereoWidth, 4) + ","
            + juce::String(row.finite ? "true" : "false") + ","
            + csvEscape(row.status) + ","
            + csvEscape(row.notes));
    }

    reportFile.getParentDirectory().createDirectory();
    reportFile.replaceWithText(lines.joinIntoString("\n"));
}

const mds::KitPreset& identityKitForPad(const int pad)
{
    switch (pad)
    {
        case 2:
        case 3:
            return requireFactoryKit("Acoustique Studio");
        case 4:
        case 5:
            return requireFactoryKit("Moderne Trap");
        case 6:
        case 7:
        case 8:
        case 9:
            return requireFactoryKit("Cinematique Percussion");
        case 10:
            return requireFactoryKit("Cinematique Epic");
        case 11:
            return requireFactoryKit("Moderne Electro");
        default:
            return requireFactoryKit("Classique Standard");
    }
}

static int runRenderReleaseSuite(const juce::File& outputBase,
                                 const juce::File& reportFile)
{
    constexpr double kBars = 8.0;
    constexpr double kBeatsPerBar = 4.0;
    constexpr double kBpm = 90.0;
    constexpr double kTailSeconds = 2.0;
    constexpr float kMainTargetPeak = 0.89f; // -1.01 dBFS.
    const auto durationSeconds = (kBars * kBeatsPerBar * 60.0 / kBpm) + kTailSeconds;
    const int totalSamples = static_cast<int>(std::ceil(durationSeconds * kSampleRate));

    outputBase.createDirectory();
    const auto stemsDir = outputBase.getChildFile("stems");
    const auto identityDir = outputBase.getChildFile("identity");
    stemsDir.createDirectory();
    identityDir.createDirectory();

    const auto releaseEvents = makeDrumReleaseEvents();
    const std::array<const mds::KitPreset*, 4> kits =
    {{
        &requireFactoryKit("Classique Standard"),
        &requireFactoryKit("Moderne Trap"),
        &requireFactoryKit("Cinematique Percussion"),
        &requireFactoryKit("Moderne Electro")
    }};

    std::array<juce::AudioBuffer<float>, kDrumReleaseStemCount> stems;
    juce::AudioBuffer<float> mainBuffer(2, totalSamples);
    mainBuffer.clear();
    for (auto& stem : stems)
    {
        stem.setSize(2, totalSamples);
        stem.clear();
    }

    for (const auto* kit : kits)
    {
        for (int stemIndex = 0; stemIndex < kDrumReleaseStemCount; ++stemIndex)
        {
            auto buffer = renderReleaseEvents(releaseEvents, *kit, totalSamples, stemIndex);
            addBufferTo(stems[static_cast<std::size_t>(stemIndex)], buffer);
            addBufferTo(mainBuffer, buffer);
        }
    }

    const auto mainPeakBefore = peakMagnitude(mainBuffer);
    if (mainPeakBefore > 0.000001f)
    {
        const auto scale = kMainTargetPeak / mainPeakBefore;
        mainBuffer.applyGain(scale);
        for (auto& stem : stems)
            stem.applyGain(scale);
    }

    std::vector<DrumReleaseReportRow> rows;
    const auto mainFile = outputBase.getChildFile("main.wav");
    if (!writeWav(mainFile, mainBuffer))
        throw std::runtime_error(("Failed to write drum release main WAV: " + mainFile.getFullPathName()).toStdString());
    rows.push_back(makeDrumReleaseReportRow(mainFile, "main", "MULTI", "8-bar deterministic drum RC groove",
                                            mainBuffer, "slow groove, fast hats, cinematic fill, electro outro", true));

    for (int stemIndex = 0; stemIndex < kDrumReleaseStemCount; ++stemIndex)
    {
        const auto stemName = drumReleaseStemName(stemIndex);
        const auto file = stemsDir.getChildFile(stemName + ".wav");
        auto& buffer = stems[static_cast<std::size_t>(stemIndex)];
        if (!writeWav(file, buffer))
            throw std::runtime_error(("Failed to write drum release stem WAV: " + file.getFullPathName()).toStdString());

        rows.push_back(makeDrumReleaseReportRow(file, "stem", "MULTI", stemName, buffer,
                                                "stem from release groove suite", false));
    }

    for (int pad = 0; pad < mds::kNumPads; ++pad)
    {
        const auto& kit = identityKitForPad(pad);
        std::vector<ScheduledPadEvent> events { { 0, pad, 0.88f } };
        auto buffer = renderScheduledPads(kit, events, 3.0);
        applyMasterFxChain(buffer, kit.fx);
        scaleBufferToPeak(buffer, 0.62f, true);

        const auto padName = juce::String(mds::kPadNames[static_cast<std::size_t>(pad)]);
        const auto file = identityDir.getChildFile(twoDigit(pad) + "_" + slug(padName) + ".wav");
        if (!writeWav(file, buffer))
            throw std::runtime_error(("Failed to write drum release identity WAV: " + file.getFullPathName()).toStdString());

        rows.push_back(makeDrumReleaseReportRow(file, "identity", juce::String(kit.name),
                                                padName, buffer, "single-pad identity render", false));
    }

    writeDrumReleaseReport(reportFile, rows);

    int fails = 0;
    for (const auto& row : rows)
        if (row.status != "PASS")
            ++fails;

    const auto identityCount = std::count_if(rows.begin(), rows.end(), [] (const DrumReleaseReportRow& row)
    {
        return row.type == "identity";
    });
    if (identityCount != mds::kNumPads)
        ++fails;

    std::cout << "Drum release suite: " << (static_cast<int>(rows.size()) - fails)
              << "/" << rows.size() << " checks passed";
    if (fails > 0)
        std::cout << "  (" << fails << " failed)";
    std::cout << "\nOutput: " << outputBase.getFullPathName()
              << "\nReport: " << reportFile.getFullPathName() << "\n";
    return fails > 0 ? 1 : 0;
}
} // namespace

// =============================================================================
// --validate-presets : render one hit per kit/pad, check peak / NaN / Inf
// =============================================================================
static int runValidatePresets(const juce::String& reportPath = {})
{
    using namespace mds;
    constexpr float kMinPeakDb = musique::qa::kMinimumAudiblePeakDb;
    constexpr float kMaxPeakDb = musique::qa::kMaximumPeakDb + musique::qa::kClippingToleranceDb;
    constexpr std::array<float, 3> kVelocities = { 0.35f, 0.8f, 1.0f };
    constexpr std::array<double, 3> kDurations = { 0.25, 0.5, 1.25 };

    int fails = 0;
    int checks = 0;
    juce::StringArray reportLines;
    reportLines.add("kit_name,family,mix_role,output_profile,nominal_peak_db,measured_peak_db,rms_db,crest_db,tail_ms,stereo_width,metadata_complete,status,tags");

    for (const auto& kit : getFactoryPresets())
    {
        const int kitFailsAtStart = fails;

        for (int pad = 0; pad < kNumPads; ++pad)
        {
            const auto& settings = kit.pads[static_cast<std::size_t>(pad)];

            for (const auto velocity : kVelocities)
            {
                for (const auto durationSeconds : kDurations)
                {
                    ++checks;
                    auto buffer = renderSingleHit(settings, velocity, durationSeconds);
                    bool hasNaN = false;
                    for (int ch = 0; ch < buffer.getNumChannels() && !hasNaN; ++ch)
                    {
                        const auto* data = buffer.getReadPointer(ch);
                        for (int i = 0; i < buffer.getNumSamples(); ++i)
                        {
                            if (std::isnan(data[i]) || std::isinf(data[i]))
                            {
                                hasNaN = true;
                                break;
                            }
                        }
                    }

                    const auto metrics = measureBuffer(buffer);
                    const bool wantsStereo = settings.voiceModel == PadVoiceModel::Clap
                        || settings.voiceModel == PadVoiceModel::Crash
                        || settings.voiceModel == PadVoiceModel::Fx
                        || std::abs(settings.pan) > 0.08f;
                    const float minTailMs = settings.decaySeconds >= 0.14f ? 28.0f : 6.0f;

                    if (hasNaN)
                    {
                        ++fails;
                        std::cout << "[FAIL] " << kit.name << " / pad " << pad
                                  << " : NaN or Inf in output\n";
                    }
                    else if (metrics.peakDb < kMinPeakDb)
                    {
                        ++fails;
                        std::cout << "[FAIL] " << kit.name << " / pad " << pad
                                  << " : silent (peak " << metrics.peakDb << " dBFS)\n";
                    }
                    else if (metrics.peakDb > kMaxPeakDb)
                    {
                        ++fails;
                        std::cout << "[FAIL] " << kit.name << " / pad " << pad
                                  << " : clipping (peak " << metrics.peakDb << " dBFS)\n";
                    }
                    else if (durationSeconds >= 1.0 && metrics.tailMs < minTailMs)
                    {
                        ++fails;
                        std::cout << "[FAIL] " << kit.name << " / pad " << pad
                                  << " : tail too short (" << metrics.tailMs << " ms)\n";
                    }
                    else if (wantsStereo && metrics.stereoWidth < 0.01f)
                    {
                        ++fails;
                        std::cout << "[FAIL] " << kit.name << " / pad " << pad
                                  << " : stereo image collapsed (" << metrics.stereoWidth << ")\n";
                    }
                }
            }
        }

        ++checks;
        {
            auto chokeKit = kit;
            chokeKit.pads[5].decaySeconds = juce::jmax(chokeKit.pads[5].decaySeconds, 0.18f);
            chokeKit.pads[5].level = juce::jmax(chokeKit.pads[5].level, 0.42f);
            chokeKit.pads[4].decaySeconds = juce::jmin(chokeKit.pads[4].decaySeconds, 0.03f);

            auto openHat = renderScheduledPads(chokeKit, { { 0, 5, 0.9f } }, 0.5);
            auto chokedHat = renderScheduledPads(chokeKit, {
                { 0, 5, 0.9f },
                { static_cast<int>(0.06 * kSampleRate), 4, 0.85f }
            }, 0.5);
            const auto openTailRms = measureWindowRmsDb(openHat, static_cast<int>(0.12 * kSampleRate));
            const auto chokedTailRms = measureWindowRmsDb(chokedHat, static_cast<int>(0.12 * kSampleRate));
            if (chokedTailRms > openTailRms - 1.5f)
            {
                ++fails;
                std::cout << "[FAIL] " << kit.name
                          << " : hat choke ineffective (open tail rms " << openTailRms
                          << " dB, choked tail rms " << chokedTailRms << " dB)\n";
            }
        }

        ++checks;
        {
            auto dry = renderGroupHits(kit, { 0, 2, 10, 11 }, 0.9f, 1.4);
            auto wet = dry;
            applyMasterFxChain(wet, kit.fx);
            const auto dryMetrics = measureBuffer(dry);
            const auto wetMetrics = measureBuffer(wet);
            const bool changedEnough =
                std::abs(wetMetrics.peakDb - dryMetrics.peakDb) >= 0.15f
                || std::abs(wetMetrics.tailMs - dryMetrics.tailMs) >= 20.0f
                || std::abs(wetMetrics.stereoWidth - dryMetrics.stereoWidth) >= 0.01f;
            if (!changedEnough)
            {
                ++fails;
                std::cout << "[FAIL] " << kit.name
                          << " : FX chain produced no meaningful delta\n";
            }
        }

        ++checks;
        {
            auto auxHot = renderGroupHits(kit, { 0, 2, 10 }, 1.0f, 1.0);
            auxHot.applyGain(1.8f);
            applyAuxBusSafety(auxHot);
            const auto auxMetrics = measureBuffer(auxHot);
            if (auxMetrics.peakDb > -0.1f)
            {
                ++fails;
                std::cout << "[FAIL] " << kit.name
                          << " : aux safety peak too high (" << auxMetrics.peakDb << " dBFS)\n";
            }
        }

        ++checks;
        {
            const auto xml = writeKitPresetXml(kit);
            const auto roundTrip = readKitPresetXml(*xml, kit);
            if (!kitsAreEquivalent(kit, roundTrip))
            {
                ++fails;
                std::cout << "[FAIL] " << kit.name << " : preset round-trip mismatch\n";
            }
        }

        auto kitRender = renderGroupHits(kit, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, 0.9f, 1.6);
        applyMasterFxChain(kitRender, kit.fx);
        const auto kitMetrics = measureBuffer(kitRender);
        const auto crestDb = kitMetrics.peakDb - kitMetrics.rmsDb;
        const bool metadataComplete = hasCompleteMetadata(kit);

        ++checks;
        if (!metadataComplete)
        {
            ++fails;
            std::cout << "[FAIL] " << kit.name << " : incomplete factory metadata\n";
        }

        ++checks;
        if (kitMetrics.peakDb > -0.3f)
        {
            ++fails;
            std::cout << "[FAIL] " << kit.name
                      << " : mix-ready peak too high (" << kitMetrics.peakDb << " dBFS)\n";
        }

        ++checks;
        if (std::abs(kitMetrics.peakDb - kit.nominalPeakDb) > 1.5f)
        {
            ++fails;
            std::cout << "[FAIL] " << kit.name
                      << " : nominal peak mismatch (measured " << kitMetrics.peakDb
                      << " dBFS, target " << kit.nominalPeakDb << " dBFS)\n";
        }

        juce::StringArray tagValues;
        for (const auto& tag : kit.tags)
            tagValues.add(juce::String(tag));

        const bool kitPassed = fails == kitFailsAtStart;
        reportLines.add(csvEscape(juce::String(kit.name)) + ","
                        + csvEscape(juce::String(kit.familyLabel)) + ","
                        + csvEscape(juce::String(kit.mixRole)) + ","
                        + csvEscape(juce::String(kit.outputProfile)) + ","
                        + juce::String(kit.nominalPeakDb, 2) + ","
                        + juce::String(kitMetrics.peakDb, 2) + ","
                        + juce::String(kitMetrics.rmsDb, 2) + ","
                        + juce::String(crestDb, 2) + ","
                        + juce::String(kitMetrics.tailMs, 2) + ","
                        + juce::String(kitMetrics.stereoWidth, 4) + ","
                        + juce::String(metadataComplete ? "true" : "false") + ","
                        + juce::String(kitPassed ? "OK" : "FAIL") + ","
                        + csvEscape(tagValues.joinIntoString(";")));
    }

    const auto reportFile = reportPath.isNotEmpty()
        ? juce::File(reportPath)
        : getQaDirectory().getChildFile("drum_preset_qa_report.csv");
    reportFile.getParentDirectory().createDirectory();
    reportFile.replaceWithText(reportLines.joinIntoString("\n"));

    std::cout << "Preset validation: " << (checks - fails) << "/" << checks << " passed";
    if (fails > 0) std::cout << "  (" << fails << " failed)";
    std::cout << "\nReport: " << reportFile.getFullPathName() << "\n";
    return fails > 0 ? 1 : 0;
}

// =============================================================================
// --validate-matrix : check factory presets against SOUND_DESIGN_MATRIX targets
// =============================================================================
static void runValidateMatrix()
{
    using namespace mds;
    struct GroupSpec
    {
        const char* label = "";
        float minPeakDb = -24.0f;
        float maxPeakDb = -1.0f;
        float minHfRatio = 0.02f;
        float maxHfRatio = 0.95f;
        float minTailMs = 10.0f;
        float maxTailMs = 1200.0f;
        float minStereoWidth = 0.0f;
        float maxStereoWidth = 0.50f;
    };

    const GroupSpec kickSpec  { "Kick",  -18.0f, -0.05f, 0.008f, 0.18f,  40.0f, 1050.0f, 0.00f, 0.06f };
    const GroupSpec snareSpec { "Snare", -22.0f, -1.2f,  0.06f, 0.92f,  18.0f, 460.0f,  0.00f, 0.24f };
    const GroupSpec hatSpec   { "Hat",   -28.0f, -6.0f,  0.14f, 1.00f,   6.0f, 300.0f,  0.02f, 0.28f };
    const GroupSpec crashSpec { "Crash", -30.0f, -8.0f,  0.08f, 0.95f,  60.0f, 1300.0f, 0.02f, 0.35f };
    const GroupSpec fxSpec    { "FX",    -28.0f, -4.0f,  0.04f, 0.92f,  10.0f, 720.0f,  0.02f, 0.35f };

    auto validateGroup = [&](const std::string& kitName,
                             const GroupSpec& spec,
                             const AudioMetrics& metrics,
                             int& fails,
                             int& checks)
    {
        ++checks;
        const bool ok =
            metrics.peakDb >= spec.minPeakDb && metrics.peakDb <= spec.maxPeakDb &&
            metrics.hfRatio >= spec.minHfRatio && metrics.hfRatio <= spec.maxHfRatio &&
            metrics.tailMs >= spec.minTailMs && metrics.tailMs <= spec.maxTailMs &&
            metrics.stereoWidth >= spec.minStereoWidth && metrics.stereoWidth <= spec.maxStereoWidth;
        if (!ok)
        {
            ++fails;
            std::cout << "[FAIL] " << kitName << " / " << spec.label
                      << " peak=" << metrics.peakDb << "dB"
                      << " hf=" << metrics.hfRatio
                      << " tail=" << metrics.tailMs << "ms"
                      << " stereo=" << metrics.stereoWidth << "\n";
        }
    };

    int fails = 0;
    int checks = 0;
    for (const auto& kit : getFactoryPresets())
    {
        validateGroup(kit.name, kickSpec,  measureBuffer(renderGroupHits(kit, { 0, 1 }, 0.9f, 1.0)), fails, checks);
        validateGroup(kit.name, snareSpec, measureBuffer(renderGroupHits(kit, { 2, 3 }, 0.9f, 0.8)), fails, checks);
        validateGroup(kit.name, hatSpec,   measureBuffer(renderGroupHits(kit, { 4, 5 }, 0.9f, 0.4)), fails, checks);
        validateGroup(kit.name, crashSpec, measureBuffer(renderGroupHits(kit, { 10 },   0.9f, 1.2)), fails, checks);
        validateGroup(kit.name, fxSpec,    measureBuffer(renderGroupHits(kit, { 11 },   0.9f, 1.0)), fails, checks);
    }
    std::cout << "Matrix validation: " << (checks - fails) << "/" << checks << " rendered groups within tolerance";
    if (fails > 0)
        std::cout << "  (" << fails << " outside tolerance)";
    std::cout << "\n";
}

// =============================================================================
// --benchmark : render 64 voices of each model on a 512-sample buffer
// =============================================================================
static int runBenchmark(const juce::String& baselineCsvPath = {}, const juce::String& reportPath = {})
{
    struct BenchmarkScenario
    {
        const char* name = "";
        const mds::KitPreset* kit = nullptr;
        std::vector<ScheduledPadEvent> events;
        double durationSeconds = 1.0;
        bool applyFx = false;
    };

    auto secondsToSample = [](const double seconds)
    {
        return static_cast<int>(std::round(seconds * kSampleRate));
    };

    auto addHit = [&](std::vector<ScheduledPadEvent>& events, const double seconds, const int pad, const float velocity)
    {
        events.push_back({ secondsToSample(seconds), pad, velocity });
    };

    const auto* classique = findFactoryKitByName("Classique Standard");
    const auto* club = findFactoryKitByName("Moderne Club");
    const auto* trap = findFactoryKitByName("Moderne Trap");
    const auto* electro = findFactoryKitByName("Moderne Electro");
    const auto& fallback = mds::getFactoryPresets().front();

    std::vector<BenchmarkScenario> scenarios;

    {
        BenchmarkScenario singleHit;
        singleHit.name = "Single Hit";
        singleHit.kit = classique != nullptr ? classique : &fallback;
        singleHit.durationSeconds = 1.0;
        addHit(singleHit.events, 0.00, 0, 0.95f);
        addHit(singleHit.events, 0.22, 2, 0.88f);
        addHit(singleHit.events, 0.38, 4, 0.72f);
        addHit(singleHit.events, 0.62, 10, 0.82f);
        scenarios.push_back(std::move(singleHit));
    }

    {
        BenchmarkScenario denseGroove;
        denseGroove.name = "Dense Groove";
        denseGroove.kit = club != nullptr ? club : &fallback;
        denseGroove.durationSeconds = 2.0;
        for (int step = 0; step < 16; ++step)
            addHit(denseGroove.events, step * 0.125, 4, (step % 4 == 0) ? 0.78f : 0.64f);
        for (double beat : { 0.0, 0.5, 1.0, 1.5 })
            addHit(denseGroove.events, beat, 0, 0.96f);
        for (double beat : { 0.5, 1.5 })
        {
            addHit(denseGroove.events, beat, 2, 0.92f);
            addHit(denseGroove.events, beat, 3, 0.76f);
        }
        addHit(denseGroove.events, 0.75, 5, 0.74f);
        addHit(denseGroove.events, 1.75, 5, 0.70f);
        addHit(denseGroove.events, 1.50, 8, 0.80f);
        addHit(denseGroove.events, 1.75, 9, 0.78f);
        scenarios.push_back(std::move(denseGroove));
    }

    {
        BenchmarkScenario hatChoke;
        hatChoke.name = "Hat Choke";
        hatChoke.kit = trap != nullptr ? trap : &fallback;
        hatChoke.durationSeconds = 1.5;
        for (int step = 0; step < 12; ++step)
        {
            const double t = step * 0.125;
            if ((step % 3) == 0)
                addHit(hatChoke.events, t, 5, 0.78f);
            addHit(hatChoke.events, t + 0.0625, 4, 0.70f);
        }
        addHit(hatChoke.events, 0.00, 0, 0.95f);
        addHit(hatChoke.events, 0.75, 2, 0.88f);
        scenarios.push_back(std::move(hatChoke));
    }

    {
        BenchmarkScenario fxChain;
        fxChain.name = "FX Chain";
        fxChain.kit = electro != nullptr ? electro : &fallback;
        fxChain.durationSeconds = 2.0;
        fxChain.applyFx = true;
        for (int step = 0; step < 16; ++step)
        {
            addHit(fxChain.events, step * 0.125, 4, 0.66f);
            if ((step % 4) == 2)
                addHit(fxChain.events, step * 0.125, 11, 0.62f);
        }
        for (double beat : { 0.0, 0.75, 1.0, 1.5 })
            addHit(fxChain.events, beat, 0, 0.92f);
        for (double beat : { 0.5, 1.5 })
            addHit(fxChain.events, beat, 2, 0.90f);
        addHit(fxChain.events, 0.00, 10, 0.76f);
        scenarios.push_back(std::move(fxChain));
    }

    constexpr int kIterations = 40;
    std::cout << "Benchmark: representative production scenarios x " << kIterations << " iterations\n";
    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << " logical CPUs\n";

    std::map<juce::String, double> baselineValues;
    if (baselineCsvPath.isNotEmpty())
    {
        const auto baselineFile = juce::File(baselineCsvPath);
        if (!baselineFile.existsAsFile())
            throw std::runtime_error(("Benchmark baseline file not found: " + baselineCsvPath).toStdString());

        const auto lines = juce::StringArray::fromLines(baselineFile.loadFileAsString());
        for (int lineIndex = 1; lineIndex < lines.size(); ++lineIndex)
        {
            const auto line = lines[lineIndex].trim();
            if (line.isEmpty())
                continue;

            const auto cols = juce::StringArray::fromTokens(line, ",", "\"");
            if (cols.size() < 3)
                continue;

            baselineValues[cols[0].trim() + "|" + cols[1].trim()] = cols[2].trim().getDoubleValue();
        }
    }

    double maxCpuPct = 0.0;
    int baselineRegressions = 0;
    juce::StringArray reportLines;
    reportLines.add("scenario,kit,cpu_pct,duration_seconds,iterations,baseline_cpu_pct,regression_pct");
    for (const auto& scenario : scenarios)
    {
        auto renderPass = [&]()
        {
            auto buffer = renderScheduledPads(*scenario.kit, scenario.events, scenario.durationSeconds);
            if (scenario.applyFx)
                applyMasterFxChain(buffer, scenario.kit->fx);
        };

        renderPass();
        const auto wallStart = std::chrono::high_resolution_clock::now();
        for (int iter = 0; iter < kIterations; ++iter)
            renderPass();
        const auto wallEnd = std::chrono::high_resolution_clock::now();

        const double wallMs = std::chrono::duration<double, std::milli>(wallEnd - wallStart).count();
        const double audioMs = scenario.durationSeconds * 1000.0 * static_cast<double>(kIterations);
        const double cpuPct = wallMs / audioMs * 100.0;
        maxCpuPct = std::max(maxCpuPct, cpuPct);

        const auto key = juce::String(scenario.name) + "|" + juce::String(scenario.kit->name);
        const auto baselineIt = baselineValues.find(key);
        const double baselineCpuPct = baselineIt != baselineValues.end() ? baselineIt->second : 0.0;
        const double regressionPct = baselineCpuPct > 0.0 ? ((cpuPct / baselineCpuPct) - 1.0) * 100.0 : 0.0;
        if (baselineCpuPct > 0.0 && cpuPct > baselineCpuPct * 1.20)
            ++baselineRegressions;

        std::cout << "  " << scenario.name << " / " << scenario.kit->name << " : " << cpuPct << "% CPU";
        if (cpuPct > 5.0)
            std::cout << "  *** EXCEEDS 5% TARGET ***";
        if (baselineCpuPct > 0.0 && cpuPct > baselineCpuPct * 1.20)
            std::cout << "  *** >20% ABOVE BASELINE (" << baselineCpuPct << "%) ***";
        std::cout << "\n";

        reportLines.add(csvEscape(juce::String(scenario.name)) + ","
                        + csvEscape(juce::String(scenario.kit->name)) + ","
                        + juce::String(cpuPct, 4) + ","
                        + juce::String(scenario.durationSeconds, 2) + ","
                        + juce::String(kIterations) + ","
                        + (baselineCpuPct > 0.0 ? juce::String(baselineCpuPct, 4) : juce::String()) + ","
                        + (baselineCpuPct > 0.0 ? juce::String(regressionPct, 2) : juce::String()));
    }

    const auto reportFile = reportPath.isNotEmpty()
        ? juce::File(reportPath)
        : getQaDirectory().getChildFile("drum_cpu_benchmark.csv");
    reportFile.getParentDirectory().createDirectory();
    reportFile.replaceWithText(reportLines.joinIntoString("\n"));

    std::cout << "Peak CPU across scenarios: " << maxCpuPct << "% (target < 5%)\n";
    std::cout << "Report: " << reportFile.getFullPathName() << "\n";
    return baselineRegressions > 0 ? 1 : 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: UWdeVST_drum_renderer <manifest.csv> [--output-base <dir>] [--limit <n>] [--overwrite]\n"
                     "       UWdeVST_drum_renderer --validate-matrix\n"
                     "       UWdeVST_drum_renderer --validate-presets [--report <preset_qa.csv>]\n"
                     "       UWdeVST_drum_renderer --benchmark [--baseline <benchmark.csv>] [--report <benchmark.csv>]\n"
                     "       UWdeVST_drum_renderer --render-release-suite [--output-base <dir>] [--report <csv>]\n"
                     "       UWdeVST_drum_renderer --render-pad-presets [--output-base <dir>] [--overwrite]\n";
        return 1;
    }

    const juce::String firstArg(argv[1]);
    if (firstArg == "--validate-matrix")
    {
        runValidateMatrix();
        return 0;
    }
    if (firstArg == "--validate-presets")
    {
        juce::String reportPath;
        for (int i = 2; i < argc; ++i)
        {
            const juce::String key(argv[i]);
            if (key == "--report" && i + 1 < argc)
                reportPath = juce::String(argv[++i]);
        }
        return runValidatePresets(reportPath);
    }
    if (firstArg == "--benchmark")
    {
        juce::String baselinePath;
        juce::String reportPath;
        for (int i = 2; i < argc; ++i)
        {
            const juce::String key(argv[i]);
            if (key == "--baseline" && i + 1 < argc)
                baselinePath = juce::String(argv[++i]);
            else if (key == "--report" && i + 1 < argc)
                reportPath = juce::String(argv[++i]);
        }
        return runBenchmark(baselinePath, reportPath);
    }
    if (firstArg == "--render-release-suite")
    {
        auto outputBase = juce::File::getCurrentWorkingDirectory().getChildFile("qa/drum_release_suite");
        auto reportFile = juce::File::getCurrentWorkingDirectory().getChildFile("qa/drum_release_suite_report.csv");
        for (int i = 2; i < argc; ++i)
        {
            const juce::String key(argv[i]);
            if (key == "--output-base" && i + 1 < argc)
                outputBase = juce::File(argv[++i]);
            else if (key == "--report" && i + 1 < argc)
                reportFile = juce::File(argv[++i]);
        }
        return runRenderReleaseSuite(outputBase, reportFile);
    }
    if (firstArg == "--render-pad-presets")
    {
        using namespace mds;
        juce::File outputBase = juce::File::getCurrentWorkingDirectory().getChildFile("pad_preset_samples");
        bool overwrite = false;

        for (int i = 2; i < argc; ++i)
        {
            const juce::String key(argv[i]);
            if (key == "--output-base" && i + 1 < argc) outputBase = juce::File(argv[++i]);
            else if (key == "--overwrite") overwrite = true;
        }

        int rendered = 0;
        int total = 0;
        for (int pad = 0; pad < kNumPads; ++pad)
        {
            const auto& presets = getFactoryPadPresets(pad);
            total += static_cast<int>(presets.size());
        }

        for (int pad = 0; pad < kNumPads; ++pad)
        {
            const auto& padName = kPadNames[static_cast<std::size_t>(pad)];
            const auto& presets = getFactoryPadPresets(pad);

            for (std::size_t p = 0; p < presets.size(); ++p)
            {
                const auto& preset = presets[p];
                const auto slugName = slug(juce::String(preset.name.c_str()));
                const auto padSlug = slug(juce::String(padName));
                const auto relativePath = padSlug + "/" + slugName + ".wav";
                const auto outputFile = outputBase.getChildFile(relativePath.replaceCharacter('/', juce::File::getSeparatorChar()));

                if (outputFile.existsAsFile() && !overwrite)
                {
                    ++rendered;
                    continue;
                }

                const auto& settings = preset.settings;
                const float duration = settings.decaySeconds * 2.2f + 0.15f;
                const int renderSamples = std::max(1, static_cast<int>(std::ceil((duration + 0.5) * kSampleRate)));
                juce::AudioBuffer<float> buffer(2, renderSamples);
                buffer.clear();

                auto voice = createVoiceForModel(settings.voiceModel);
                if (voice != nullptr)
                {
                    voice->start(settings, 0.85f, kSampleRate);
                    voice->render(buffer, 0, renderSamples);
                }

                FxSettings fx;
                const auto& presetFx = preset.fx;
                fx.compThresholdDb = presetFx.compThreshold;
                fx.compRatio = presetFx.compRatio;
                fx.compAttackMs = presetFx.compAttack;
                fx.compReleaseMs = presetFx.compRelease;
                fx.compMix = presetFx.compMix;
                fx.satDrive = presetFx.satDrive;
                fx.satMix = presetFx.satMix;
                fx.transientAttack = presetFx.transientAttack;
                fx.transientSustain = presetFx.transientSustain;
                fx.transientMix = presetFx.transientMix;
                fx.reverbSize = presetFx.reverbSize;
                fx.reverbDamping = presetFx.reverbDamping;
                fx.reverbWidth = presetFx.reverbWidth;
                fx.reverbWet = presetFx.reverbMix;
                fx.reverbPredelay = presetFx.reverbPredelay;
                fx.eqEnable = presetFx.eqEnable;
                fx.eqLowFreq = presetFx.eqLowFreq;
                fx.eqLowGain = presetFx.eqLowGain;
                fx.eqMidFreq = presetFx.eqMidFreq;
                fx.eqMidGain = presetFx.eqMidGain;
                fx.eqMidQ = presetFx.eqMidQ;
                fx.eqHighFreq = presetFx.eqHighFreq;
                fx.eqHighGain = presetFx.eqHighGain;
                fx.targetPeak = 0.92f;

                applyTransient(buffer, fx);
                applySaturator(buffer, fx);
                applyEQ(buffer, fx);
                applyCompressor(buffer, fx);
                applyStereoWidth(buffer, fx);
                if (presetFx.reverbMix > 0.001f)
                    applyReverb(buffer, fx);
                applyDcHighPass(buffer);

                int first = 0;
                int last = renderSamples - 1;
                auto magnitudeAt = [&buffer](const int sample)
                {
                    float mag = 0.0f;
                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                        mag = juce::jmax(mag, std::abs(buffer.getSample(ch, sample)));
                    return mag;
                };
                while (first < renderSamples && magnitudeAt(first) < 0.0007f) ++first;
                while (last > first && magnitudeAt(last) < 0.0007f) --last;
                first = juce::jmax(0, first - 48);
                last = juce::jmin(renderSamples - 1, last + 900);
                const auto newLength = juce::jmax(1, last - first + 1);
                juce::AudioBuffer<float> trimmed(2, newLength);
                for (int ch = 0; ch < 2; ++ch)
                    trimmed.copyFrom(ch, 0, buffer, ch, first, newLength);
                buffer.makeCopyOf(trimmed);

                const auto fadeIn = juce::jmin(48, buffer.getNumSamples() / 6);
                const auto fadeOut = juce::jmin(160, buffer.getNumSamples() / 3);
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    if (fadeIn > 0)
                        buffer.applyGainRamp(ch, 0, fadeIn, 0.0f, 1.0f);
                    if (fadeOut > 0)
                        buffer.applyGainRamp(ch, buffer.getNumSamples() - fadeOut, fadeOut, 1.0f, 0.0f);
                }

                float peak = 0.0f;
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    peak = juce::jmax(peak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
                if (peak > 0.0001f)
                    buffer.applyGain(fx.targetPeak / peak);

                if (!writeWav(outputFile, buffer))
                    std::cerr << "Failed to write: " << outputFile.getFullPathName() << "\n";

                ++rendered;
                std::cout << "[" << rendered << "/" << total << "] " << padName << " / " << preset.name << "\n";
            }
        }

        std::cout << "Rendered " << rendered << " pad preset WAV files into " << outputBase.getFullPathName() << "\n";
        return 0;
    }

    const juce::File manifestFile(argv[1]);
    juce::File outputBase = juce::File::getCurrentWorkingDirectory().getChildFile("render_output_drum");
    int limit = -1;
    bool overwrite = false;

    for (int i = 2; i < argc; ++i)
    {
        const juce::String key(argv[i]);
        if (key == "--output-base" && i + 1 < argc) outputBase = juce::File(argv[++i]);
        else if (key == "--limit" && i + 1 < argc) limit = juce::String(argv[++i]).getIntValue();
        else if (key == "--overwrite") overwrite = true;
    }

    try
    {
        auto jobs = readManifestCsv(manifestFile);
        if (limit > 0 && limit < static_cast<int>(jobs.size()))
            jobs.resize(static_cast<std::size_t>(limit));

        int rendered = 0;
        for (const auto& job : jobs)
        {
            const auto padIndex = padIndexFromName(job.instrument);
            if (padIndex < 0)
            {
                std::cerr << "[WARN] Unknown drum pad in manifest: " << job.instrument << " -> skipped\n";
                continue;
            }

            const auto outputFile = outputBase.getChildFile(job.finalRelativePath.replaceCharacter('/', juce::File::getSeparatorChar()));
            if (outputFile.existsAsFile() && !overwrite)
                continue;

            auto settings = seedSettingsForPad(padIndex, job);
            applyPresetMacro(job, settings);
            adaptSettingsForJob(job, settings);
            applyVariation(job, settings);

            const auto duration = juce::jmax(0.03f, job.durationSeconds.getFloatValue());
            const auto extraTail = isTextureJob(job) ? 1.10f : 0.42f;
            juce::AudioBuffer<float> buffer(2, static_cast<int>(std::ceil((duration + extraTail) * kSampleRate)));
            buffer.clear();

            renderTimeline(buffer, buildEvents(job, duration), settings);
            const auto fx = fxProfileFor(job);
            applyTransient(buffer, fx);
            applySaturator(buffer, fx);
            applyEQ(buffer, fx);
            applyCompressor(buffer, fx);
            applyStereoWidth(buffer, fx);
            if (isTextureJob(job) || slug(job.family).contains("fx") || slug(job.instrument).contains("crash"))
                applyReverb(buffer, fx);
            applyDcHighPass(buffer);
            trimAndProtect(buffer, job, fx);

            if (!writeWav(outputFile, buffer))
                throw std::runtime_error(("Failed to write WAV: " + outputFile.getFullPathName()).toStdString());

            ++rendered;
            std::cout << "[" << rendered << "/" << jobs.size() << "] " << job.instrument << " / " << job.articulation << " / " << job.finalRelativePath << "\n";
        }

        std::cout << "Rendered " << rendered << " WAV files into " << outputBase.getFullPathName() << "\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Renderer error: " << e.what() << "\n";
        return 1;
    }
}
