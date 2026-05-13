#include "DrumSynthVoice.h"
#include "DrumConstants.h"
#include "SinTable.h"

#include <algorithm>
#include <cmath>

namespace mds
{

namespace k = mds::constants;

void DrumVoice::start(const PadSettings& settingsToUse, const float noteVelocity, const double currentSampleRate)
{
    settings = settingsToUse;
    sampleRate = std::max(1.0, currentSampleRate);
    velocity = juce::jlimit(0.0f, 1.0f, noteVelocity);
    phase = 0.0f;
    subPhase = 0.0f;
    metalPhase = 0.0f;
    svfBand = 0.0f;
    svfLow = 0.0f;
    noiseLowPassState = 0.0f;
    noiseHighPassState = 0.0f;
    noiseHighPassInput = 0.0f;
    clickBandState = 0.0f;
    clickPhase = 0.0f;
    ageSamples = 0;

    attackSamples = juce::jmax(1, static_cast<int>(settings.attackSeconds * static_cast<float>(sampleRate)));
    clickLengthSamples = juce::jmax(1, static_cast<int>(k::kClickLengthSec * static_cast<float>(sampleRate)));
    clickSamples = clickLengthSamples;
    maxAgeSamples = static_cast<int>(sampleRate * std::max(k::kMinDecaySec, settings.decaySeconds * getLifeScale()));

    const auto tuneRatio = std::pow(2.0f, settings.tuneSemitones / 12.0f);
    const auto startFrequency = settings.baseFrequencyHz * tuneRatio;
    metalBaseHz = std::max(k::kMetalBaseMinHz, startFrequency / k::kMetalBaseFreqDivisor);
    const auto dropRatio = std::pow(2.0f, -settings.pitchDropSemitones / 12.0f);

    pitchCurrentHz = startFrequency;
    pitchTargetHz = startFrequency * dropRatio;
    pitchDecayCoeff = std::exp(-1.0f / (std::max(0.001f, settings.pitchDecaySeconds) * static_cast<float>(sampleRate)));
    pitchStage = 0;
    pitchMidHz = pitchTargetHz;
    pitchDecayCoeff2 = pitchDecayCoeff;
    twoStagePitch = (settings.voiceModel == PadVoiceModel::Kick && settings.pitchDropSemitones > 1.0f);
    if (twoStagePitch)
    {
        pitchMidHz = pitchTargetHz + (startFrequency - pitchTargetHz) * k::kKickPitchMidRatio;
        const float fastDecaySec = std::max(0.001f, settings.pitchDecaySeconds * k::kKickPitchFastScale);
        pitchDecayCoeff = std::exp(-1.0f / (fastDecaySec * static_cast<float>(sampleRate)));
        const float slowDecaySec = std::max(0.001f, settings.pitchDecaySeconds * k::kKickPitchSlowScale);
        pitchDecayCoeff2 = std::exp(-1.0f / (slowDecaySec * static_cast<float>(sampleRate)));
    }

    amplitude = settings.attackSeconds <= 0.0f ? 1.0f : 0.0f;
    amplitudeDecayCoeff = std::exp(-1.0f / (std::max(0.001f, settings.decaySeconds) * static_cast<float>(sampleRate)));
    noiseEnvelope = 1.0f;
    clickEnvelope = 1.0f;

    const auto noiseScale = getNoiseScale();
    const auto noiseDecaySeconds = juce::jlimit(k::kNoiseDecayMinSec,
                                                k::kNoiseDecayMaxSec,
                                                settings.decaySeconds * noiseScale * (1.0f + (1.0f - settings.noiseAmount) * 1.5f));
    noiseDecayCoeff = std::exp(-1.0f / (noiseDecaySeconds * static_cast<float>(sampleRate)));

    const auto clickDecaySeconds = juce::jlimit(k::kClickDecayMinSec,
                                                k::kClickDecayMaxSec,
                                                k::kClickDecayBaseSec + settings.clickAmount * k::kClickDecayAmountScale);
    clickDecayCoeff = std::exp(-1.0f / (clickDecaySeconds * static_cast<float>(sampleRate)));

    const auto p = juce::jlimit(-1.0f, 1.0f, settings.pan);
    float panValue = p;
    if (settings.voiceModel == PadVoiceModel::Clap)
        panValue = juce::jlimit(-1.0f, 1.0f, panValue + random.nextFloat() * 0.14f - 0.07f);
    else if (settings.voiceModel == PadVoiceModel::Crash || settings.voiceModel == PadVoiceModel::Fx)
        panValue = juce::jlimit(-1.0f, 1.0f, panValue + random.nextFloat() * 0.10f - 0.05f);
    else if (settings.voiceModel == PadVoiceModel::Hat)
        panValue = juce::jlimit(-1.0f, 1.0f, panValue + random.nextFloat() * 0.06f - 0.03f);
    panLeft = std::sqrt(0.5f * (1.0f - panValue));
    panRight = std::sqrt(0.5f * (1.0f + panValue));

    // NOTE: velocity is already applied in render() via `sample *= velocity * settings.level`,
    // so we do NOT scale clickAmount/noiseAmount by velocity here (that would double-apply it).
    // Audit Phase 5 D1: per-pad `velocityToClick` (0..1) replaces the Phase 3.1
    // hard-coded Snare/Hat curve. Universal formula:
    //     clickAmount *= (1 - velToClick) + velToClick * velocity
    // velToClick = 0   → no extra modulation (×1.0 for any vel)
    // velToClick = 1   → click linearly tracks velocity (×velocity)
    // velToClick = 0.6 → matches the previous Snare/Hat curve (×(0.4 + 0.6·vel))
    // Combined with the linear velocity factor in render(), the effective click
    // contribution scales as vel × (1 - velToClick + velToClick · vel), keeping the
    // same perceptual dynamic for percussive transients while letting other voice
    // models opt-in (default 0.6 chosen to match the previous Snare/Hat behaviour).
    {
        const float v = juce::jlimit(0.0f, 1.0f, settings.velocityToClick);
        settings.clickAmount *= (1.0f - v) + v * velocity;
    }
    settings.clickAmount = juce::jlimit(0.0f, 1.0f, settings.clickAmount);
    settings.noiseAmount = juce::jlimit(0.0f, 1.0f, settings.noiseAmount);
    if (settings.pitchDropSemitones > k::kPitchDropVelThreshold)
        settings.pitchDropSemitones = juce::jlimit(0.0f, 48.0f,
            settings.pitchDropSemitones * (k::kPitchDropVelBase + k::kPitchDropVelRange * velocity));

    settings.cutoffHz = juce::jlimit(120.0f, 18000.0f,
        settings.cutoffHz * (k::kVelCutoffBase + k::kVelCutoffRange * velocity));

    {
        const float sr = static_cast<float>(sampleRate);
        noiseLowPassAlpha = computeLowPassAlpha(
            std::min(settings.cutoffHz * k::kNoiseLPCutoffScale, sr * k::kNoiseLPMaxRatio), sampleRate);
        noiseHighPassAlpha = computeHighPassAlpha(
            std::max(k::kNoiseHPMinHz, std::min(settings.cutoffHz * k::kNoiseHPCutoffScale, k::kNoiseHPMaxHz)), sampleRate);
        clickBandAlpha = computeLowPassAlpha(
            std::min(std::max(settings.cutoffHz * k::kClickBandCutoffScale, k::kClickBandMinHz), sr * k::kClickBandMaxRatio), sampleRate);
    }

    {
        const float sr = static_cast<float>(sampleRate);
        const float cutNorm = std::clamp(settings.cutoffHz / sr, 20.0f / sr, 0.45f);
        filterF = 2.0f * mds::fastSin(cutNorm * 0.5f);
        const float Q = kVoiceFilterQ[static_cast<std::size_t>(settings.voiceModel)];
        filterQinv = 1.0f / std::max(k::kMinFilterQ, Q);
        const float maxF = -filterQinv + std::sqrt(filterQinv * filterQinv + 4.0f);
        filterF = std::min(filterF, maxF * k::kSVFStabilityMargin);
    }

    bodyBuf.fill(0.0f);
    bodyWritePos = 0;
    bodyDampState = 0.0f;
    {
        const auto config = getBodyResonatorConfig();
        bodyFeedback = std::clamp(config.feedback, 0.0f, 0.85f);
        bodyDamping = config.damping;
        if (bodyFeedback > 0.001f)
        {
            const float sr = static_cast<float>(sampleRate);
            float bodyHz = startFrequency * config.frequencyRatio;
            bodyHz = std::clamp(bodyHz, k::kBodyMinFreqHz, sr * k::kBodyMaxFreqRatio);
            bodyDelay = sr / bodyHz;
            bodyDelay = std::min(bodyDelay, static_cast<float>(kBodyBufSize - 2));
        }
        else
        {
            bodyDelay = 0.0f;
        }
    }

    holdSamples = static_cast<int>(getHoldSeconds() * static_cast<float>(sampleRate));
    const float sr = static_cast<float>(sampleRate);
    outputTrim = 0.90f;
    activeMetallicPartials = kMaxMetallicPartials;
    stereoSideAmount = 0.0f;
    stereoPhase = random.nextFloat();
    stereoPhaseInc = 0.0f;
    if (settings.voiceModel == PadVoiceModel::Kick)
        outputTrim = 0.94f;  // kick: +4% headroom (foundational element)
    else if (settings.voiceModel == PadVoiceModel::Snare)
        outputTrim = 0.90f;  // was 0.84f: +6% to normalize with kick
    else if (settings.voiceModel == PadVoiceModel::Clap)
    {
        outputTrim = 0.90f;  // was 0.82f: +8% to normalize
        stereoSideAmount = 0.16f;
        stereoPhaseInc = (28.0f + random.nextFloat() * 36.0f) / sr;
    }
    else if (settings.voiceModel == PadVoiceModel::Hat)
    {
        outputTrim = 0.90f;  // was 0.74f: +16% to normalize (major fix)
        stereoSideAmount = 0.05f;
        stereoPhaseInc = (18.0f + random.nextFloat() * 18.0f) / sr;
    }
    else if (settings.voiceModel == PadVoiceModel::PercWood || settings.voiceModel == PadVoiceModel::PercMetal)
        outputTrim = 0.90f;  // was 0.86f: +4% to normalize
    else if (settings.voiceModel == PadVoiceModel::Tom)
        outputTrim = 0.90f;  // was 0.88f: +2% to normalize
    else if (settings.voiceModel == PadVoiceModel::Crash)
    {
        outputTrim = 0.90f;  // was 0.70f: +20% to normalize (major fix)
        stereoSideAmount = 0.12f;
        stereoPhaseInc = (12.0f + random.nextFloat() * 18.0f) / sr;
    }
    else if (settings.voiceModel == PadVoiceModel::Fx)
    {
        outputTrim = 0.90f;  // was 0.78f: +12% to normalize
        stereoSideAmount = 0.09f;
        stereoPhaseInc = (16.0f + random.nextFloat() * 22.0f) / sr;
    }

    if (settings.voiceModel == PadVoiceModel::Hat || settings.voiceModel == PadVoiceModel::Crash)
    {
        const float openAmount = juce::jlimit(0.0f, 1.0f, settings.openAmount);
        const float metallicDensity = juce::jlimit(0.0f, 1.0f, settings.metallicDensity);
        settings.decaySeconds = std::max(0.01f, settings.decaySeconds
            * (settings.voiceModel == PadVoiceModel::Crash
                ? (0.82f + openAmount * 1.10f)
                : (0.45f + openAmount * 1.75f)));
        settings.cutoffHz = juce::jlimit(120.0f, 18000.0f,
            settings.cutoffHz * (0.88f + openAmount * 0.28f + metallicDensity * 0.10f));

        const bool shortDecay = settings.decaySeconds < (settings.voiceModel == PadVoiceModel::Hat ? 0.05f : 0.22f);
        const bool darkTone = settings.cutoffHz < (settings.voiceModel == PadVoiceModel::Hat ? 9000.0f : 7000.0f);
        activeMetallicPartials = settings.voiceModel == PadVoiceModel::Hat ? 10 : 12;
        if (shortDecay)
            activeMetallicPartials -= 2;
        if (darkTone)
            activeMetallicPartials -= 2;
        activeMetallicPartials += static_cast<int>(std::round((openAmount - 0.5f) * 2.0f
                                                              + (metallicDensity - 0.5f) * 3.0f));
        activeMetallicPartials = juce::jlimit(6, kMaxMetallicPartials, activeMetallicPartials);
    }

    dcBlockX1 = 0.0f;
    dcBlockY1 = 0.0f;
    active = true;
}

void DrumVoice::render(juce::AudioBuffer<float>& buffer, const int startSample, const int numSamples)
{
    if (! active)
        return;

    juce::ScopedNoDenormals noDenormals;

    const auto channelCount = buffer.getNumChannels();
    if (channelCount <= 0)
        return;

    auto* left = buffer.getWritePointer(0);
    auto* right = channelCount > 1 ? buffer.getWritePointer(1) : nullptr;
    const float sr = static_cast<float>(sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        if (! active)
            break;

        if (ageSamples < attackSamples)
            amplitude = static_cast<float>(ageSamples) / static_cast<float>(attackSamples);
        else if (ageSamples < attackSamples + holdSamples)
            amplitude = 1.0f;
        else
            amplitude *= amplitudeDecayCoeff;

        noiseEnvelope *= noiseDecayCoeff;
        clickEnvelope *= clickDecayCoeff;

        if (twoStagePitch && pitchStage == 0)
        {
            pitchCurrentHz = pitchMidHz + (pitchCurrentHz - pitchMidHz) * pitchDecayCoeff;
            if (pitchCurrentHz <= pitchMidHz * k::kKickPitchStageThresh)
            {
                pitchStage = 1;
                pitchCurrentHz = pitchMidHz;
            }
        }
        else if (twoStagePitch && pitchStage == 1)
        {
            pitchCurrentHz = pitchTargetHz + (pitchCurrentHz - pitchTargetHz) * pitchDecayCoeff2;
        }
        else
        {
            pitchCurrentHz = pitchTargetHz + (pitchCurrentHz - pitchTargetHz) * pitchDecayCoeff;
        }
        phase += pitchCurrentHz * pitchBendFactor / sr;
        if (phase >= 1.0f) phase -= 1.0f;

        subPhase += pitchCurrentHz * pitchBendFactor * 0.5f / sr;
        if (subPhase >= 1.0f) subPhase -= 1.0f;

        metalPhase += metalBaseHz / sr;
        if (metalPhase >= 1.0f) metalPhase -= 1.0f;

        float click = 0.0f;
        if (clickSamples > 0)
        {
            const auto clickProgress = 1.0f - static_cast<float>(clickSamples) / static_cast<float>(clickLengthSamples);
            const float clickBaseFreq = std::max(k::kClickBaseFreqMin, settings.cutoffHz * k::kClickCutoffRatio);
            clickPhase += (clickBaseFreq + clickBaseFreq * 2.0f * clickProgress) / sr;
            if (clickPhase >= 1.0f) clickPhase -= 1.0f;
            const auto clickCarrier = mds::fastSin(clickPhase);
            const auto clickPulse = (random.nextFloat() * 2.0f - 1.0f) * k::kClickNoiseBlend + clickCarrier * k::kClickCarrierBlend;
            clickBandState += clickBandAlpha * (clickPulse - clickBandState);
            click = clickBandState * settings.clickAmount * clickEnvelope;
            --clickSamples;
        }

        float noise = 0.0f;
        if (settings.noiseAmount > 0.001f)
        {
            const auto rawNoise = random.nextFloat() * 2.0f - 1.0f;
            noiseLowPassState += noiseLowPassAlpha * (rawNoise - noiseLowPassState);
            const auto highPassed = noiseHighPassAlpha * (noiseHighPassState + noiseLowPassState - noiseHighPassInput);
            noiseHighPassState = highPassed;
            noiseHighPassInput = noiseLowPassState;
            noise = highPassed * settings.noiseAmount * noiseEnvelope;
        }

        float body = 0.0f;
        float transientMix = 0.0f;
        renderModel(body, noise, click, transientMix);

        if (bodyFeedback > 0.001f && bodyDelay > 1.0f)
        {
            const float delayed = readBody(bodyDelay);
            bodyDampState += bodyDamping * (delayed - bodyDampState);
            if (!(bodyDampState > k::kDenormalFloor || bodyDampState < -k::kDenormalFloor)) bodyDampState = 0.0f;
            const float bodyRes = bodyDampState * bodyFeedback;
            bodyBuf[static_cast<std::size_t>(bodyWritePos)] = body + bodyRes;
            bodyWritePos = (bodyWritePos + 1) % kBodyBufSize;
            body += bodyRes * k::kBodyResBlend;
        }

        const auto tonalAndNoise = body + noise + click;

        {
            const auto hp = tonalAndNoise - svfLow - filterQinv * svfBand;
            svfBand += filterF * hp;
            svfLow  += filterF * svfBand;
            if (!(svfBand > k::kDenormalFloor || svfBand < -k::kDenormalFloor)) svfBand = 0.0f;
            if (!(svfLow  > k::kDenormalFloor || svfLow  < -k::kDenormalFloor)) svfLow  = 0.0f;
        }

        auto sample = svfLow;

        const auto outputDrive = 1.0f + (settings.drive - 1.0f) * k::kDriveScaleFactor;
        const auto driveNorm = 1.0f / std::max(0.0001f, std::tanh(outputDrive));
        sample = std::tanh(sample * outputDrive) * driveNorm;

        // DC blocker: y[n] = x[n] - x[n-1] + R * y[n-1]
        const float dcBlockOut = sample - dcBlockX1 + dcBlockR * dcBlockY1;
        dcBlockX1 = sample;
        dcBlockY1 = dcBlockOut;
        sample = dcBlockOut;

        sample *= outputTrim * velocity * settings.level;

        float stereoSide = 0.0f;
        if (right != nullptr && stereoSideAmount > 0.0001f)
        {
            stereoPhase += stereoPhaseInc;
            if (stereoPhase >= 1.0f)
                stereoPhase -= 1.0f;

            const float stereoMod = 0.65f + 0.35f * mds::fastSin(stereoPhase);
            const float sideSource = noise * 0.85f + click * 0.45f + body * 0.08f;
            stereoSide = sideSource * stereoSideAmount * stereoMod;
        }

        const auto idx = startSample + i;
        left[idx] += sample * panLeft + stereoSide;

        if (right != nullptr)
            right[idx] += sample * panRight - stereoSide;

        ++ageSamples;

        if (ageSamples > attackSamples && ((amplitude < k::kAmpDeathThreshold && noiseEnvelope < k::kNoiseDeathThreshold) || ageSamples >= maxAgeSamples))
            active = false;
    }
}

float DrumVoice::computeLowPassAlpha(const float cutoffHz, const double currentSampleRate)
{
    const auto limitedCutoff = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), cutoffHz);
    return 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * limitedCutoff / static_cast<float>(currentSampleRate));
}

