#include <JuceHeader.h>

#include "../PluginProcessor.h"
#include "../PluginEditor.h"
#include "../Engine/FactoryPresets.h"
#include "../Engine/DrumSynthVoice.h"
#include "../../../Shared/ProductionQa.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
void require(bool condition, const juce::String& message)
{
    if (!condition)
        throw std::runtime_error(message.toStdString());
}

void setParameterValue(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, float actualValue)
{
    auto* parameter = apvts.getParameter(paramId);
    require(parameter != nullptr, "Missing parameter: " + paramId);

    auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
    require(ranged != nullptr, "Parameter is not ranged: " + paramId);
    parameter->setValueNotifyingHost(ranged->convertTo0to1(actualValue));
}

void setStateValue(juce::ValueTree& state, const juce::String& paramId, const juce::var& value)
{
    for (int childIndex = 0; childIndex < state.getNumChildren(); ++childIndex)
    {
        auto child = state.getChild(childIndex);
        if (child.getProperty("id").toString() == paramId)
        {
            child.setProperty("value", value, nullptr);
            return;
        }
    }

    throw std::runtime_error(("Missing state node for parameter: " + paramId).toStdString());
}

juce::AudioBuffer<float> renderWithMidi(DrumSynthAudioProcessor& processor,
                                        const std::vector<std::pair<int, juce::MidiMessage>>& events,
                                        int totalSamples,
                                        int blockSize = 256)
{
    const int totalChannels = processor.getTotalNumOutputChannels();
    juce::AudioBuffer<float> rendered(juce::jmax(2, totalChannels), totalSamples);
    rendered.clear();
    juce::AudioBuffer<float> block(totalChannels, blockSize);

    std::size_t eventIndex = 0;
    for (int blockStart = 0; blockStart < totalSamples; blockStart += blockSize)
    {
        block.clear();
        juce::MidiBuffer midi;
        while (eventIndex < events.size() && events[eventIndex].first < blockStart + blockSize)
        {
            if (events[eventIndex].first >= blockStart)
                midi.addEvent(events[eventIndex].second, events[eventIndex].first - blockStart);
            ++eventIndex;
        }

        processor.processBlock(block, midi);

        const int samplesToCopy = juce::jmin(blockSize, totalSamples - blockStart);
        for (int channel = 0; channel < juce::jmin(rendered.getNumChannels(), block.getNumChannels()); ++channel)
            rendered.copyFrom(channel, blockStart, block, channel, 0, samplesToCopy);
    }

    return rendered;
}

juce::File manifestFileFor(const juce::File& presetFile)
{
    return presetFile.getSiblingFile(presetFile.getFileNameWithoutExtension() + ".preset.json");
}

void cleanupPresetArtifacts(const juce::File& presetFile)
{
    manifestFileFor(presetFile).deleteFile();
    presetFile.deleteFile();
}

bool buffersNearlyEqual(const juce::AudioBuffer<float>& lhs,
                        const juce::AudioBuffer<float>& rhs,
                        float epsilon = 1.0e-6f)
{
    if (lhs.getNumChannels() != rhs.getNumChannels() || lhs.getNumSamples() != rhs.getNumSamples())
        return false;

    for (int channel = 0; channel < lhs.getNumChannels(); ++channel)
    {
        const auto* left = lhs.getReadPointer(channel);
        const auto* right = rhs.getReadPointer(channel);
        for (int sample = 0; sample < lhs.getNumSamples(); ++sample)
        {
            if (std::abs(left[sample] - right[sample]) > epsilon)
                return false;
        }
    }

    return true;
}

template <typename ComponentType, typename Predicate>
ComponentType* findComponentRecursive(juce::Component& root, Predicate&& predicate)
{
    for (int childIndex = 0; childIndex < root.getNumChildComponents(); ++childIndex)
    {
        auto* child = root.getChildComponent(childIndex);
        if (auto* typed = dynamic_cast<ComponentType*>(child); typed != nullptr && predicate(*typed))
            return typed;
        if (auto* nested = findComponentRecursive<ComponentType>(*child, predicate); nested != nullptr)
            return nested;
    }

    return nullptr;
}