float DrumVoice::computeHighPassAlpha(const float cutoffHz, const double currentSampleRate)
{
    const auto limitedCutoff = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), cutoffHz);
    const auto rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * limitedCutoff);
    const auto dt = 1.0f / static_cast<float>(currentSampleRate);
    return rc / (rc + dt);
}

float DrumVoice::readBody(const float delaySamples) const
{
    float readPos = static_cast<float>(bodyWritePos) - delaySamples;
    if (readPos < 0.0f)
        readPos += static_cast<float>(kBodyBufSize);

    const int idx0 = static_cast<int>(readPos) % kBodyBufSize;
    const float frac = readPos - std::floor(readPos);

    auto wrap = [](int i) -> int {
        return ((i % kBodyBufSize) + kBodyBufSize) % kBodyBufSize;
    };

    const float sm1 = bodyBuf[static_cast<std::size_t>(wrap(idx0 - 1))];
    const float s0  = bodyBuf[static_cast<std::size_t>(idx0)];
    const float s1  = bodyBuf[static_cast<std::size_t>(wrap(idx0 + 1))];
    const float s2  = bodyBuf[static_cast<std::size_t>(wrap(idx0 + 2))];

    const float c0 = s0;
    const float c1 = 0.5f * (s1 - sm1);
    const float c2 = sm1 - 2.5f * s0 + 2.0f * s1 - 0.5f * s2;
    const float c3 = 0.5f * (s2 - sm1) + 1.5f * (s0 - s1);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

// =========================================================================
// TonalDrumVoice
// =========================================================================
float TonalDrumVoice::getLifeScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(tonalConfig().modelIndex)].lifeScale;
}