float busMagnitude(const DrumSynthAudioProcessor& processor,
                   const juce::AudioBuffer<float>& buffer,
                   const int busIndex)
{
    const int busChannels = processor.getChannelCountOfBus(false, busIndex);
    if (busChannels <= 0)
        return 0.0f;

    float magnitude = 0.0f;
    for (int channel = 0; channel < busChannels; ++channel)
    {
        const int absoluteChannel = processor.getChannelIndexInProcessBlockBuffer(false, busIndex, channel);
        if (absoluteChannel >= 0 && absoluteChannel < buffer.getNumChannels())
            magnitude = juce::jmax(magnitude, buffer.getMagnitude(absoluteChannel, 0, buffer.getNumSamples()));
    }
    return magnitude;
}

void testFactoryBankShape()
{
    const auto& presets = mds::getFactoryPresets();
    require(!presets.empty(), "Drum factory preset bank cannot be empty");

    for (const auto& preset : presets)
    {
        require(!preset.name.empty(), "Drum preset name cannot be empty");
        for (int pad = 0; pad < mds::kNumPads; ++pad)
        {
            const auto& settings = preset.pads[static_cast<std::size_t>(pad)];
            require(settings.level >= 0.0f && settings.level <= 1.2f, "Pad level must stay in production range");
            require(preset.outputBuses[static_cast<std::size_t>(pad)] >= 0
                && preset.outputBuses[static_cast<std::size_t>(pad)] <= DrumSynthAudioProcessor::kNumAuxOutputs,
                "Pad output bus out of range");
        }
    }
}

void testBootPresetMatchesFactory()
{
    DrumSynthAudioProcessor processor;
    const auto& bootPreset = mds::getFactoryPresets().front();

    require(processor.getCurrentFactoryPresetIndex() == 0, "Boot preset index must default to 0");
    require(std::abs(processor.getAPVTS().getRawParameterValue(DrumSynthAudioProcessor::makePadParamId(0, "level"))->load()
        - bootPreset.pads[0].level) < 1.0e-4f, "Boot pad level must match first factory preset");
    require(std::abs(processor.getAPVTS().getRawParameterValue(DrumSynthAudioProcessor::makePadParamId(0, "output"))->load()
        - static_cast<float>(bootPreset.outputBuses[0])) < 1.0e-4f, "Boot pad output must match first factory preset");
}

void testPresetStoragePaths()
{
    const auto userPresetDir = DrumSynthAudioProcessor::getUserPresetsDirectory();
    const auto factoryOverrideDir = DrumSynthAudioProcessor::getFactoryOverridesDirectory();
    require(userPresetDir.getFullPathName().containsIgnoreCase("MusiqueDrumSynth"), "Unexpected drum app-data root");
    require(userPresetDir.getFullPathName().containsIgnoreCase("Presets"), "User presets must live in the preset library");
    require(factoryOverrideDir.getFullPathName().containsIgnoreCase("FactoryOverrides_StudioKit"), "Factory overrides must use FactoryOverrides_StudioKit");
}

void testStateSanitization()
{
    DrumSynthAudioProcessor processor;
    auto state = processor.getAPVTS().copyState();

    setStateValue(state, "selected_pad", 99);
    setStateValue(state, "delay_note_div", 99);
    setStateValue(state, DrumSynthAudioProcessor::makePadParamId(0, "level"), -5.0);
    setStateValue(state, DrumSynthAudioProcessor::makePadParamId(0, "output"), 99);

    auto xml = state.createXml();
    require(xml != nullptr, "Failed to snapshot drum APVTS XML");

    juce::MemoryBlock block;
    juce::AudioProcessor::copyXmlToBinary(*xml, block);
    processor.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    const auto selectedPad = processor.getAPVTS().getRawParameterValue("selected_pad")->load();
    const auto delayNoteDiv = processor.getAPVTS().getRawParameterValue("delay_note_div")->load();
    const auto level = processor.getAPVTS().getRawParameterValue(DrumSynthAudioProcessor::makePadParamId(0, "level"))->load();
    const auto outputBus = processor.getAPVTS().getRawParameterValue(DrumSynthAudioProcessor::makePadParamId(0, "output"))->load();

    require(selectedPad >= 0.0f && selectedPad < static_cast<float>(mds::kNumPads), "selected_pad must be clamped");
    require(delayNoteDiv >= 0.0f && delayNoteDiv <= 4.0f, "delay_note_div must be clamped");
    require(level >= 0.0f && level <= 1.0f, "Pad level must be clamped");
    require(outputBus >= 0.0f && outputBus <= static_cast<float>(DrumSynthAudioProcessor::kNumAuxOutputs), "Pad output bus must be clamped");
}