float TonalDrumVoice::getNoiseScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(tonalConfig().modelIndex)].noiseScale;
}

float TonalDrumVoice::getHoldSeconds() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(tonalConfig().modelIndex)].holdSeconds;
}

DrumVoice::BodyResonatorConfig TonalDrumVoice::getBodyResonatorConfig() const
{
    const auto& br = kBodyResonators[static_cast<std::size_t>(tonalConfig().modelIndex)];
    return { br.feedback, br.damping, br.frequencyRatio };
}

void TonalDrumVoice::renderModel(float& body, float& noise, float& click, float& transientMix)
{
    const auto& cfg = tonalConfig();

    float pitchContour = 1.0f;
    if (cfg.pitchContourOnH2)
        pitchContour = juce::jlimit(0.0f, 1.0f,
            1.0f - static_cast<float>(ageSamples) / static_cast<float>(juce::jmax(1, maxAgeSamples / 3)));

    float signal = mds::fastSin(phase) * cfg.sineLevel;

    if (cfg.subLevel > 0.0f)
        signal += mds::fastSin(subPhase) * cfg.subLevel;

    if (cfg.harmonic2 > 0.0f)
    {
        float h2 = mds::fastSin(phase * 2.0f) * cfg.harmonic2;
        if (cfg.pitchContourOnH2) h2 *= pitchContour;
        signal += h2;
    }

    if (cfg.harmonic3 > 0.0f)
        signal += mds::fastSin(phase * 3.0f) * cfg.harmonic3;

    if (cfg.goldenRatioLevel > 0.0f)
        signal += mds::fastSin(phase * k::kGoldenRatio) * cfg.goldenRatioLevel;

    body = signal * amplitude;
    noise *= cfg.noiseModScale;
    click *= cfg.clickScale;
    transientMix = cfg.transientBase;
}