void testUserPresetRoundTrip()
{
    DrumSynthAudioProcessor processor;
    const int presetIndex = juce::jmin(1, static_cast<int>(mds::getFactoryPresets().size()) - 1);
    processor.applyFactoryPreset(presetIndex);
    const auto savedLevel = processor.getAPVTS().getRawParameterValue(DrumSynthAudioProcessor::makePadParamId(0, "level"))->load();

    const auto presetName = "codex_drum_roundtrip_" + juce::String(juce::Time::getCurrentTime().toMilliseconds());
    const auto file = DrumSynthAudioProcessor::getUserPresetsDirectory()
        .getChildFile(juce::File::createLegalFileName(presetName) + ".xml");
    cleanupPresetArtifacts(file);
    require(processor.saveUserPreset(presetName), "Failed to save drum user preset via product API");
    require(file.existsAsFile(), "Saved drum preset file must exist");

    setParameterValue(processor.getAPVTS(), DrumSynthAudioProcessor::makePadParamId(0, "level"), 0.15f);
    require(processor.loadUserPreset(file), "Failed to reload saved drum preset");

    const auto reloadedLevel = processor.getAPVTS().getRawParameterValue(DrumSynthAudioProcessor::makePadParamId(0, "level"))->load();
    require(std::abs(reloadedLevel - savedLevel) < 1.0e-4f, "Reloaded drum preset must restore saved level");
    cleanupPresetArtifacts(file);
}

void testStateBinaryRoundTrip()
{
    DrumSynthAudioProcessor source;
    const int presetIndex = juce::jmin(1, static_cast<int>(mds::getFactoryPresets().size()) - 1);
    source.applyFactoryPreset(presetIndex);
    setParameterValue(source.getAPVTS(), "selected_pad", 4.0f);
    setParameterValue(source.getAPVTS(), "output_gain", 3.5f);
    setParameterValue(source.getAPVTS(), DrumSynthAudioProcessor::makePadParamId(1, "level"), 0.63f);

    juce::MemoryBlock block;
    source.getStateInformation(block);

    DrumSynthAudioProcessor restored;
    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    require(restored.getCurrentFactoryPresetIndex() == presetIndex, "State round-trip must restore factory preset index");
    require(std::abs(restored.getAPVTS().getRawParameterValue("selected_pad")->load() - 4.0f) < 1.0e-4f,
            "State round-trip must restore selected_pad");
    require(std::abs(restored.getAPVTS().getRawParameterValue("output_gain")->load() - 3.5f) < 1.0e-4f,
            "State round-trip must restore output_gain");
    require(std::abs(restored.getAPVTS().getRawParameterValue(DrumSynthAudioProcessor::makePadParamId(1, "level"))->load() - 0.63f) < 1.0e-4f,
            "State round-trip must restore pad level");
}

void testQualityModeAndDelaySyncStateRoundTrip()
{
    DrumSynthAudioProcessor source;
    setParameterValue(source.getAPVTS(), "quality_mode", 1.0f);
    setParameterValue(source.getAPVTS(), "delay_sync", 1.0f);
    setParameterValue(source.getAPVTS(), "delay_note_div", 4.0f);
    setParameterValue(source.getAPVTS(), "humanize_timing", 12.5f);
    setParameterValue(source.getAPVTS(), "humanize_level", 0.12f);

    juce::MemoryBlock block;
    source.getStateInformation(block);

    DrumSynthAudioProcessor restored;
    restored.setStateInformation(block.getData(), static_cast<int>(block.getSize()));

    require(restored.getQualityMode() == DrumSynthAudioProcessor::QualityMode::Studio,
            "quality_mode must persist through state round-trip");
    require(std::abs(restored.getAPVTS().getRawParameterValue("delay_sync")->load() - 1.0f) < 1.0e-4f,
            "delay_sync must persist through state round-trip");
    require(std::abs(restored.getAPVTS().getRawParameterValue("delay_note_div")->load() - 4.0f) < 1.0e-4f,
            "delay_note_div must persist through state round-trip");
        require(std::abs(restored.getAPVTS().getRawParameterValue("humanize_timing")->load() - 12.5f) < 1.0e-4f,
            "humanize_timing must persist through state round-trip");
        require(std::abs(restored.getAPVTS().getRawParameterValue("humanize_level")->load() - 0.12f) < 1.0e-4f,
            "humanize_level must persist through state round-trip");
}