// -- Tonal leaf configs --

const TonalDrumVoice::TonalConfig& KickVoice::tonalConfig() const
{
    //                                idx  sine   sub    h2     h3     golden pCont  click  noise  trans
    static constexpr TonalConfig cfg = { 0, 0.62f, 0.34f, 0.09f, 0.02f,  0.0f,  true,  0.70f, 0.03f, 0.12f };
    return cfg;
}

const TonalDrumVoice::TonalConfig& SnareVoice::tonalConfig() const
{
    static constexpr TonalConfig cfg = { 1, 0.14f, 0.0f, 0.10f, 0.03f, 0.0f, false, 0.56f, 1.18f, 0.18f };
    return cfg;
}

const TonalDrumVoice::TonalConfig& TomVoice::tonalConfig() const
{
    static constexpr TonalConfig cfg = { 6, 0.78f, 0.06f, 0.08f, 0.03f, 0.09f, false, 0.72f, 0.10f, 0.10f };
    return cfg;
}

// =========================================================================
// MetallicDrumVoice
// =========================================================================
float MetallicDrumVoice::getLifeScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(metallicConfig().modelIndex)].lifeScale;
}

float MetallicDrumVoice::getNoiseScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(metallicConfig().modelIndex)].noiseScale;
}

void MetallicDrumVoice::renderModel(float& body, float& noise, float& click, float& transientMix)
{
    const auto& cfg = metallicConfig();
    const auto& partials = cfg.useCrashPartials ? kCrashPartials : kHatPartials;
    const float metallicDensity = juce::jlimit(0.0f, 1.0f, settings.metallicDensity);

    float metal = 0.0f;
    for (int p = 0; p < activeMetallicPartials; ++p)
    {
        const float densityWeight = 1.0f + metallicDensity * 0.22f
            * static_cast<float>(p) / static_cast<float>(juce::jmax(1, activeMetallicPartials - 1));
        metal += mds::fastSin(metalPhase * partials.ratios[p]) * partials.amplitudes[p] * densityWeight;
    }

    body = metal * (cfg.bodyBase + noiseEnvelope * (cfg.bodyModulation + metallicDensity * 0.08f))
         * (0.82f + metallicDensity * 0.34f);
    noise *= cfg.noiseModScale * (1.12f - metallicDensity * 0.34f);
    click *= cfg.clickScale * (0.86f + metallicDensity * 0.20f);
    transientMix = cfg.transientBase + metallicDensity * 0.03f;
}