void testMidiLearnRoundTripAndCcNeutrality()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 128;

    DrumSynthAudioProcessor source;
    source.prepareToPlay(sampleRate, blockSize);

    const auto initialMacroPunch = source.getAPVTS().getRawParameterValue("macro_punch")->load();
    renderWithMidi(source, { { 0, juce::MidiMessage::controllerEvent(1, 1, 127) } }, blockSize, blockSize);
    require(std::abs(source.getAPVTS().getRawParameterValue("macro_punch")->load() - initialMacroPunch) < 1.0e-5f,
            "CC1 must stay neutral when no MIDI Learn mapping exists");

    source.startMidiLearn("output_gain");
    renderWithMidi(source, { { 0, juce::MidiMessage::controllerEvent(1, 74, 96) } }, blockSize, blockSize);
    require(source.getMidiCcForParam("output_gain") == 74,
            "MIDI Learn must capture CC74 for output_gain");

    juce::MemoryBlock state;
    source.getStateInformation(state);

    DrumSynthAudioProcessor restored;
    restored.prepareToPlay(sampleRate, blockSize);
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    require(restored.getMidiCcForParam("output_gain") == 74,
            "MIDI Learn mapping must persist in plugin state");

    setParameterValue(restored.getAPVTS(), "output_gain", -12.0f);
    renderWithMidi(restored, { { 0, juce::MidiMessage::controllerEvent(1, 74, 127) } }, blockSize, blockSize);
    require(restored.getAPVTS().getRawParameterValue("output_gain")->load() > 5.5f,
            "Restored MIDI Learn mapping must drive the learned parameter");
}

void testUserPresetManifestGenerationAndBackfill()
{
    DrumSynthAudioProcessor processor;
    const auto uniqueId = juce::String(juce::Time::getCurrentTime().toMilliseconds());
    const auto presetName = "codex_drum_manifest_" + uniqueId;
    const auto presetFile = DrumSynthAudioProcessor::getUserPresetsDirectory()
        .getChildFile(juce::File::createLegalFileName(presetName) + ".xml");
    const auto sidecar = manifestFileFor(presetFile);
    cleanupPresetArtifacts(presetFile);

    require(processor.saveUserPreset(presetName), "saveUserPreset must succeed for manifest QA");
    require(presetFile.existsAsFile(), "User preset XML must be created");
    require(sidecar.existsAsFile(), "User preset manifest sidecar must be created");

    const auto entries = processor.scanPresetLibrary();
    bool foundSavedPreset = false;
    for (const auto& entry : entries)
    {
        if (entry.presetFile == presetFile)
        {
            foundSavedPreset = true;
            require(entry.manifestFile == sidecar, "Preset library entry must point to the generated manifest");
            require(entry.familyLabel.isNotEmpty(), "Preset library entry must expose a family label");
            require(entry.mixRole.isNotEmpty(), "Preset library entry must expose a mix role");
            break;
        }
    }
    require(foundSavedPreset, "Saved preset must be discoverable in the preset library");

    cleanupPresetArtifacts(presetFile);

    const auto legacyName = "codex_drum_legacy_" + uniqueId;
    const auto legacyFile = DrumSynthAudioProcessor::getUserPresetsDirectory()
        .getChildFile(juce::File::createLegalFileName(legacyName) + ".xml");
    const auto legacyManifest = manifestFileFor(legacyFile);
    cleanupPresetArtifacts(legacyFile);

    processor.applyFactoryPreset(0);
    auto legacyState = processor.getAPVTS().copyState();
    legacyState.setProperty("user_preset_name", legacyName, nullptr);
    legacyState.setProperty("user_preset_file", legacyFile.getFullPathName(), nullptr);
    legacyState.setProperty("factory_preset_index", 0, nullptr);
    legacyState.setProperty("preset_version", 2, nullptr);
    legacyState.removeProperty("preset_family_label", nullptr);
    legacyState.removeProperty("preset_mix_role", nullptr);
    legacyState.removeProperty("preset_description", nullptr);
    legacyState.removeProperty("preset_output_profile", nullptr);
    legacyState.removeProperty("preset_nominal_peak_db", nullptr);
    legacyState.removeProperty("preset_tags", nullptr);
    auto xml = legacyState.createXml();
    require(xml != nullptr && xml->writeTo(legacyFile), "Legacy preset XML must be written for backfill QA");
    require(!legacyManifest.existsAsFile(), "Legacy manifest must not exist before scan");

    DrumSynthAudioProcessor backfilledProcessor;
    bool foundLegacyPreset = false;
    for (const auto& entry : backfilledProcessor.scanPresetLibrary())
    {
        if (entry.presetFile == legacyFile)
        {
            foundLegacyPreset = true;
            require(legacyManifest.existsAsFile(), "Processor startup maintenance must backfill a manifest sidecar");
            require(entry.familyLabel.isNotEmpty(), "Legacy preset backfill must infer family metadata");
            require(entry.mixRole.isNotEmpty(), "Legacy preset backfill must infer mix role metadata");
            break;
        }
    }
    require(foundLegacyPreset, "Legacy preset must be discoverable after scan");

    cleanupPresetArtifacts(legacyFile);
}