// -- Metallic leaf configs --

const MetallicDrumVoice::MetallicConfig& HatVoice::metallicConfig() const
{
    //                                   idx crash  base   mod    noise  click  trans
    static constexpr MetallicConfig cfg = { 3, false, 0.16f, 0.18f, 0.72f, 0.08f, 0.10f };
    return cfg;
}

const MetallicDrumVoice::MetallicConfig& CrashVoice::metallicConfig() const
{
    static constexpr MetallicConfig cfg = { 7, true, 0.18f, 0.22f, 0.52f, 0.025f, 0.05f };
    return cfg;
}

// =========================================================================
// ClapVoice (NoiseBurst)
// =========================================================================
float ClapVoice::getLifeScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(PadVoiceModel::Clap)].lifeScale;
}

float ClapVoice::getNoiseScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(PadVoiceModel::Clap)].noiseScale;
}

void ClapVoice::renderModel(float& body, float& noise, float& click, float& transientMix)
{
    const auto burstTime = static_cast<float>(ageSamples) / static_cast<float>(sampleRate);
    const float clapSpread = juce::jlimit(0.0f, 1.0f, settings.clapSpread);
    const float clapDensity = juce::jlimit(0.0f, 1.0f, settings.clapDensity);
    const float spreadScale = 0.55f + clapSpread * 1.20f;
    const int burstCount = juce::jlimit(3, 6, 3 + static_cast<int>(std::round(clapDensity * 3.0f)));
    float burstEnv = 0.0f;
    const std::array<float, 6> burstOffsets { 0.0f, 0.012f, 0.026f, 0.040f, 0.055f, 0.072f };
    for (int burstIndex = 0; burstIndex < burstCount; ++burstIndex)
    {
        const float offset = burstOffsets[static_cast<std::size_t>(burstIndex)] * spreadScale;
        const auto dt = burstTime - offset;
        if (dt >= 0.0f)
            burstEnv += std::exp(-dt * (k::kClapBurstDecayRate * (1.05f - clapDensity * 0.20f)));
    }
    burstEnv = juce::jlimit(0.0f, k::kClapBurstMaxEnv, burstEnv);
    const auto midNoise = noiseLowPassState * k::kClapMidNoiseBlend;
    body = midNoise * burstEnv * (k::kClapBodyScale * (0.85f + clapDensity * 0.50f));
    noise = (noise * (k::kClapNoiseDirectScale * (0.78f + clapDensity * 0.34f))
           + midNoise * k::kClapNoiseMidScale) * burstEnv;
    click *= 0.04f + clapDensity * 0.05f;
    transientMix = 0.03f + clapDensity * 0.04f;
}