void testTailLengthTracksDelayAndReverb()
{
    DrumSynthAudioProcessor processor;
    const auto dryTail = processor.getTailLengthSeconds();

    setParameterValue(processor.getAPVTS(), "fx_delay_en", 1.0f);
    setParameterValue(processor.getAPVTS(), "delay_mix", 0.6f);
    setParameterValue(processor.getAPVTS(), "delay_time", 780.0f);
    setParameterValue(processor.getAPVTS(), "delay_feedback", 0.82f);
    setParameterValue(processor.getAPVTS(), "reverb_mix", 0.45f);
    setParameterValue(processor.getAPVTS(), "reverb_size", 0.92f);
    setParameterValue(processor.getAPVTS(), "reverb_predelay", 48.0f);

    const auto wetTail = processor.getTailLengthSeconds();
    require(wetTail > dryTail + 2.0, "Dynamic tail length must expand when delay/reverb are active");
    require(wetTail <= 30.0, "Dynamic tail length must remain capped at 30 seconds");
}

void testSampleRateAndBlockSizeMatrix()
{
    const std::vector<double> sampleRates = { 44100.0, 48000.0, 96000.0 };
    const std::vector<int> blockSizes = { 64, 256, 1024 };

    for (const auto sampleRate : sampleRates)
    {
        for (const auto blockSize : blockSizes)
        {
            DrumSynthAudioProcessor processor;
            processor.prepareToPlay(sampleRate, blockSize);

            const std::vector<std::pair<int, juce::MidiMessage>> events = {
                { 0, juce::MidiMessage::noteOn(1, 36, static_cast<juce::uint8>(112)) },
                { blockSize / 2, juce::MidiMessage::controllerEvent(1, 74, 96) },
                { blockSize, juce::MidiMessage::noteOn(1, 42, static_cast<juce::uint8>(90)) },
                { blockSize * 2, juce::MidiMessage::noteOn(1, 38, static_cast<juce::uint8>(100)) }
            };

            const auto rendered = renderWithMidi(processor, events, blockSize * 6, blockSize);
            require(musique::qa::bufferIsFinite(rendered),
                    "Processor must remain finite across sample-rate/block-size matrix");
        }
    }
}

void testOfflineRenderIsDeterministic()
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int totalSamples = 4096;

    const std::vector<std::pair<int, juce::MidiMessage>> events = {
        { 0, juce::MidiMessage::noteOn(1, 36, static_cast<juce::uint8>(112)) },
        { 128, juce::MidiMessage::noteOn(1, 42, static_cast<juce::uint8>(82)) },
        { 512, juce::MidiMessage::noteOn(1, 38, static_cast<juce::uint8>(96)) },
        { 1024, juce::MidiMessage::controllerEvent(1, 74, 64) },
        { 1536, juce::MidiMessage::noteOn(1, 46, static_cast<juce::uint8>(78)) }
    };

    DrumSynthAudioProcessor first;
    first.prepareToPlay(sampleRate, blockSize);
    const auto renderA = renderWithMidi(first, events, totalSamples, blockSize);

    DrumSynthAudioProcessor second;
    second.prepareToPlay(sampleRate, blockSize);
    const auto renderB = renderWithMidi(second, events, totalSamples, blockSize);

    require(buffersNearlyEqual(renderA, renderB),
            "Offline render must be deterministic for an identical MIDI clip");
}

void testAllFactoryPresetsRenderStable()
{
    constexpr double sampleRate = 48000.0;
    constexpr int renderSamples = 8192;

    for (int presetIndex = 0; presetIndex < static_cast<int>(mds::getFactoryPresets().size()); ++presetIndex)
    {
        const auto& preset = mds::getFactoryPresets()[static_cast<std::size_t>(presetIndex)];
        bool presetHasAudiblePad = false;

        for (int pad = 0; pad < mds::kNumPads; ++pad)
        {
            const auto& settings = preset.pads[static_cast<std::size_t>(pad)];
            auto voice = mds::createVoiceForModel(settings.voiceModel);
            require(voice != nullptr, "Failed to create drum voice for QA render");

            juce::AudioBuffer<float> rendered(2, renderSamples);
            rendered.clear();

            voice->start(settings, 110.0f / 127.0f, sampleRate);
            voice->render(rendered, 0, renderSamples);

            const auto peak = musique::qa::bufferPeak(rendered);
            require(musique::qa::bufferIsFinite(rendered),
                    "Factory preset render must stay finite");
            require(peak >= musique::qa::minimumAudiblePeakLinear(),
                    "Factory preset pad render must not be silent");
            require(peak <= musique::qa::maximumSafePeakLinear(),
                    "Factory preset pad render must not clip past QA ceiling");
            presetHasAudiblePad = true;
        }

        require(presetHasAudiblePad, "Factory preset must expose at least one audible pad");
    }
}

void testChokeGroupEnforcement()
{
    // Structural check: choke group assignments defined in DrumDefs.h
    require(mds::kPadCharacteristics[4].chokeGroup == 1, "Hat Closed must be in choke group 1");
    require(mds::kPadCharacteristics[5].chokeGroup == 1, "Hat Open must be in choke group 1");
    require(mds::kPadCharacteristics[10].chokeGroup == 2, "Crash must be in choke group 2 (self-choke on retrigger)");
    require(mds::kPadCharacteristics[0].chokeGroup == 0, "Kick A must have no choke group");
    require(mds::kPadCharacteristics[1].chokeGroup == 0, "Kick B must have no choke group");

    // Functional check: rapidly alternating same-choke-group triggers must render cleanly
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;

    DrumSynthAudioProcessor proc;
    proc.prepareToPlay(sampleRate, blockSize);
    juce::AudioBuffer<float> buf(2, blockSize);
    juce::MidiBuffer emptyMidi;

    for (int cycle = 0; cycle < 6; ++cycle)
    {
        proc.queuePadTrigger(5, 0.9f);   // Hat Open  (chokeGroup = 1)
        buf.clear();
        proc.processBlock(buf, emptyMidi);
        require(musique::qa::bufferIsFinite(buf),
                "Buffer not finite after Hat Open trigger (cycle " + juce::String(cycle) + ")");

        proc.queuePadTrigger(4, 0.9f);   // Hat Closed (chokeGroup = 1) — kills Hat Open
        buf.clear();
        proc.processBlock(buf, emptyMidi);
        require(musique::qa::bufferIsFinite(buf),
                "Buffer not finite after Hat Closed choke (cycle " + juce::String(cycle) + ")");
    }

}

void testVoicePoolStressRendersCleanly()
{
    constexpr double sampleRate  = 48000.0;
    constexpr int    blockSize   = 512;
    constexpr int    stressCycles = 10;

    DrumSynthAudioProcessor proc;
    proc.prepareToPlay(sampleRate, blockSize);
    juce::AudioBuffer<float> buf(2, blockSize);
    juce::MidiBuffer emptyMidi;

    for (int cycle = 0; cycle < stressCycles; ++cycle)
    {
        // Flood all 12 pads simultaneously — exercises voice pool stealing and kMaxActiveVoices cap
        for (int pad = 0; pad < mds::kNumPads; ++pad)
            proc.queuePadTrigger(pad, 0.8f);

        buf.clear();
        proc.processBlock(buf, emptyMidi);

        require(musique::qa::bufferIsFinite(buf),
                "Stress cycle " + juce::String(cycle) + " produced non-finite samples");
        require(musique::qa::bufferPeak(buf) <= musique::qa::maximumSafePeakLinear(),
                "Stress cycle " + juce::String(cycle) + " exceeded safe peak ceiling");
    }
}