// =========================================================================
// ModalDrumVoice
// =========================================================================
float ModalDrumVoice::getLifeScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(modalConfig().modelIndex)].lifeScale;
}

float ModalDrumVoice::getNoiseScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(modalConfig().modelIndex)].noiseScale;
}

DrumVoice::BodyResonatorConfig ModalDrumVoice::getBodyResonatorConfig() const
{
    const auto& br = kBodyResonators[static_cast<std::size_t>(modalConfig().modelIndex)];
    return { br.feedback, br.damping, br.frequencyRatio };
}

void ModalDrumVoice::renderModel(float& body, float& noise, float& click, float& transientMix)
{
    const auto& cfg = modalConfig();
    const auto hitTime = static_cast<float>(ageSamples) / static_cast<float>(sampleRate);
    const float bodyTone = juce::jlimit(0.0f, 1.0f, settings.bodyTone);
    const float modalRing = juce::jlimit(0.0f, 1.0f, settings.modalRing);
    const float tailRate = 1.0f / std::max(0.01f, settings.decaySeconds * (0.72f + modalRing * 0.95f));
    const float knockRate = std::max(tailRate * cfg.knockRateMultiplier, cfg.minKnockRate);
    const auto knockEnv = std::exp(-hitTime * knockRate);
    const auto tailEnv = std::exp(-hitTime * tailRate);

    float signal = mds::fastSin(phase) * (cfg.sineLevel * (0.78f + bodyTone * 0.48f))
                 + mds::fastSin(phase * 2.0f) * (cfg.harmonic2 * (1.18f - bodyTone * 0.36f));

    if (cfg.hasMetalPartials)
    {
        const float metallicBias = 0.40f + bodyTone * 0.90f;
        signal += mds::fastSin(phase * k::kGoldenRatio) * k::kModalGoldenPartialAmp * metallicBias
                + mds::fastSin(phase * k::kSqrt6Approx) * k::kModalSqrt6PartialAmp * metallicBias;
    }

    const auto impact = noiseLowPassState * knockEnv * cfg.impactLevel * (1.18f - bodyTone * 0.38f);
    body = signal * tailEnv * (0.80f + bodyTone * 0.42f) + impact;
    noise = noise * cfg.noiseKnockScale * knockEnv * (1.05f - bodyTone * 0.40f);
    click *= cfg.clickKnockScale * knockEnv * (0.88f + bodyTone * 0.22f);
    transientMix = cfg.transientBase + modalRing * 0.03f;
}

// -- Modal leaf configs --

const ModalDrumVoice::ModalConfig& PercWoodVoice::modalConfig() const
{
    //                                idx  sine   h2     knockM minK   impact metal  noiseK clickK trans
    static constexpr ModalConfig cfg = { 4, 0.28f, 0.055f, 6.0f, 80.0f, 0.58f, false, 0.22f, 0.07f, 0.03f };
    return cfg;
}

const ModalDrumVoice::ModalConfig& PercMetalVoice::modalConfig() const
{
    static constexpr ModalConfig cfg = { 5, 0.28f, 0.040f, 6.0f, 100.0f, 0.56f, true, 0.20f, 0.06f, 0.03f };
    return cfg;
}

// =========================================================================
// FxVoice (FM)
// =========================================================================
float FxVoice::getLifeScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(PadVoiceModel::Fx)].lifeScale;
}

float FxVoice::getNoiseScale() const
{
    return kVoiceEnvelopes[static_cast<std::size_t>(PadVoiceModel::Fx)].noiseScale;
}

void FxVoice::renderModel(float& body, float& noise, float& click, float& transientMix)
{
    const float fmIndex = juce::jlimit(0.0f, 1.0f, settings.fmIndex);
    const float fmSweep = juce::jlimit(0.0f, 1.0f, settings.fmSweep);
    const float modFreq = 0.55f + fmSweep * 2.10f;
    const float modDepth = k::kFmBaseModDepth * (0.35f + fmIndex * 1.85f)
                         + noiseEnvelope * k::kFmModEnvScale * (0.30f + fmSweep * 1.40f);
    const auto sweep = mds::fastSin(phase * (1.0f + fmSweep * 0.55f)
                                    + mds::fastSin(phase * modFreq) * modDepth);
    body = sweep * amplitude * (k::kFmOutputScale * (0.72f + fmIndex * 0.42f));
    noise *= 0.03f + (1.0f - fmSweep) * 0.05f;
    click *= 0.16f + fmSweep * 0.24f;
    transientMix = 0.06f + fmIndex * 0.08f;
}