void testAllVoiceModelsRenderStable()
{
    // Verify each of the 9 PadVoiceModels renders audio without NaN/inf.
    // Uses one representative pad per model (matches PadVoiceModel enum order).
    constexpr double sampleRate    = 48000.0;
    constexpr int    renderSamples = 4096;

    constexpr std::array<int, mds::kNumVoiceModels> kModelPad = { 0, 2, 3, 4, 6, 7, 8, 10, 11 };

    for (int modelIdx = 0; modelIdx < mds::kNumVoiceModels; ++modelIdx)
    {
        const int padIndex = kModelPad[static_cast<std::size_t>(modelIdx)];
        const auto model   = static_cast<mds::PadVoiceModel>(modelIdx);

        auto voice = mds::createVoiceForModel(model);
        require(voice != nullptr,
                "createVoiceForModel returned null for model " + juce::String(modelIdx));

        mds::PadSettings settings = mds::getDefaultPadSettings(padIndex);
        settings.voiceModel = model;

        juce::AudioBuffer<float> rendered(2, renderSamples);
        rendered.clear();

        voice->start(settings, 0.8f, sampleRate);
        voice->render(rendered, 0, renderSamples);

        require(musique::qa::bufferIsFinite(rendered),
                "Voice model " + juce::String(modelIdx) + " produced non-finite samples");
        require(musique::qa::bufferPeak(rendered) >= musique::qa::minimumAudiblePeakLinear(),
                "Voice model " + juce::String(modelIdx) + " produced no audible output");
    }
}

void testMainAndAuxRouting()
{
    constexpr double sampleRate = 48000.0;
    constexpr int    blockSize  = 512;

    DrumSynthAudioProcessor masterProcessor;
    masterProcessor.prepareToPlay(sampleRate, blockSize);
    setParameterValue(masterProcessor.getAPVTS(),
                      DrumSynthAudioProcessor::makePadParamId(0, "output"),
                      0.0f);
    juce::AudioBuffer<float> masterBuffer(masterProcessor.getTotalNumOutputChannels(), blockSize);
    masterBuffer.clear();
    juce::MidiBuffer masterMidi;
    masterProcessor.queuePadTrigger(0, 0.9f);
    masterProcessor.processBlock(masterBuffer, masterMidi);
    require(musique::qa::bufferIsFinite(masterBuffer),
            "Master-routed drum trigger must stay finite");
    const auto masterPeak = musique::qa::bufferPeak(masterBuffer);
    require(masterPeak > musique::qa::minimumAudiblePeakLinear(),
            "Master-routed drum trigger must hit the master bus (peak="
                + juce::String(masterPeak, 8) + ")");
    require(busMagnitude(masterProcessor, masterBuffer, 1) < 1.0e-5f,
            "Master-routed drum trigger must keep aux bus 1 silent");

    DrumSynthAudioProcessor auxProcessor;
    auxProcessor.enableAllBuses();
    auxProcessor.prepareToPlay(sampleRate, blockSize);
    setParameterValue(auxProcessor.getAPVTS(),
                      DrumSynthAudioProcessor::makePadParamId(0, "output"),
                      2.0f);
    juce::AudioBuffer<float> auxBuffer(auxProcessor.getTotalNumOutputChannels(), blockSize);
    auxBuffer.clear();
    juce::MidiBuffer auxMidi;
    auxProcessor.queuePadTrigger(0, 0.9f);
    auxProcessor.processBlock(auxBuffer, auxMidi);
    require(musique::qa::bufferIsFinite(auxBuffer),
            "Aux-routed drum trigger must stay finite");
    require(busMagnitude(auxProcessor, auxBuffer, 0) < 1.0e-5f,
            "Aux-routed drum trigger must leave the master bus dry");
    require(busMagnitude(auxProcessor, auxBuffer, 2) > musique::qa::minimumAudiblePeakLinear(),
            "Aux-routed drum trigger must hit the requested aux bus");
}

void testAuxRoutingFallsBackToMainWhenBusUnavailable()
{
    DrumSynthAudioProcessor processor;
    require(processor.getBusCount(false) > 4,
            "Drum processor must expose disabled aux buses for fallback coverage");
    require(processor.getChannelCountOfBus(false, 4) == 0,
            "Drum aux bus 4 must be unavailable in the default layout for fallback coverage");

    processor.prepareToPlay(48000.0, 256);
    setParameterValue(processor.getAPVTS(),
                      DrumSynthAudioProcessor::makePadParamId(0, "output"),
                      4.0f);

    juce::AudioBuffer<float> buffer(processor.getTotalNumOutputChannels(), 256);
    buffer.clear();
    juce::MidiBuffer midi;
    processor.queuePadTrigger(0, 0.85f);
    processor.processBlock(buffer, midi);

    require(musique::qa::bufferIsFinite(buffer),
            "Routing to an unavailable drum aux bus must stay finite");
    require(musique::qa::bufferPeak(buffer) > musique::qa::minimumAudiblePeakLinear(),
            "Routing to an unavailable drum aux bus must fall back to the master output");
}