// =========================================================================
// VoicePool
// =========================================================================
VoicePool::VoicePool()
{
    for (int m = 0; m < kNumVoiceModels; ++m)
        for (int s = 0; s < kMaxVoicesPerModel; ++s)
            bank[static_cast<std::size_t>(m)][static_cast<std::size_t>(s)].voice =
                createVoiceForModel(static_cast<PadVoiceModel>(m));
}

DrumVoice* VoicePool::acquire(const PadVoiceModel model)
{
    auto& slots = bank[static_cast<std::size_t>(model)];
    for (auto& slot : slots)
    {
        if (! slot.inUse)
        {
            slot.inUse = true;
            return slot.voice.get();
        }
    }
    return nullptr;
}

void VoicePool::release(const PadVoiceModel model, DrumVoice* voice) noexcept
{
    if (voice == nullptr)
        return;
    auto& slots = bank[static_cast<std::size_t>(model)];
    for (auto& slot : slots)
    {
        if (slot.voice.get() == voice)
        {
            slot.inUse = false;
            return;
        }
    }
}

// =========================================================================
// Voice factory (used by renderer and pool constructor)
// =========================================================================
std::unique_ptr<DrumVoice> createVoiceForModel(const PadVoiceModel voiceModel)
{
    switch (voiceModel)
    {
    case PadVoiceModel::Kick:      return std::make_unique<KickVoice>();
    case PadVoiceModel::Snare:     return std::make_unique<SnareVoice>();
    case PadVoiceModel::Clap:      return std::make_unique<ClapVoice>();
    case PadVoiceModel::Hat:       return std::make_unique<HatVoice>();
    case PadVoiceModel::PercWood:  return std::make_unique<PercWoodVoice>();
    case PadVoiceModel::PercMetal: return std::make_unique<PercMetalVoice>();
    case PadVoiceModel::Tom:       return std::make_unique<TomVoice>();
    case PadVoiceModel::Crash:     return std::make_unique<CrashVoice>();
    case PadVoiceModel::Fx:        return std::make_unique<FxVoice>();
    }

    return std::make_unique<PercWoodVoice>();
}

std::unique_ptr<DrumVoice> createVoiceForPad(const int padIndex)
{
    return createVoiceForModel(getPadVoiceModel(padIndex));
}
} // namespace mds