void testPadParamBoundsRenderClean()
{
    // Verify that extreme pad parameter values do not produce NaN/inf.
    // Uses only parameter IDs confirmed to exist in the test suite.
    constexpr double sampleRate    = 48000.0;
    constexpr int    renderSamples = 2048;

    struct TestCase { int pad; float level; };
    const std::vector<TestCase> cases = {
        { 0,  0.0f },   // Kick A silent — must not crash
        { 0,  1.2f },   // Kick A max level
        { 2,  1.2f },   // Snare max level
        { 4,  0.0f },   // Hat Closed silent
        { 4,  1.2f },   // Hat Closed max level
        { 10, 1.2f },   // Crash max level
        { 11, 1.2f },   // FX max level
    };

    for (const auto& tc : cases)
    {
        DrumSynthAudioProcessor proc;
        proc.prepareToPlay(sampleRate, 512);
        setParameterValue(proc.getAPVTS(),
                          DrumSynthAudioProcessor::makePadParamId(tc.pad, "level"), tc.level);
        proc.queuePadTrigger(tc.pad, 0.8f);

        const auto rendered = renderWithMidi(proc, {}, renderSamples);
        require(musique::qa::bufferIsFinite(rendered),
                "Pad " + juce::String(tc.pad) + " level=" + juce::String(tc.level, 1)
                + " caused non-finite output");
    }
}

void testEditorSmokeExposesPadRoutingAndUtilityDrawer()
{
    juce::ScopedJuceInitialiser_GUI gui;

    DrumSynthAudioProcessor processor;
    setParameterValue(processor.getAPVTS(), "selected_pad", 4.0f);
    setParameterValue(processor.getAPVTS(), DrumSynthAudioProcessor::makePadParamId(4, "output"), 2.0f);

    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    require(editor != nullptr, "Editor smoke test must create the drum editor");
    editor->setSize(1280, 760);
    editor->resized();

    auto* utilityButton = findComponentRecursive<juce::TextButton>(
        *editor,
        [] (juce::TextButton& button)
        {
            return button.getButtonText().containsIgnoreCase("Utility");
        });
    require(utilityButton != nullptr
                && utilityButton->isVisible()
                && !utilityButton->getBounds().isEmpty()
                && static_cast<bool>(utilityButton->onClick),
            "Drum utility drawer button must stay visible");

    auto* padPresetBox = findComponentRecursive<juce::ComboBox>(
        *editor,
        [] (juce::ComboBox& combo)
        {
            return combo.getTextWhenNothingSelected() == "Factory Pad Preset";
        });
    require(padPresetBox != nullptr && padPresetBox->isVisible() && !padPresetBox->getBounds().isEmpty(),
            "Drum pad preset selector must stay visible");

    auto* outputBox = findComponentRecursive<juce::ComboBox>(
        *editor,
        [] (juce::ComboBox& combo)
        {
            return combo.getNumItems() > 2
                && combo.getItemText(0) == "Master"
                && combo.getItemText(1) == "Out 1";
        });
    require(outputBox != nullptr && outputBox->isVisible() && !outputBox->getBounds().isEmpty(),
            "Drum pad routing selector must stay visible");
    require(outputBox->getText() == "Out 2",
            "Drum pad routing selector must reflect the selected pad output");

}

} // namespace

int main()
{
    try
    {
        testFactoryBankShape();
        testBootPresetMatchesFactory();
        testPresetStoragePaths();
        testStateSanitization();
        testUserPresetRoundTrip();
        testStateBinaryRoundTrip();
        testQualityModeAndDelaySyncStateRoundTrip();
        testMidiLearnRoundTripAndCcNeutrality();
        testUserPresetManifestGenerationAndBackfill();
        testTailLengthTracksDelayAndReverb();
        testSampleRateAndBlockSizeMatrix();
        testOfflineRenderIsDeterministic();
        testAllFactoryPresetsRenderStable();
        testChokeGroupEnforcement();
        testVoicePoolStressRendersCleanly();
        testAllVoiceModelsRenderStable();
        testMainAndAuxRouting();
        testAuxRoutingFallsBackToMainWhenBusUnavailable();
        testPadParamBoundsRenderClean();
        testEditorSmokeExposesPadRoutingAndUtilityDrawer();
    }
    catch (const std::exception& e)
    {
        std::cerr << "UWdeVST drum production tests failed: " << e.what() << '\n';
        return 1;
    }

    std::cout << "UWdeVST drum production tests: OK (18 tests)\n";
    return 0;
}
