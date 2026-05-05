#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>

namespace
{
constexpr const char* kOutputGain = "output_gain";
constexpr const char* kSingleNoteMode = "single_note_mode";
constexpr const char* kSelectedPad = "selected_pad";
constexpr const char* kPresetIndexProperty = "factory_preset_index";

constexpr int kPresetFormatVersion = 1;

constexpr const char* kMacroPunch = "macro_punch";
constexpr const char* kMacroWeight = "macro_weight";
constexpr const char* kMacroAir = "macro_air";
constexpr const char* kMacroDirt = "macro_dirt";

constexpr const char* kCompThreshold = "comp_threshold";
constexpr const char* kCompRatio = "comp_ratio";
constexpr const char* kCompAttack = "comp_attack";
constexpr const char* kCompRelease = "comp_release";
constexpr const char* kCompMakeup = "comp_makeup";
constexpr const char* kCompMix = "comp_mix";

constexpr const char* kSatDrive = "sat_drive";
constexpr const char* kSatMix = "sat_mix";

constexpr const char* kTransientAttack = "transient_attack";
constexpr const char* kTransientSustain = "transient_sustain";
constexpr const char* kTransientMix = "transient_mix";

constexpr const char* kPadOutputSuffix = "output";
constexpr const char* kClapSpreadSuffix = "clap_spread";
constexpr const char* kClapDensitySuffix = "clap_density";
constexpr const char* kMetallicDensitySuffix = "metallic_density";
constexpr const char* kOpenAmountSuffix = "open_amount";
constexpr const char* kBodyToneSuffix = "body_tone";
constexpr const char* kModalRingSuffix = "modal_ring";
constexpr const char* kFmIndexSuffix = "fm_index";
constexpr const char* kFmSweepSuffix = "fm_sweep";
// Audit Phase 5: per-pad parameter suffixes
constexpr const char* kVelToClickSuffix = "vel_to_click";  // D1
constexpr const char* kRevSendSuffix    = "rev_send";       // D3
constexpr const char* kDlySendSuffix    = "dly_send";       // D3

constexpr const char* kReverbSize      = "reverb_size";
constexpr const char* kReverbDamping   = "reverb_damping";
constexpr const char* kReverbWidth     = "reverb_width";
constexpr const char* kReverbMix       = "reverb_mix";
constexpr const char* kReverbPredelay  = "reverb_predelay";

constexpr const char* kEqLowFreq    = "eq_low_freq";
constexpr const char* kEqLowGain    = "eq_low_gain";
constexpr const char* kEqMidFreq    = "eq_mid_freq";
constexpr const char* kEqMidGain    = "eq_mid_gain";
constexpr const char* kEqMidQ       = "eq_mid_q";
constexpr const char* kEqHighFreq   = "eq_high_freq";
constexpr const char* kEqHighGain   = "eq_high_gain";
constexpr const char* kFxEqEn       = "fx_eq_en";

constexpr const char* kChorusRate   = "chorus_rate";
constexpr const char* kChorusDepth  = "chorus_depth";
constexpr const char* kChorusMix    = "chorus_mix";
constexpr const char* kFxChorusEn   = "fx_chorus_en";

constexpr const char* kDelayTime     = "delay_time";
constexpr const char* kDelayFeedback = "delay_feedback";
constexpr const char* kDelayMix      = "delay_mix";
constexpr const char* kDelaySync     = "delay_sync";
constexpr const char* kDelayNoteDiv  = "delay_note_div";
constexpr const char* kFxDelayEn     = "fx_delay_en";

constexpr const char* kLimiterThreshold = "limiter_threshold";
constexpr const char* kLimiterRelease   = "limiter_release";
constexpr const char* kFxLimiterEn      = "fx_limiter_en";
constexpr const char* kFxReverbEn       = "fx_reverb_en";
constexpr const char* kFxTransientEn    = "fx_transient_en";
constexpr const char* kFxSaturatorEn    = "fx_saturator_en";
constexpr const char* kFxCompEn         = "fx_comp_en";
constexpr const char* kVelocityCurve    = "velocity_curve";
constexpr const char* kLfoRate          = "lfo_rate";
constexpr const char* kLfoDepth         = "lfo_depth";
constexpr const char* kLfoWave          = "lfo_wave";
constexpr const char* kHumanizeTiming   = "humanize_timing";
constexpr const char* kHumanizeLevel    = "humanize_level";
constexpr const char* kAuxPostFx        = "aux_post_fx";
constexpr const char* kQualityMode      = "quality_mode";

constexpr const char* kPadPresetIndexProperty = "pad_preset_index_";
constexpr const char* kUserPresetNameProperty = "user_preset_name";
constexpr const char* kUserPresetFileProperty = "user_preset_file";
constexpr const char* kPresetFamilyProperty = "preset_family_label";
constexpr const char* kPresetMixRoleProperty = "preset_mix_role";
constexpr const char* kPresetDescriptionProperty = "preset_description";
constexpr const char* kPresetOutputProfileProperty = "preset_output_profile";
constexpr const char* kPresetNominalPeakProperty = "preset_nominal_peak_db";
constexpr const char* kPresetTagsProperty = "preset_tags";
constexpr const char* kPresetSourceProperty = "preset_source";

juce::StringArray makePadOutputChoices()
{
    juce::StringArray outputs;
    outputs.add("Master");

    for (int i = 0; i < DrumSynthAudioProcessor::kNumAuxOutputs; ++i)
        outputs.add("Out " + juce::String(i + 1));

    return outputs;
}

float clamp01(const float value)
{
    return juce::jlimit(0.0f, 1.0f, value);
}

juce::StringArray splitTags(const juce::String& raw)
{
    juce::StringArray tags;
    const auto tokens = juce::StringArray::fromTokens(raw, ",;", "\"");
    for (const auto& token : tokens)
    {
        const auto cleaned = token.trim();
        if (cleaned.isNotEmpty())
            tags.addIfNotAlreadyThere(cleaned);
    }
    return tags;
}

juce::String joinTags(const juce::StringArray& tags)
{
    juce::StringArray cleaned;
    for (const auto& tag : tags)
    {
        const auto trimmed = tag.trim();
        if (trimmed.isNotEmpty())
            cleaned.addIfNotAlreadyThere(trimmed);
    }
    return cleaned.joinIntoString(";");
}

juce::File findWritableDirectory(const juce::File& preferred, const juce::String& fallbackRelative)
{
    auto tryDirectory = [](const juce::File& base) -> juce::File
    {
        auto dir = base;
        dir.createDirectory();
        if (!dir.isDirectory())
            return {};

        auto probe = dir.getNonexistentChildFile(".write_probe", ".tmp", false);
        if (probe.replaceWithText("ok"))
        {
            probe.deleteFile();
            return dir;
        }

        probe.deleteFile();
        return {};
    };

    if (auto dir = tryDirectory(preferred); dir != juce::File{})
        return dir;

    const auto tempFallback = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getChildFile(fallbackRelative);
    if (auto dir = tryDirectory(tempFallback); dir != juce::File{})
        return dir;

    auto cwdFallback = juce::File::getCurrentWorkingDirectory()
                           .getChildFile(".musique_user_data")
                           .getChildFile(fallbackRelative);
    cwdFallback.createDirectory();
    return cwdFallback;
}

juce::File getPresetManifestFile(const juce::File& presetFile)
{
    return presetFile.getSiblingFile(presetFile.getFileNameWithoutExtension() + ".preset.json");
}

DrumSynthAudioProcessor::PresetLibraryEntry makeFactoryPresetEntry(const mds::KitPreset& preset, const int index)
{
    DrumSynthAudioProcessor::PresetLibraryEntry entry;
    entry.name = juce::String(preset.name);
    entry.familyLabel = juce::String(preset.familyLabel);
    entry.mixRole = juce::String(preset.mixRole);
    entry.description = juce::String(preset.description);
    entry.outputProfile = juce::String(preset.outputProfile);
    entry.nominalPeakDb = preset.nominalPeakDb;
    for (const auto& tag : preset.tags)
        entry.tags.addIfNotAlreadyThere(juce::String(tag));
    entry.isFactory = true;
    entry.factoryIndex = index;
    return entry;
}

DrumSynthAudioProcessor::PresetLibraryEntry makePresetEntryFromState(const juce::ValueTree& state,
                                                                     const juce::File& presetFile,
                                                                     const std::vector<mds::KitPreset>& factoryPresets)
{
    DrumSynthAudioProcessor::PresetLibraryEntry entry;
    entry.name = state.getProperty(kUserPresetNameProperty, presetFile.getFileNameWithoutExtension()).toString();
    entry.familyLabel = state.getProperty(kPresetFamilyProperty, {}).toString();
    entry.mixRole = state.getProperty(kPresetMixRoleProperty, {}).toString();
    entry.description = state.getProperty(kPresetDescriptionProperty, {}).toString();
    entry.outputProfile = state.getProperty(kPresetOutputProfileProperty, {}).toString();
    entry.nominalPeakDb = static_cast<float>(state.getProperty(kPresetNominalPeakProperty, -6.0f));
    entry.tags = splitTags(state.getProperty(kPresetTagsProperty, {}).toString());
    entry.isFactory = false;
    entry.factoryIndex = -1;
    entry.presetFile = presetFile;
    entry.manifestFile = getPresetManifestFile(presetFile);

    const auto presetIndex = static_cast<int>(state.getProperty(kPresetIndexProperty, -1));
    if (presetIndex >= 0 && presetIndex < static_cast<int>(factoryPresets.size()))
    {
        const auto factoryEntry = makeFactoryPresetEntry(factoryPresets[static_cast<std::size_t>(presetIndex)], presetIndex);
        if (entry.familyLabel.isEmpty()) entry.familyLabel = factoryEntry.familyLabel;
        if (entry.mixRole.isEmpty()) entry.mixRole = factoryEntry.mixRole;
        if (entry.description.isEmpty()) entry.description = factoryEntry.description;
        if (entry.outputProfile.isEmpty()) entry.outputProfile = factoryEntry.outputProfile;
        if (entry.tags.isEmpty()) entry.tags = factoryEntry.tags;
        if (!std::isfinite(entry.nominalPeakDb)) entry.nominalPeakDb = factoryEntry.nominalPeakDb;
    }

    if (entry.familyLabel.isEmpty()) entry.familyLabel = "User";
    if (entry.mixRole.isEmpty()) entry.mixRole = "custom";
    if (entry.description.isEmpty()) entry.description = "Preset utilisateur.";
    if (entry.outputProfile.isEmpty()) entry.outputProfile = "master-ready";
    if (!std::isfinite(entry.nominalPeakDb)) entry.nominalPeakDb = -6.0f;
    return entry;
}

void writePresetMetadataProperties(juce::ValueTree& state, const DrumSynthAudioProcessor::PresetLibraryEntry& entry)
{
    state.setProperty(kPresetFamilyProperty, entry.familyLabel, nullptr);
    state.setProperty(kPresetMixRoleProperty, entry.mixRole, nullptr);
    state.setProperty(kPresetDescriptionProperty, entry.description, nullptr);
    state.setProperty(kPresetOutputProfileProperty, entry.outputProfile, nullptr);
    state.setProperty(kPresetNominalPeakProperty, entry.nominalPeakDb, nullptr);
    state.setProperty(kPresetTagsProperty, joinTags(entry.tags), nullptr);
    state.setProperty(kPresetSourceProperty, entry.isFactory ? "factory" : "user", nullptr);
}

bool writePresetManifestFile(const juce::File& manifestFile, const DrumSynthAudioProcessor::PresetLibraryEntry& entry)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("name", entry.name);
    obj->setProperty("familyLabel", entry.familyLabel);
    obj->setProperty("mixRole", entry.mixRole);
    obj->setProperty("description", entry.description);
    obj->setProperty("outputProfile", entry.outputProfile);
    obj->setProperty("nominalPeakDb", entry.nominalPeakDb);
    obj->setProperty("source", entry.isFactory ? "factory" : "user");
    obj->setProperty("presetVersion", mds::kPresetVersion);
    obj->setProperty("tags", joinTags(entry.tags));

    const auto json = juce::JSON::toString(juce::var(obj), true);
    return manifestFile.replaceWithText(json);
}

bool readPresetManifestFile(const juce::File& manifestFile, DrumSynthAudioProcessor::PresetLibraryEntry& entry)
{
    if (!manifestFile.existsAsFile())
        return false;

    auto parsed = juce::JSON::parse(manifestFile.loadFileAsString());
    auto* object = parsed.getDynamicObject();
    if (object == nullptr)
        return false;

    entry.manifestFile = manifestFile;
    entry.name = object->getProperty("name").toString();
    entry.familyLabel = object->getProperty("familyLabel").toString();
    entry.mixRole = object->getProperty("mixRole").toString();
    entry.description = object->getProperty("description").toString();
    entry.outputProfile = object->getProperty("outputProfile").toString();
    entry.nominalPeakDb = static_cast<float>(object->getProperty("nominalPeakDb"));
    entry.tags.clear();
    if (const auto* array = object->getProperty("tags").getArray())
    {
        for (const auto& value : *array)
            entry.tags.addIfNotAlreadyThere(value.toString());
    }
    else
    {
        entry.tags = splitTags(object->getProperty("tags").toString());
    }
    return true;
}

void writeMidiLearnXml(juce::XmlElement& root, const std::map<int, juce::String>& midiLearnMap)
{
    if (midiLearnMap.empty())
        return;

    auto* midiLearn = root.createNewChildElement("MidiLearn");
    for (const auto& [cc, paramId] : midiLearnMap)
    {
        auto* mapping = midiLearn->createNewChildElement("Mapping");
        mapping->setAttribute("cc", cc);
        mapping->setAttribute("param", paramId);
    }
}

void readMidiLearnXml(const juce::XmlElement& root, std::map<int, juce::String>& midiLearnMap)
{
    midiLearnMap.clear();
    if (const auto* midiLearn = root.getChildByName("MidiLearn"))
    {
        for (auto* mapping = midiLearn->getChildByName("Mapping");
             mapping != nullptr;
             mapping = mapping->getNextElementWithTagName("Mapping"))
        {
            const auto paramId = mapping->getStringAttribute("param").trim();
            const int cc = mapping->getIntAttribute("cc", -1);
            if (cc >= 0 && cc <= 127 && paramId.isNotEmpty())
                midiLearnMap[cc] = paramId;
        }
    }
}

bool isReservedMidiCc(const int ccNumber)
{
    return ccNumber == 102
        || ccNumber == 103
        || (ccNumber >= 44 && ccNumber <= 50)
        || (ccNumber >= 21 && ccNumber <= 28);
}

bool tryParseXmlDoubleAttribute(const juce::XmlElement& xml, const char* attrName, double& value)
{
    if (!xml.hasAttribute(attrName))
        return false;

    const auto raw = xml.getStringAttribute(attrName).trim();
    if (raw.isEmpty())
        return false;

    char* end = nullptr;
    const auto parsed = std::strtod(raw.toRawUTF8(), &end);
    if (end == raw.toRawUTF8() || end == nullptr || *end != '\0' || !std::isfinite(parsed))
        return false;

    value = parsed;
    return true;
}

float readValidatedXmlFloat(const juce::XmlElement& xml,
                            const char* attrName,
                            float fallback,
                            float minValue,
                            float maxValue,
                            int& warningCount)
{
    double parsed = 0.0;
    if (!tryParseXmlDoubleAttribute(xml, attrName, parsed))
    {
        if (xml.hasAttribute(attrName))
            ++warningCount;
        return fallback;
    }

    const auto clamped = juce::jlimit(minValue, maxValue, static_cast<float>(parsed));
    if (clamped != static_cast<float>(parsed))
        ++warningCount;
    return clamped;
}

int readValidatedXmlInt(const juce::XmlElement& xml,
                        const char* attrName,
                        int fallback,
                        int minValue,
                        int maxValue,
                        int& warningCount)
{
    double parsed = 0.0;
    if (!tryParseXmlDoubleAttribute(xml, attrName, parsed))
    {
        if (xml.hasAttribute(attrName))
            ++warningCount;
        return fallback;
    }

    const auto rounded = static_cast<int>(std::lround(parsed));
    const auto clamped = juce::jlimit(minValue, maxValue, rounded);
    if (clamped != rounded || std::abs(parsed - std::round(parsed)) > 1.0e-6)
        ++warningCount;
    return clamped;
}

void sanitizeLoadedParameter(juce::RangedAudioParameter& parameter,
                             std::atomic<float>& value,
                             const bool notifyHost = false)
{
    float normalized = parameter.convertTo0to1(value.load());
    if (!std::isfinite(normalized))
        normalized = parameter.getDefaultValue();
    normalized = juce::jlimit(0.0f, 1.0f, normalized);

    if (notifyHost)
        parameter.setValueNotifyingHost(normalized);
    else
        parameter.setValue(normalized);
}

void sanitizeParameterState(juce::AudioProcessorValueTreeState& parameters)
{
    auto sanitizeById = [&](const juce::String& id)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*>(parameters.getParameter(id));
        auto* raw = parameters.getRawParameterValue(id);
        if (param != nullptr && raw != nullptr)
            sanitizeLoadedParameter(*param, *raw);
    };

    sanitizeById(kOutputGain);
    sanitizeById(kSingleNoteMode);
    sanitizeById(kSelectedPad);
    sanitizeById(kMacroPunch);
    sanitizeById(kMacroWeight);
    sanitizeById(kMacroAir);
    sanitizeById(kMacroDirt);
    sanitizeById(kCompThreshold);
    sanitizeById(kCompRatio);
    sanitizeById(kCompAttack);
    sanitizeById(kCompRelease);
    sanitizeById(kCompMakeup);
    sanitizeById(kCompMix);
    sanitizeById(kSatDrive);
    sanitizeById(kSatMix);
    sanitizeById(kTransientAttack);
    sanitizeById(kTransientSustain);
    sanitizeById(kTransientMix);
    sanitizeById(kReverbSize);
    sanitizeById(kReverbDamping);
    sanitizeById(kReverbWidth);
    sanitizeById(kReverbMix);
    sanitizeById(kReverbPredelay);
    sanitizeById(kEqLowFreq);
    sanitizeById(kEqLowGain);
    sanitizeById(kEqMidFreq);
    sanitizeById(kEqMidGain);
    sanitizeById(kEqMidQ);
    sanitizeById(kEqHighFreq);
    sanitizeById(kEqHighGain);
    sanitizeById(kFxEqEn);
    sanitizeById(kChorusRate);
    sanitizeById(kChorusDepth);
    sanitizeById(kChorusMix);
    sanitizeById(kFxChorusEn);
    sanitizeById(kDelayTime);
    sanitizeById(kDelayFeedback);
    sanitizeById(kDelayMix);
    sanitizeById(kDelaySync);
    sanitizeById(kDelayNoteDiv);
    sanitizeById(kFxDelayEn);
    sanitizeById(kLimiterThreshold);
    sanitizeById(kLimiterRelease);
    sanitizeById(kFxLimiterEn);
    sanitizeById(kAuxPostFx);
    sanitizeById(kQualityMode);

    for (int pad = 0; pad < mds::kNumPads; ++pad)
    {
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "level"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "tune"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "decay"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "attack"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "pitch_drop"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "pitch_decay"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "noise"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "click"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "drive"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "cutoff"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, "pan"));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kClapSpreadSuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kClapDensitySuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kMetallicDensitySuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kOpenAmountSuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kBodyToneSuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kModalRingSuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kFmIndexSuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kFmSweepSuffix));
        // Audit Phase 5 D1/D3: clamp new per-pad params on state load.
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kVelToClickSuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kRevSendSuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kDlySendSuffix));
        sanitizeById(DrumSynthAudioProcessor::makePadParamId(pad, kPadOutputSuffix));
    }
}

void applyPresetMigrationDefaults(juce::AudioProcessorValueTreeState& parameters, const int savedVersion)
{
    if (savedVersion >= 5)
        return;

    auto setDefaultIfMissing = [&](const juce::String& id)
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameters.getParameter(id)))
            ranged->setValue(ranged->getDefaultValue());
    };

    if (savedVersion < 2)
    {
        setDefaultIfMissing(kReverbPredelay);
        setDefaultIfMissing(kFxEqEn);
        setDefaultIfMissing(kFxChorusEn);
        setDefaultIfMissing(kFxDelayEn);
        setDefaultIfMissing(kFxLimiterEn);
    }

    if (savedVersion < 3)
    {
        for (int pad = 0; pad < mds::kNumPads; ++pad)
        {
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kClapSpreadSuffix));
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kClapDensitySuffix));
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kMetallicDensitySuffix));
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kOpenAmountSuffix));
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kBodyToneSuffix));
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kModalRingSuffix));
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kFmIndexSuffix));
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kFmSweepSuffix));
        }
    }

    if (savedVersion < 4)
        setDefaultIfMissing(kQualityMode);

    if (savedVersion < 5)
    {
        setDefaultIfMissing(kFxReverbEn);
        setDefaultIfMissing(kFxTransientEn);
        setDefaultIfMissing(kFxSaturatorEn);
        setDefaultIfMissing(kFxCompEn);
        setDefaultIfMissing(kVelocityCurve);
        setDefaultIfMissing(kLfoRate);
        setDefaultIfMissing(kLfoDepth);
        setDefaultIfMissing(kLfoWave);
        setDefaultIfMissing(kHumanizeTiming);
        setDefaultIfMissing(kHumanizeLevel);
    }

    // Audit Phase 5 D1/D3: ensure new per-pad parameters get their factory
    // defaults when loading older presets (vel_to_click / rev_send / dly_send).
    if (savedVersion < 6)
    {
        for (int pad = 0; pad < mds::kNumPads; ++pad)
        {
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kVelToClickSuffix));
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kRevSendSuffix));
            setDefaultIfMissing(DrumSynthAudioProcessor::makePadParamId(pad, kDlySendSuffix));
        }
    }
}

void restorePresetMetadata(const juce::ValueTree& restoredState,
                           juce::File& currentUserPresetFile,
                           int& currentPresetIndex,
                           std::array<int, mds::kNumPads>& currentPadPresetIndices,
                           const std::vector<mds::KitPreset>& factoryPresets)
{
    currentPresetIndex = static_cast<int>(restoredState.getProperty(kPresetIndexProperty, -1));
    if (factoryPresets.empty())
        currentPresetIndex = -1;
    else
        currentPresetIndex = juce::jlimit(0, static_cast<int>(factoryPresets.size()) - 1, currentPresetIndex);

    currentUserPresetFile = {};
    auto userPath = restoredState.getProperty(kUserPresetFileProperty, "").toString();
    if (userPath.isNotEmpty())
    {
        juce::File f(userPath);
        if (f.existsAsFile())
            currentUserPresetFile = f;
    }

    for (int pad = 0; pad < mds::kNumPads; ++pad)
    {
        const auto key = juce::String(kPadPresetIndexProperty) + juce::String(pad);
        const int idx = static_cast<int>(restoredState.getProperty(key, -1));
        const int maxIdx = static_cast<int>(mds::getFactoryPadPresets(pad).size()) - 1;
        currentPadPresetIndices[static_cast<std::size_t>(pad)] = maxIdx >= 0
            ? juce::jlimit(-1, maxIdx, idx)
            : -1;
    }
}

void writeGlobalFxAttributes(juce::XmlElement& root, const mds::GlobalFxSettings& fx)
{
    root.setAttribute("output_gain",        static_cast<double>(fx.outputGainDb));
    root.setAttribute("macro_punch",        static_cast<double>(fx.macroPunch));
    root.setAttribute("macro_weight",       static_cast<double>(fx.macroWeight));
    root.setAttribute("macro_air",          static_cast<double>(fx.macroAir));
    root.setAttribute("macro_dirt",         static_cast<double>(fx.macroDirt));
    root.setAttribute("comp_threshold",     static_cast<double>(fx.compThreshold));
    root.setAttribute("comp_ratio",         static_cast<double>(fx.compRatio));
    root.setAttribute("comp_attack",        static_cast<double>(fx.compAttack));
    root.setAttribute("comp_release",       static_cast<double>(fx.compRelease));
    root.setAttribute("comp_makeup",        static_cast<double>(fx.compMakeup));
    root.setAttribute("comp_mix",           static_cast<double>(fx.compMix));
    root.setAttribute("sat_drive",          static_cast<double>(fx.satDrive));
    root.setAttribute("sat_mix",            static_cast<double>(fx.satMix));
    root.setAttribute("transient_attack",   static_cast<double>(fx.transientAttack));
    root.setAttribute("transient_sustain",  static_cast<double>(fx.transientSustain));
    root.setAttribute("transient_mix",      static_cast<double>(fx.transientMix));
    root.setAttribute("reverb_size",        static_cast<double>(fx.reverbSize));
    root.setAttribute("reverb_damping",     static_cast<double>(fx.reverbDamping));
    root.setAttribute("reverb_width",       static_cast<double>(fx.reverbWidth));
    root.setAttribute("reverb_mix",         static_cast<double>(fx.reverbMix));
    root.setAttribute("reverb_predelay",    static_cast<double>(fx.reverbPredelay));
    root.setAttribute("eq_low_freq",        static_cast<double>(fx.eqLowFreq));
    root.setAttribute("eq_low_gain",        static_cast<double>(fx.eqLowGain));
    root.setAttribute("eq_mid_freq",        static_cast<double>(fx.eqMidFreq));
    root.setAttribute("eq_mid_gain",        static_cast<double>(fx.eqMidGain));
    root.setAttribute("eq_mid_q",           static_cast<double>(fx.eqMidQ));
    root.setAttribute("eq_high_freq",       static_cast<double>(fx.eqHighFreq));
    root.setAttribute("eq_high_gain",       static_cast<double>(fx.eqHighGain));
    root.setAttribute("fx_eq_en",           fx.eqEnable ? 1 : 0);
    root.setAttribute("chorus_rate",        static_cast<double>(fx.chorusRate));
    root.setAttribute("chorus_depth",       static_cast<double>(fx.chorusDepth));
    root.setAttribute("chorus_mix",         static_cast<double>(fx.chorusMix));
    root.setAttribute("fx_chorus_en",       fx.chorusEnable ? 1 : 0);
    root.setAttribute("delay_time",         static_cast<double>(fx.delayTime));
    root.setAttribute("delay_feedback",     static_cast<double>(fx.delayFeedback));
    root.setAttribute("delay_mix",          static_cast<double>(fx.delayMix));
    root.setAttribute("delay_sync",         fx.delaySync ? 1 : 0);
    root.setAttribute("delay_note_div",     fx.delayNoteDiv);
    root.setAttribute("fx_delay_en",        fx.delayEnable ? 1 : 0);
    root.setAttribute("limiter_threshold",  static_cast<double>(fx.limiterThreshold));
    root.setAttribute("limiter_release",    static_cast<double>(fx.limiterRelease));
    root.setAttribute("fx_limiter_en",      fx.limiterEnable ? 1 : 0);
    root.setAttribute("fx_reverb_en",       fx.reverbEnable ? 1 : 0);
    root.setAttribute("fx_transient_en",    fx.transientEnable ? 1 : 0);
    root.setAttribute("fx_saturator_en",    fx.saturatorEnable ? 1 : 0);
    root.setAttribute("fx_comp_en",         fx.compEnable ? 1 : 0);
    root.setAttribute("velocity_curve",     fx.velocityCurve);
    root.setAttribute("lfo_rate",           static_cast<double>(fx.lfoRate));
    root.setAttribute("lfo_depth",          static_cast<double>(fx.lfoDepth));
    root.setAttribute("lfo_wave",           fx.lfoWave);
    root.setAttribute("humanize_timing",    static_cast<double>(fx.humanizeTimingMs));
    root.setAttribute("humanize_level",     static_cast<double>(fx.humanizeLevel));
    root.setAttribute("aux_post_fx",        fx.auxPostFx ? 1 : 0);
}

void readGlobalFxAttributes(const juce::XmlElement& xml, mds::GlobalFxSettings& fx)
{
    int warningCount = 0;
    fx.outputGainDb      = readValidatedXmlFloat(xml, "output_gain",        fx.outputGainDb,    -24.0f,   24.0f, warningCount);
    fx.macroPunch        = readValidatedXmlFloat(xml, "macro_punch",        fx.macroPunch,        0.0f,    1.0f, warningCount);
    fx.macroWeight       = readValidatedXmlFloat(xml, "macro_weight",       fx.macroWeight,       0.0f,    1.0f, warningCount);
    fx.macroAir          = readValidatedXmlFloat(xml, "macro_air",          fx.macroAir,          0.0f,    1.0f, warningCount);
    fx.macroDirt         = readValidatedXmlFloat(xml, "macro_dirt",         fx.macroDirt,         0.0f,    1.0f, warningCount);
    fx.compThreshold     = readValidatedXmlFloat(xml, "comp_threshold",     fx.compThreshold,   -60.0f,    0.0f, warningCount);
    fx.compRatio         = readValidatedXmlFloat(xml, "comp_ratio",         fx.compRatio,         1.0f,   20.0f, warningCount);
    fx.compAttack        = readValidatedXmlFloat(xml, "comp_attack",        fx.compAttack,        0.1f,  100.0f, warningCount);
    fx.compRelease       = readValidatedXmlFloat(xml, "comp_release",       fx.compRelease,       5.0f,  500.0f, warningCount);
    fx.compMakeup        = readValidatedXmlFloat(xml, "comp_makeup",        fx.compMakeup,        0.0f,   24.0f, warningCount);
    fx.compMix           = readValidatedXmlFloat(xml, "comp_mix",           fx.compMix,           0.0f,    1.0f, warningCount);
    fx.satDrive          = readValidatedXmlFloat(xml, "sat_drive",          fx.satDrive,          1.0f,   16.0f, warningCount);
    fx.satMix            = readValidatedXmlFloat(xml, "sat_mix",            fx.satMix,            0.0f,    1.0f, warningCount);
    fx.transientAttack   = readValidatedXmlFloat(xml, "transient_attack",   fx.transientAttack,  -1.0f,    1.0f, warningCount);
    fx.transientSustain  = readValidatedXmlFloat(xml, "transient_sustain",  fx.transientSustain, -1.0f,    1.0f, warningCount);
    fx.transientMix      = readValidatedXmlFloat(xml, "transient_mix",      fx.transientMix,      0.0f,    1.0f, warningCount);
    fx.reverbSize        = readValidatedXmlFloat(xml, "reverb_size",        fx.reverbSize,        0.0f,    1.0f, warningCount);
    fx.reverbDamping     = readValidatedXmlFloat(xml, "reverb_damping",     fx.reverbDamping,     0.0f,    1.0f, warningCount);
    fx.reverbWidth       = readValidatedXmlFloat(xml, "reverb_width",       fx.reverbWidth,       0.0f,    1.0f, warningCount);
    fx.reverbMix         = readValidatedXmlFloat(xml, "reverb_mix",         fx.reverbMix,         0.0f,    1.0f, warningCount);
    fx.reverbPredelay    = readValidatedXmlFloat(xml, "reverb_predelay",    fx.reverbPredelay,    0.0f,  100.0f, warningCount);
    fx.eqLowFreq         = readValidatedXmlFloat(xml, "eq_low_freq",        fx.eqLowFreq,        40.0f,  500.0f, warningCount);
    fx.eqLowGain         = readValidatedXmlFloat(xml, "eq_low_gain",        fx.eqLowGain,       -12.0f,   12.0f, warningCount);
    fx.eqMidFreq         = readValidatedXmlFloat(xml, "eq_mid_freq",        fx.eqMidFreq,       200.0f, 8000.0f, warningCount);
    fx.eqMidGain         = readValidatedXmlFloat(xml, "eq_mid_gain",        fx.eqMidGain,       -12.0f,   12.0f, warningCount);
    fx.eqMidQ            = readValidatedXmlFloat(xml, "eq_mid_q",           fx.eqMidQ,            0.1f,   10.0f, warningCount);
    fx.eqHighFreq        = readValidatedXmlFloat(xml, "eq_high_freq",       fx.eqHighFreq,     2000.0f,16000.0f, warningCount);
    fx.eqHighGain        = readValidatedXmlFloat(xml, "eq_high_gain",       fx.eqHighGain,      -12.0f,   12.0f, warningCount);
    fx.eqEnable          = xml.getIntAttribute("fx_eq_en", fx.eqEnable ? 1 : 0) != 0;
    fx.chorusRate        = readValidatedXmlFloat(xml, "chorus_rate",        fx.chorusRate,       0.1f,    5.0f, warningCount);
    fx.chorusDepth       = readValidatedXmlFloat(xml, "chorus_depth",       fx.chorusDepth,      0.0f,    1.0f, warningCount);
    fx.chorusMix         = readValidatedXmlFloat(xml, "chorus_mix",         fx.chorusMix,        0.0f,    1.0f, warningCount);
    fx.chorusEnable      = xml.getIntAttribute("fx_chorus_en", fx.chorusEnable ? 1 : 0) != 0;
    fx.delayTime         = readValidatedXmlFloat(xml, "delay_time",         fx.delayTime,        1.0f, 2000.0f, warningCount);
    fx.delayFeedback     = readValidatedXmlFloat(xml, "delay_feedback",     fx.delayFeedback,    0.0f,    0.95f, warningCount);
    fx.delayMix          = readValidatedXmlFloat(xml, "delay_mix",          fx.delayMix,         0.0f,    1.0f, warningCount);
    fx.delaySync         = xml.getIntAttribute("delay_sync", fx.delaySync ? 1 : 0) != 0;
    fx.delayNoteDiv      = readValidatedXmlInt(xml, "delay_note_div", fx.delayNoteDiv, 0, 4, warningCount);
    fx.delayEnable       = xml.getIntAttribute("fx_delay_en", fx.delayEnable ? 1 : 0) != 0;
    fx.limiterThreshold  = readValidatedXmlFloat(xml, "limiter_threshold",  fx.limiterThreshold,-12.0f,    0.0f, warningCount);
    fx.limiterRelease    = readValidatedXmlFloat(xml, "limiter_release",    fx.limiterRelease,   1.0f,  200.0f, warningCount);
    fx.limiterEnable     = xml.getIntAttribute("fx_limiter_en", fx.limiterEnable ? 1 : 0) != 0;
    fx.reverbEnable      = xml.getIntAttribute("fx_reverb_en", fx.reverbEnable ? 1 : 0) != 0;
    fx.transientEnable   = xml.getIntAttribute("fx_transient_en", fx.transientEnable ? 1 : 0) != 0;
    fx.saturatorEnable   = xml.getIntAttribute("fx_saturator_en", fx.saturatorEnable ? 1 : 0) != 0;
    fx.compEnable        = xml.getIntAttribute("fx_comp_en", fx.compEnable ? 1 : 0) != 0;
    fx.velocityCurve     = readValidatedXmlInt(xml, "velocity_curve", fx.velocityCurve, 0, 6, warningCount);
    fx.lfoRate           = readValidatedXmlFloat(xml, "lfo_rate",    fx.lfoRate,    0.1f,   20.0f, warningCount);
    fx.lfoDepth          = readValidatedXmlFloat(xml, "lfo_depth",   fx.lfoDepth,   0.0f,    1.0f, warningCount);
    fx.lfoWave           = readValidatedXmlInt(xml, "lfo_wave",      fx.lfoWave, 0, 3, warningCount);
    fx.humanizeTimingMs  = readValidatedXmlFloat(xml, "humanize_timing", fx.humanizeTimingMs, 0.0f, 50.0f, warningCount);
    fx.humanizeLevel     = readValidatedXmlFloat(xml, "humanize_level", fx.humanizeLevel, 0.0f, 0.2f, warningCount);
    fx.auxPostFx         = xml.getIntAttribute("aux_post_fx", fx.auxPostFx ? 1 : 0) != 0;

    if (warningCount > 0)
        juce::Logger::writeToLog("[DrumPreset] FX sanitization warnings=" + juce::String(warningCount));
}

// ── MIDI CC page system ──
// Slot: paramId!=nullptr → global param, padSuffix!=nullptr → per-pad param
struct CCSlot { const char* paramId; const char* padSuffix; };
constexpr int kKnobsPerPage = 8;

constexpr const char* kCCPageNames[DrumSynthAudioProcessor::kNumCCPages] = {
    "MACROS", "PAD MAIN", "PAD TONE", "REVERB",
    "DYNAMICS", "TRANSIENT", "EQ"
};

constexpr CCSlot kCCPages[DrumSynthAudioProcessor::kNumCCPages][kKnobsPerPage] = {
    // Page 0 — MACROS
    {{ kMacroPunch, nullptr }, { kMacroWeight, nullptr },
     { kMacroAir,   nullptr }, { kMacroDirt,   nullptr },
     { kOutputGain, nullptr }, { nullptr, nullptr },
     { nullptr, nullptr },     { nullptr, nullptr }},
    // Page 1 — PAD MAIN (per-pad)
    {{ nullptr, "level" },     { nullptr, "tune" },
     { nullptr, "decay" },     { nullptr, "attack" },
     { nullptr, "pitch_drop" },{ nullptr, "pitch_decay" },
     { nullptr, "noise" },     { nullptr, "click" }},
    // Page 2 — PAD TONE (per-pad)
    {{ nullptr, "drive" },     { nullptr, "cutoff" },
     { nullptr, "pan" },       { nullptr, nullptr },
     { nullptr, nullptr },     { nullptr, nullptr },
     { nullptr, nullptr },     { nullptr, nullptr }},
    // Page 3 — REVERB
    {{ kReverbSize,  nullptr }, { kReverbDamping, nullptr },
     { kReverbWidth, nullptr }, { kReverbMix,     nullptr },
     { kReverbPredelay, nullptr }, { nullptr, nullptr },
     { nullptr, nullptr },     { nullptr, nullptr }},
    // Page 4 — DYNAMICS
    {{ kCompThreshold, nullptr }, { kCompRatio,   nullptr },
     { kCompAttack,    nullptr }, { kCompRelease, nullptr },
     { kCompMakeup,    nullptr }, { kCompMix,     nullptr },
     { kSatDrive,      nullptr }, { kSatMix,      nullptr }},
    // Page 5 — TRANSIENT
    {{ kTransientAttack,  nullptr }, { kTransientSustain, nullptr },
     { kTransientMix,     nullptr }, { nullptr, nullptr },
     { nullptr, nullptr },           { nullptr, nullptr },
     { nullptr, nullptr },           { nullptr, nullptr }},
    // Page 6 — EQ
    {{ kEqLowFreq,  nullptr }, { kEqLowGain,  nullptr },
     { kEqMidFreq,  nullptr }, { kEqMidGain,  nullptr },
     { kEqMidQ,     nullptr }, { kEqHighFreq, nullptr },
     { kEqHighGain, nullptr }, { nullptr, nullptr }}
};

} // namespace

auto DrumSynthAudioProcessor::createBusLayout() -> BusesProperties
{
    BusesProperties buses;
    buses = buses.withOutput("Master", juce::AudioChannelSet::stereo(), true);

    for (int i = 0; i < kNumAuxOutputs; ++i)
        buses = buses.withOutput("Pad " + juce::String(i + 1) + " Out", juce::AudioChannelSet::stereo(), false);

    return buses;
}

DrumSynthAudioProcessor::DrumSynthAudioProcessor()
    : AudioProcessor(createBusLayout()),
      parameters(*this, nullptr, juce::Identifier("MDS_PARAMS"), createParameterLayout()),
      factoryPresets(mds::getFactoryPresets())
{
    currentPadPresetIndices.fill(-1);
    loadFactoryOverrides();
    backfillMissingPresetManifests();
    if (!factoryPresets.empty())
        applyFactoryPreset(0);
    else
        currentPresetIndex = -1;
    resetRuntimeTelemetry();
}

juce::AudioProcessorValueTreeState::ParameterLayout DrumSynthAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto outputChoices = makePadOutputChoices();

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kOutputGain,
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.01f),
        -3.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        kSingleNoteMode,
        "Single Note Mode",
        false));

    juce::StringArray padChoices;
    for (int i = 0; i < mds::kNumPads; ++i)
        padChoices.add("Pad " + juce::String(i + 1));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kSelectedPad,
        "Selected Pad",
        padChoices,
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroPunch,
        "Macro Punch",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroWeight,
        "Macro Weight",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroAir,
        "Macro Air",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kMacroDirt,
        "Macro Dirt",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.18f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompThreshold,
        "Comp Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.01f),
        -14.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompRatio,
        "Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.01f),
        2.4f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompAttack,
        "Comp Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.01f),
        8.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompRelease,
        "Comp Release",
        juce::NormalisableRange<float>(5.0f, 500.0f, 0.01f),
        140.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompMakeup,
        "Comp Makeup",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kCompMix,
        "Comp Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.55f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kSatDrive,
        "Sat Drive",
        juce::NormalisableRange<float>(1.0f, 16.0f, 0.01f),
        1.45f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kSatMix,
        "Sat Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.18f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kTransientAttack,
        "Transient Attack",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0001f),
        0.12f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kTransientSustain,
        "Transient Sustain",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.0001f),
        -0.05f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kTransientMix,
        "Transient Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
        0.28f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbSize,    "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.35f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbDamping, "Reverb Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.70f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbWidth,   "Reverb Width",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.80f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbMix,     "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.15f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kReverbPredelay, "Reverb Pre-delay",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 12.0f));

    // --- EQ ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqLowFreq, "EQ Low Freq",
        juce::NormalisableRange<float>(40.0f, 500.0f, 0.1f, 0.4f), 120.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqLowGain, "EQ Low Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqMidFreq, "EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 8000.0f, 0.1f, 0.35f), 1200.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqMidGain, "EQ Mid Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqMidQ, "EQ Mid Q",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqHighFreq, "EQ High Freq",
        juce::NormalisableRange<float>(2000.0f, 16000.0f, 0.1f, 0.4f), 6000.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kEqHighGain, "EQ High Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        kFxEqEn, "EQ Enable", false));

    // --- Chorus ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusRate, "Chorus Rate",
        juce::NormalisableRange<float>(0.1f, 5.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusDepth, "Chorus Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kChorusMix, "Chorus Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        kFxChorusEn, "Chorus Enable", false));

    // --- Delay ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kDelayTime, "Delay Time",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.35f), 300.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kDelayFeedback, "Delay Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.30f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kDelayMix, "Delay Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        kDelaySync, "Delay Sync", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kDelayNoteDiv, "Delay Note Div",
        juce::StringArray{ "1/4", "1/8", "1/16", "dotted 1/8", "triplet 1/8" }, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        kFxDelayEn, "Delay Enable", false));

    // --- Limiter ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLimiterThreshold, "Limiter Threshold",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.01f), -0.3f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLimiterRelease, "Limiter Release",
        juce::NormalisableRange<float>(1.0f, 200.0f, 0.1f), 50.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        kFxLimiterEn, "Limiter Enable", true));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        kFxReverbEn, "Reverb Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        kFxTransientEn, "Transient Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        kFxSaturatorEn, "Saturator Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        kFxCompEn, "Compressor Enable", true));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        kAuxPostFx, "Aux Post-FX", false));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kQualityMode, "Quality Mode",
        juce::StringArray{ "Live", "Studio" }, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kVelocityCurve, "Velocity Curve",
        juce::StringArray{ "Linear", "Soft", "Softer", "Hard", "Harder", "Fixed", "Touch" }, 0));

    // --- Global LFO ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLfoRate, "LFO Rate",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.4f), 2.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kLfoDepth, "LFO Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        kLfoWave, "LFO Wave",
        juce::StringArray{ "Sine", "Triangle", "Saw", "Square" }, 0));

    // --- Humanize ---
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kHumanizeTiming, "Humanize Timing",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        kHumanizeLevel, "Humanize Level",
        juce::NormalisableRange<float>(0.0f, 0.2f, 0.0001f), 0.0f));

    for (int pad = 0; pad < mds::kNumPads; ++pad)
    {
        const auto defaults = mds::getDefaultPadSettings(pad);
        const auto prefix = "Pad " + juce::String(pad + 1) + " ";

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "level"),
            prefix + "Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.level));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "tune"),
            prefix + "Tune",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
            defaults.tuneSemitones));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "decay"),
            prefix + "Decay",
            juce::NormalisableRange<float>(0.03f, 2.5f, 0.0001f),
            defaults.decaySeconds));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "attack"),
            prefix + "Attack",
            juce::NormalisableRange<float>(0.0f, 0.05f, 0.0001f),
            defaults.attackSeconds));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "pitch_drop"),
            prefix + "Pitch Drop",
            juce::NormalisableRange<float>(0.0f, 48.0f, 0.01f),
            defaults.pitchDropSemitones));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "pitch_decay"),
            prefix + "Pitch Decay",
            juce::NormalisableRange<float>(0.005f, 1.2f, 0.0001f),
            defaults.pitchDecaySeconds));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "noise"),
            prefix + "Noise",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.noiseAmount));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "click"),
            prefix + "Click",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.clickAmount));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "drive"),
            prefix + "Drive",
            juce::NormalisableRange<float>(1.0f, 12.0f, 0.01f),
            defaults.drive));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "cutoff"),
            prefix + "Cutoff",
            juce::NormalisableRange<float>(120.0f, 18000.0f, 0.0f, 0.28f),
            defaults.cutoffHz));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, "pan"),
            prefix + "Pan",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f),
            defaults.pan));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kClapSpreadSuffix),
            prefix + "Clap Spread",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.clapSpread));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kClapDensitySuffix),
            prefix + "Clap Density",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.clapDensity));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kMetallicDensitySuffix),
            prefix + "Metallic Density",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.metallicDensity));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kOpenAmountSuffix),
            prefix + "Open Amount",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.openAmount));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kBodyToneSuffix),
            prefix + "Body Tone",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.bodyTone));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kModalRingSuffix),
            prefix + "Modal Ring",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.modalRing));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kFmIndexSuffix),
            prefix + "FM Index",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.fmIndex));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kFmSweepSuffix),
            prefix + "FM Sweep",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.fmSweep));

        // Audit Phase 5 D1: per-pad velocity-to-click sensitivity.
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kVelToClickSuffix),
            prefix + "Vel To Click",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.velocityToClick));

        // Audit Phase 5 D3: per-pad FX sends.
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kRevSendSuffix),
            prefix + "Reverb Send",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.reverbSend));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            makePadParamId(pad, kDlySendSuffix),
            prefix + "Delay Send",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0001f),
            defaults.delaySend));

        const auto defaultOutputChoice = juce::jlimit(0, kNumAuxOutputs, pad + 1);
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            makePadParamId(pad, kPadOutputSuffix),
            prefix + "Output",
            outputChoices,
            defaultOutputChoice));
    }

    return layout;
}

juce::String DrumSynthAudioProcessor::makePadParamId(const int padIndex, const juce::String& suffix)
{
    return "pad_" + juce::String(padIndex) + "_" + suffix;
}

void DrumSynthAudioProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock)
{
    preparedSampleRate = std::max(1.0, sampleRate);
    constexpr double kMacroSmoothingSeconds = 0.02;
    constexpr double kGainMixSmoothingSeconds = 0.02;

    // Release all active voices back to the pool
    for (int i = 0; i < activeVoiceCount; ++i)
    {
        auto& av = activeVoices[static_cast<std::size_t>(i)];
        if (av.voice != nullptr)
            voicePool.release(av.voiceModel, av.voice);
        av = {};
    }
    activeVoiceCount = 0;

    // Discard any stale triggers queued before the sample rate changed
    triggerFifo.reset();
    triggerBatch.clear();
    pendingTriggerCount = 0;

    const juce::dsp::ProcessSpec spec {
        preparedSampleRate,
        static_cast<juce::uint32>(juce::jmax(1, samplesPerBlock)),
        static_cast<juce::uint32>(juce::jmax(1, getMainBusNumOutputChannels()))
    };

    compressor.reset();
    compressor.prepare(spec);
    compressor.setThreshold(getParamValue(kCompThreshold));
    compressor.setRatio(getParamValue(kCompRatio));
    compressor.setAttack(getParamValue(kCompAttack));
    compressor.setRelease(getParamValue(kCompRelease));

    compCache = CompressorCache{};
    triggerBatch.reserve(32);
    outputGainSmoother.reset(preparedSampleRate, kGainMixSmoothingSeconds);
    outputGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(getParamValue(kOutputGain)));
    transientAttackSmoother.reset(preparedSampleRate, kGainMixSmoothingSeconds);
    transientSustainSmoother.reset(preparedSampleRate, kGainMixSmoothingSeconds);
    transientMixSmoother.reset(preparedSampleRate, kGainMixSmoothingSeconds);
    transientAttackSmoother.setCurrentAndTargetValue(getParamValue(kTransientAttack));
    transientSustainSmoother.setCurrentAndTargetValue(getParamValue(kTransientSustain));
    transientMixSmoother.setCurrentAndTargetValue(getParamValue(kTransientMix));
    satDriveSmoother.reset(preparedSampleRate, kGainMixSmoothingSeconds);
    satMixSmoother.reset(preparedSampleRate, kGainMixSmoothingSeconds);
    satDriveSmoother.setCurrentAndTargetValue(getParamValue(kSatDrive));
    satMixSmoother.setCurrentAndTargetValue(getParamValue(kSatMix));
    macroPunchSmoother.reset(preparedSampleRate, kMacroSmoothingSeconds);
    macroWeightSmoother.reset(preparedSampleRate, kMacroSmoothingSeconds);
    macroAirSmoother.reset(preparedSampleRate, kMacroSmoothingSeconds);
    macroDirtSmoother.reset(preparedSampleRate, kMacroSmoothingSeconds);
    macroPunchValue = getParamValue(kMacroPunch);
    macroWeightValue = getParamValue(kMacroWeight);
    macroAirValue = getParamValue(kMacroAir);
    macroDirtValue = getParamValue(kMacroDirt);
    macroPunchSmoother.setCurrentAndTargetValue(macroPunchValue);
    macroWeightSmoother.setCurrentAndTargetValue(macroWeightValue);
    macroAirSmoother.setCurrentAndTargetValue(macroAirValue);
    macroDirtSmoother.setCurrentAndTargetValue(macroDirtValue);

    dattorroReverb.prepare(preparedSampleRate, samplesPerBlock);
    fxEq.prepare(preparedSampleRate);
    fxChorus.prepare(preparedSampleRate, samplesPerBlock);
    fxDelay.prepare(preparedSampleRate, samplesPerBlock);
    fxLimiter.prepare(preparedSampleRate);

    // Audit Phase 5 D3: dedicated send instances + scratch buffers.
    sendReverb.prepare(preparedSampleRate, samplesPerBlock);
    sendDelay.prepare(preparedSampleRate, samplesPerBlock);
    const int scratchCh = juce::jmax(2, static_cast<int>(spec.numChannels));
    reverbSendBuffer.setSize(scratchCh, static_cast<int>(spec.maximumBlockSize), false, true, true);
    delaySendBuffer.setSize(scratchCh, static_cast<int>(spec.maximumBlockSize), false, true, true);
    voiceScratchBuffer.setSize(scratchCh, static_cast<int>(spec.maximumBlockSize), false, true, true);
    currentPadReverbSend.fill(0.0f);
    currentPadDelaySend.fill(0.0f);

    fxDryBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize), false, true, true);
    transientFastEnv = { 0.0f, 0.0f };
    transientSlowEnv = { 0.0f, 0.0f };
    saturatorPrevSample = { 0.0f, 0.0f };
    lfoPhase = 0.0f;
    resetRuntimeTelemetry();
}

void DrumSynthAudioProcessor::releaseResources()
{
    for (int i = 0; i < activeVoiceCount; ++i)
    {
        auto& av = activeVoices[static_cast<std::size_t>(i)];
        if (av.voice != nullptr)
            voicePool.release(av.voiceModel, av.voice);
        av = {};
    }
    activeVoiceCount = 0;
    fxDryBuffer.setSize(0, 0);
    reverbSendBuffer.setSize(0, 0);
    delaySendBuffer.setSize(0, 0);
    voiceScratchBuffer.setSize(0, 0);
    saturatorPrevSample = { 0.0f, 0.0f };
    resetRuntimeTelemetry();
}

bool DrumSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.outputBuses.size() == 0)
        return false;

    if (layouts.outputBuses.size() != 1 + kNumAuxOutputs)
        return false;

    const auto mainOutput = layouts.getMainOutputChannelSet();
    if (mainOutput != juce::AudioChannelSet::mono() && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    for (int busIndex = 1; busIndex < layouts.outputBuses.size(); ++busIndex)
    {
        const auto auxSet = layouts.getChannelSet(false, busIndex);
        if (auxSet.isDisabled())
            continue;

        if (auxSet != juce::AudioChannelSet::mono() && auxSet != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void DrumSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto blockSamples = juce::jmax(1, buffer.getNumSamples());
    lastHostBpm.store(0.0f, std::memory_order_relaxed);
    delaySyncActive.store(false, std::memory_order_relaxed);
    macroPunchSmoother.setTargetValue(getParamValue(kMacroPunch));
    macroWeightSmoother.setTargetValue(getParamValue(kMacroWeight));
    macroAirSmoother.setTargetValue(getParamValue(kMacroAir));
    macroDirtSmoother.setTargetValue(getParamValue(kMacroDirt));
    macroPunchValue = macroPunchSmoother.skip(blockSamples);
    macroWeightValue = macroWeightSmoother.skip(blockSamples);
    macroAirValue = macroAirSmoother.skip(blockSamples);
    macroDirtValue = macroDirtSmoother.skip(blockSamples);

    velocityCurve = intToVelocityCurve(static_cast<int>(std::round(getParamValue(kVelocityCurve))));

    const auto outputBusCount = getBusCount(false);
    for (int busIndex = 0; busIndex < outputBusCount; ++busIndex)
        getBusBuffer(buffer, false, busIndex).clear();

    // Audit Phase 5 D3: snapshot per-pad send amounts for this block, and
    // clear the dedicated send scratch buses. Done once per block so the
    // per-voice loop can read sends in O(1).
    for (int p = 0; p < mds::kNumPads; ++p)
    {
        currentPadReverbSend[(std::size_t) p] = clamp01(getParamValue(makePadParamId(p, kRevSendSuffix)));
        currentPadDelaySend [(std::size_t) p] = clamp01(getParamValue(makePadParamId(p, kDlySendSuffix)));
    }
    if (reverbSendBuffer.getNumChannels() > 0)
        reverbSendBuffer.clear(0, blockSamples);
    if (reverbSendBuffer.getNumChannels() > 1)
        reverbSendBuffer.clear(1, blockSamples);
    if (delaySendBuffer.getNumChannels() > 0)
        delaySendBuffer.clear(0, blockSamples);
    if (delaySendBuffer.getNumChannels() > 1)
        delaySendBuffer.clear(1, blockSamples);

    triggerBatch.clear();

    for (int i = 0; i < pendingTriggerCount;)
    {
        auto pending = pendingTriggers[static_cast<std::size_t>(i)];
        if (pending.sampleOffset < blockSamples)
        {
            triggerBatch.push_back(pending);
            pendingTriggers[static_cast<std::size_t>(i)] = pendingTriggers[static_cast<std::size_t>(--pendingTriggerCount)];
            pendingTriggers[static_cast<std::size_t>(pendingTriggerCount)] = {};
            continue;
        }

        pendingTriggers[static_cast<std::size_t>(i)].sampleOffset -= blockSamples;
        ++i;
    }

    const auto timingHumanizeMs = juce::jlimit(0.0f, 50.0f, getParamValue(kHumanizeTiming));
    const auto levelHumanize = juce::jlimit(0.0f, 0.2f, getParamValue(kHumanizeLevel));
    const auto sampleRate = static_cast<float>(std::max(1.0, preparedSampleRate));
    const auto jitterMaxSamples = static_cast<int>(std::round((timingHumanizeMs * 0.001f) * sampleRate));
    auto nextHumanizeUnit = [this]() noexcept
    {
        constexpr float denom = static_cast<float>(std::minstd_rand::max() - std::minstd_rand::min());
        return static_cast<float>(humanizeRng() - std::minstd_rand::min()) / denom;
    };
    auto humanizeVelocity = [&](const float baseVelocity) noexcept
    {
        if (levelHumanize <= 0.0001f)
            return juce::jlimit(0.0f, 1.0f, baseVelocity);

        const auto scale = 1.0f + ((nextHumanizeUnit() * 2.0f) - 1.0f) * levelHumanize;
        return juce::jlimit(0.0f, 1.0f, baseVelocity * scale);
    };
    auto humanizeOffset = [&](const int baseOffset) noexcept
    {
        if (jitterMaxSamples <= 0)
            return juce::jmax(0, baseOffset);

        const auto jitter = static_cast<int>(std::round(((nextHumanizeUnit() * 2.0f) - 1.0f) * static_cast<float>(jitterMaxSamples)));
        return juce::jmax(0, baseOffset + jitter);
    };

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            const auto padIndex = mapMidiNoteToPad(msg.getNoteNumber());
            if (padIndex >= 0)
            {
                const auto velocity = humanizeVelocity(applyVelocityCurve(msg.getFloatVelocity(), velocityCurve));
                const auto sampleOffset = humanizeOffset(metadata.samplePosition);
                if (sampleOffset < blockSamples)
                    triggerBatch.push_back({ padIndex, velocity, sampleOffset });
                else
                    enqueuePendingTrigger(padIndex, velocity, sampleOffset - blockSamples);
            }
        }
        else if (msg.isPitchWheel())
        {
            pitchBend.setPitchWheel(msg.getPitchWheelValue());
        }
        else if (msg.isController())
        {
            handleMidiCC(msg.getControllerNumber(), msg.getControllerValue());
        }
    }

    midiMessages.clear();

    // Drain lock-free FIFO from GUI-thread pad triggers
    {
        const auto scope = triggerFifo.read(triggerFifo.getNumReady());
        for (int i = 0; i < scope.blockSize1; ++i)
        {
            const auto& trigger = triggerFifoBuffer[static_cast<std::size_t>(scope.startIndex1 + i)];
            const auto velocity = humanizeVelocity(trigger.velocity);
            const auto sampleOffset = humanizeOffset(0);
            if (sampleOffset < blockSamples)
                triggerBatch.push_back({ trigger.padIndex, velocity, sampleOffset });
            else
                enqueuePendingTrigger(trigger.padIndex, velocity, sampleOffset - blockSamples);
        }
        for (int i = 0; i < scope.blockSize2; ++i)
        {
            const auto& trigger = triggerFifoBuffer[static_cast<std::size_t>(scope.startIndex2 + i)];
            const auto velocity = humanizeVelocity(trigger.velocity);
            const auto sampleOffset = humanizeOffset(0);
            if (sampleOffset < blockSamples)
                triggerBatch.push_back({ trigger.padIndex, velocity, sampleOffset });
            else
                enqueuePendingTrigger(trigger.padIndex, velocity, sampleOffset - blockSamples);
        }
    }

    std::sort(triggerBatch.begin(), triggerBatch.end(), [](const auto& lhs, const auto& rhs)
    {
        return lhs.sampleOffset < rhs.sampleOffset;
    });

    int renderStartSample = 0;
    for (const auto& trigger : triggerBatch)
    {
        const auto boundedOffset = juce::jlimit(0, blockSamples, trigger.sampleOffset);
        if (boundedOffset > renderStartSample)
            renderActiveVoicesForRange(buffer, renderStartSample, boundedOffset - renderStartSample);

        triggerPadNow(trigger.padIndex, trigger.velocity);
        renderStartSample = boundedOffset;
    }

    if (renderStartSample < blockSamples)
        renderActiveVoicesForRange(buffer, renderStartSample, blockSamples - renderStartSample);

    const bool auxPostFx = getParamValue(kAuxPostFx) >= 0.5f;

    if (!auxPostFx)
    {
        // Pre-FX mode: apply safety trim only (default behavior)
        for (int busIndex = 1; busIndex < outputBusCount; ++busIndex)
        {
            auto auxBuffer = getBusBuffer(buffer, false, busIndex);
            if (auxBuffer.getNumChannels() > 0 && auxBuffer.getNumSamples() > 0)
                processAuxBusSafety(auxBuffer);
        }
    }

    // Release finished voices back to pool (swap-and-pop, no heap ops)
    // Also release voices whose fade-out crossfade has completed
    for (int i = 0; i < activeVoiceCount; )
    {
        auto& av = activeVoices[static_cast<std::size_t>(i)];
        const bool voiceFinished = (av.voice == nullptr || !av.voice->isActive());
        const bool fadeComplete  = (av.fadeOutSamples > 0 && av.fadeOutGain <= 0.001f);

        if (voiceFinished || fadeComplete)
        {
            if (av.voice != nullptr)
                voicePool.release(av.voiceModel, av.voice);
            av = activeVoices[static_cast<std::size_t>(--activeVoiceCount)];
            activeVoices[static_cast<std::size_t>(activeVoiceCount)] = {};
        }
        else
        {
            ++i;
        }
    }

    auto mainBuffer = getBusBuffer(buffer, false, 0);
    if (mainBuffer.getNumChannels() > 0 && mainBuffer.getNumSamples() > 0)
    {
        processGlobalTransient(mainBuffer);
        processGlobalSaturator(mainBuffer);
        processGlobalEQ(mainBuffer);
        processGlobalCompressor(mainBuffer);
        processGlobalChorus(mainBuffer);
        processGlobalDelay(mainBuffer);
        processGlobalReverb(mainBuffer);
        // Audit Phase 5 D3: mix the per-pad send returns into the master bus
        // before the output gain stage so they obey the master volume.
        processPadSends(mainBuffer);
        processGlobalLfo(mainBuffer);

        const auto targetOutputGain = juce::Decibels::decibelsToGain(getParamValue(kOutputGain));
        const auto startGain = outputGainSmoother.getCurrentValue();
        outputGainSmoother.setTargetValue(targetOutputGain);
        const auto endGain = outputGainSmoother.skip(mainBuffer.getNumSamples());
        for (int channel = 0; channel < mainBuffer.getNumChannels(); ++channel)
            mainBuffer.applyGainRamp(channel, 0, mainBuffer.getNumSamples(), startGain, endGain);
        processGlobalLimiter(mainBuffer);
        updateOutputMeters(mainBuffer, false);
    }

    // Post-FX mode: route aux buses through the same FX chain
    if (auxPostFx)
    {
        for (int busIndex = 1; busIndex < outputBusCount; ++busIndex)
        {
            auto auxBuffer = getBusBuffer(buffer, false, busIndex);
            if (auxBuffer.getNumChannels() > 0 && auxBuffer.getNumSamples() > 0)
            {
                processGlobalTransient(auxBuffer);
                processGlobalSaturator(auxBuffer);
                processGlobalEQ(auxBuffer);
                processGlobalCompressor(auxBuffer);
                processGlobalChorus(auxBuffer);
                processGlobalDelay(auxBuffer);
                processGlobalReverb(auxBuffer);
                processGlobalLfo(auxBuffer);
                processGlobalLimiter(auxBuffer);
                updateOutputMeters(auxBuffer, true);
            }
        }
    }
    else
    {
        for (int busIndex = 1; busIndex < outputBusCount; ++busIndex)
        {
            auto auxBuffer = getBusBuffer(buffer, false, busIndex);
            if (auxBuffer.getNumChannels() > 0 && auxBuffer.getNumSamples() > 0)
                updateOutputMeters(auxBuffer, true);
        }
    }
}

// =============================================================================
// MIDI CC page helpers
// =============================================================================
const char* DrumSynthAudioProcessor::getCCPageName(int page) const
{
    if (page < 0 || page >= kNumCCPages) return "?";
    return kCCPageNames[page];
}

void DrumSynthAudioProcessor::startMidiLearn(const juce::String& paramId)
{
    if (paramId.isEmpty() || parameters.getParameter(paramId) == nullptr)
        return;

    midiLearnParamId = paramId;
    midiLearnCc.store(-1, std::memory_order_relaxed);
    midiLearnActive.store(true, std::memory_order_relaxed);
}

void DrumSynthAudioProcessor::cancelMidiLearn()
{
    midiLearnActive.store(false, std::memory_order_relaxed);
    midiLearnCc.store(-1, std::memory_order_relaxed);
    midiLearnParamId.clear();
}

void DrumSynthAudioProcessor::clearMidiLearn(const juce::String& paramId)
{
    if (paramId.isEmpty())
        return;

    const juce::ScopedLock midiLock(midiLearnLock);
    for (auto it = midiLearnMap.begin(); it != midiLearnMap.end();)
    {
        if (it->second == paramId)
            it = midiLearnMap.erase(it);
        else
            ++it;
    }
}

void DrumSynthAudioProcessor::clearAllMidiLearn()
{
    const juce::ScopedLock midiLock(midiLearnLock);
    midiLearnMap.clear();
    midiLearnActive.store(false, std::memory_order_relaxed);
    midiLearnCc.store(-1, std::memory_order_relaxed);
    midiLearnParamId.clear();
}

juce::StringArray DrumSynthAudioProcessor::getMidiLearnedParams() const
{
    juce::StringArray params;
    const juce::ScopedLock midiLock(midiLearnLock);
    for (const auto& [cc, paramId] : midiLearnMap)
    {
        juce::ignoreUnused(cc);
        params.addIfNotAlreadyThere(paramId);
    }
    params.sort(true);
    return params;
}

int DrumSynthAudioProcessor::getMidiCcForParam(const juce::String& paramId) const
{
    const juce::ScopedLock midiLock(midiLearnLock);
    for (const auto& [cc, mappedParam] : midiLearnMap)
        if (mappedParam == paramId)
            return cc;
    return -1;
}

juce::StringArray DrumSynthAudioProcessor::getMidiLearnTargetIds() const
{
    juce::StringArray ids;
    for (int i = 0; i < parameters.state.getNumChildren(); ++i)
    {
        const auto child = parameters.state.getChild(i);
        const auto id = child.getProperty("id").toString();
        if (id.isNotEmpty())
            ids.addIfNotAlreadyThere(id);
    }
    ids.sort(true);
    return ids;
}

juce::String DrumSynthAudioProcessor::getParameterDisplayName(const juce::String& paramId) const
{
    if (const auto* parameter = parameters.getParameter(paramId))
        return parameter->getName(64);
    return paramId;
}

DrumSynthAudioProcessor::QualityMode DrumSynthAudioProcessor::getQualityMode() const noexcept
{
    const auto value = static_cast<int>(std::round(getParamValue(kQualityMode)));
    return value <= 0 ? QualityMode::Live : QualityMode::Studio;
}

void DrumSynthAudioProcessor::handleMidiCC(int ccNumber, int ccValue)
{
    const auto normalized = juce::jlimit(0.0f, 1.0f, static_cast<float>(ccValue) / 127.0f);

    if (midiLearnActive.load(std::memory_order_relaxed))
    {
        if (isReservedMidiCc(ccNumber))
            return;

        if (auto* parameter = parameters.getParameter(midiLearnParamId))
        {
            const juce::ScopedLock midiLock(midiLearnLock);
            for (auto it = midiLearnMap.begin(); it != midiLearnMap.end();)
            {
                if (it->second == midiLearnParamId)
                    it = midiLearnMap.erase(it);
                else
                    ++it;
            }
            midiLearnMap[ccNumber] = midiLearnParamId;
            midiLearnCc.store(ccNumber, std::memory_order_relaxed);
            midiLearnActive.store(false, std::memory_order_relaxed);
            parameter->setValueNotifyingHost(normalized);
        }
        return;
    }

    {
        const juce::ScopedLock midiLock(midiLearnLock);
        const auto it = midiLearnMap.find(ccNumber);
        if (it != midiLearnMap.end())
        {
            if (auto* parameter = parameters.getParameter(it->second))
                parameter->setValueNotifyingHost(normalized);
            return;
        }
    }

    // Page navigation
    if (ccNumber == 102) { midiCCPage.store(std::max(0, midiCCPage.load(std::memory_order_relaxed) - 1), std::memory_order_relaxed); return; }
    if (ccNumber == 103) { midiCCPage.store(std::min(kNumCCPages - 1, midiCCPage.load(std::memory_order_relaxed) + 1), std::memory_order_relaxed); return; }
    if (ccNumber >= 44 && ccNumber <= 50) { midiCCPage.store(ccNumber - 44, std::memory_order_relaxed); return; }

    // Knob assignment CC21-28 → slot 0-7
    if (ccNumber >= 21 && ccNumber <= 28)
    {
        const int slot = ccNumber - 21;
        const int page = midiCCPage.load(std::memory_order_relaxed);
        if (page < 0 || page >= kNumCCPages) return;
        const auto& s = kCCPages[page][slot];
        if (s.paramId == nullptr && s.padSuffix == nullptr) return;

        juce::String id;
        if (s.paramId != nullptr)
            id = s.paramId;
        else
            id = makePadParamId(getSelectedPadIndex(), s.padSuffix);

        if (auto* param = parameters.getParameter(id))
        {
            param->setValueNotifyingHost(normalized);
        }
    }
}

juce::AudioProcessorEditor* DrumSynthAudioProcessor::createEditor()
{
    return new DrumSynthAudioProcessorEditor(*this);
}

double DrumSynthAudioProcessor::getTailLengthSeconds() const
{
    double tailSeconds = 0.0;

    if (getParamValue(kFxDelayEn) >= 0.5f && getParamValue(kDelayMix) > 0.0001f)
    {
        const auto delaySeconds = static_cast<double>(juce::jmax(1.0f, getParamValue(kDelayTime))) * 0.001;
        const auto feedback = static_cast<double>(juce::jlimit(0.0f, 0.95f, getParamValue(kDelayFeedback)));
        tailSeconds = std::max(tailSeconds, delaySeconds * (1.0 + feedback * 6.0));
    }

    if (getParamValue(kReverbMix) > 0.0001f)
    {
        const auto size = static_cast<double>(clamp01(getParamValue(kReverbSize)));
        const auto predelay = static_cast<double>(juce::jlimit(0.0f, 100.0f, getParamValue(kReverbPredelay))) * 0.001;
        const auto reverbTail = predelay + 0.20 + size * 12.0;
        tailSeconds = std::max(tailSeconds, reverbTail);
    }

    return juce::jlimit(0.0, 30.0, tailSeconds);
}

int DrumSynthAudioProcessor::getNumPrograms()
{
    return juce::jmax(1, static_cast<int>(factoryPresets.size()));
}

int DrumSynthAudioProcessor::getCurrentProgram()
{
    return juce::jmax(0, currentPresetIndex);
}

void DrumSynthAudioProcessor::setCurrentProgram(const int index)
{
    applyFactoryPreset(index);
}

const juce::String DrumSynthAudioProcessor::getProgramName(const int index)
{
    if (factoryPresets.empty())
        return index == 0 ? "Init" : juce::String{};

    if (index < 0 || index >= static_cast<int>(factoryPresets.size()))
        return {};

    return factoryPresets[static_cast<std::size_t>(index)].name;
}

void DrumSynthAudioProcessor::changeProgramName(const int, const juce::String&)
{
}

void DrumSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    state.setProperty(kPresetIndexProperty, currentPresetIndex, nullptr);
    state.setProperty("preset_version", mds::kPresetVersion, nullptr);
    if (currentUserPresetFile.existsAsFile())
        state.setProperty("user_preset_file", currentUserPresetFile.getFullPathName(), nullptr);
    writePresetMetadataProperties(state, currentPresetEntry);

    for (int pad = 0; pad < mds::kNumPads; ++pad)
        state.setProperty(juce::String(kPadPresetIndexProperty) + juce::String(pad),
                          currentPadPresetIndices[static_cast<std::size_t>(pad)], nullptr);

    if (auto xml = state.createXml())
    {
        {
            const juce::ScopedLock midiLock(midiLearnLock);
            writeMidiLearnXml(*xml, midiLearnMap);
        }
        copyXmlToBinary(*xml, destData);
    }
}

void DrumSynthAudioProcessor::setStateInformation(const void* data, const int sizeInBytes)
{
    const auto xmlState = getXmlFromBinary(data, sizeInBytes);
    if (xmlState == nullptr)
        return;

    if (! xmlState->hasTagName(parameters.state.getType()))
        return;

    auto restoredState = juce::ValueTree::fromXml(*xmlState);
    parameters.replaceState(restoredState);
    const int savedVersion = static_cast<int>(restoredState.getProperty("preset_version", 0));
    applyPresetMigrationDefaults(parameters, savedVersion);
    sanitizeParameterState(parameters);
    {
        const juce::ScopedLock midiLock(midiLearnLock);
        readMidiLearnXml(*xmlState, midiLearnMap);
    }
    midiLearnActive.store(false, std::memory_order_relaxed);
    midiLearnCc.store(-1, std::memory_order_relaxed);
    midiLearnParamId.clear();
    restorePresetMetadata(restoredState, currentUserPresetFile, currentPresetIndex,
                          currentPadPresetIndices, factoryPresets);
    if (currentUserPresetFile.existsAsFile())
        currentPresetEntry = makePresetEntryFromState(restoredState, currentUserPresetFile, factoryPresets);
    else if (currentPresetIndex >= 0 && currentPresetIndex < static_cast<int>(factoryPresets.size()))
        currentPresetEntry = makeFactoryPresetEntry(factoryPresets[static_cast<std::size_t>(currentPresetIndex)], currentPresetIndex);
    else
        currentPresetEntry = {};
}

juce::StringArray DrumSynthAudioProcessor::getFactoryPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : factoryPresets)
        names.add(preset.name);

    return names;
}

void DrumSynthAudioProcessor::applyFactoryPreset(const int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= static_cast<int>(factoryPresets.size()))
        return;

    const auto& preset = factoryPresets[static_cast<std::size_t>(presetIndex)];

    for (int pad = 0; pad < mds::kNumPads; ++pad)
    {
        const auto& settings = preset.pads[static_cast<std::size_t>(pad)];
        setParamValue(makePadParamId(pad, "level"), settings.level);
        setParamValue(makePadParamId(pad, "tune"), settings.tuneSemitones);
        setParamValue(makePadParamId(pad, "decay"), settings.decaySeconds);
        setParamValue(makePadParamId(pad, "attack"), settings.attackSeconds);
        setParamValue(makePadParamId(pad, "pitch_drop"), settings.pitchDropSemitones);
        setParamValue(makePadParamId(pad, "pitch_decay"), settings.pitchDecaySeconds);
        setParamValue(makePadParamId(pad, "noise"), settings.noiseAmount);
        setParamValue(makePadParamId(pad, "click"), settings.clickAmount);
        setParamValue(makePadParamId(pad, "drive"), settings.drive);
        setParamValue(makePadParamId(pad, "cutoff"), settings.cutoffHz);
        setParamValue(makePadParamId(pad, "pan"), settings.pan);
        setParamValue(makePadParamId(pad, kClapSpreadSuffix), settings.clapSpread);
        setParamValue(makePadParamId(pad, kClapDensitySuffix), settings.clapDensity);
        setParamValue(makePadParamId(pad, kMetallicDensitySuffix), settings.metallicDensity);
        setParamValue(makePadParamId(pad, kOpenAmountSuffix), settings.openAmount);
        setParamValue(makePadParamId(pad, kBodyToneSuffix), settings.bodyTone);
        setParamValue(makePadParamId(pad, kModalRingSuffix), settings.modalRing);
        setParamValue(makePadParamId(pad, kFmIndexSuffix), settings.fmIndex);
        setParamValue(makePadParamId(pad, kFmSweepSuffix), settings.fmSweep);
        // Audit Phase 5 D1/D3: factory presets feed the new per-pad params.
        setParamValue(makePadParamId(pad, kVelToClickSuffix), settings.velocityToClick);
        setParamValue(makePadParamId(pad, kRevSendSuffix),    settings.reverbSend);
        setParamValue(makePadParamId(pad, kDlySendSuffix),    settings.delaySend);
        setParamValue(makePadParamId(pad, kPadOutputSuffix),
                      static_cast<float>(preset.outputBuses[static_cast<std::size_t>(pad)]));
    }

    const auto& fx = preset.fx;

    setParamValue(kOutputGain,       fx.outputGainDb);
    setParamValue(kMacroPunch,       fx.macroPunch);
    setParamValue(kMacroWeight,      fx.macroWeight);
    setParamValue(kMacroAir,         fx.macroAir);
    setParamValue(kMacroDirt,        fx.macroDirt);

    setParamValue(kCompThreshold,    fx.compThreshold);
    setParamValue(kCompRatio,        fx.compRatio);
    setParamValue(kCompAttack,       fx.compAttack);
    setParamValue(kCompRelease,      fx.compRelease);
    setParamValue(kCompMakeup,       fx.compMakeup);
    setParamValue(kCompMix,          fx.compMix);

    setParamValue(kSatDrive,         fx.satDrive);
    setParamValue(kSatMix,           fx.satMix);

    setParamValue(kTransientAttack,  fx.transientAttack);
    setParamValue(kTransientSustain, fx.transientSustain);
    setParamValue(kTransientMix,     fx.transientMix);

    // Reverb (Phase 4 params)
    setParamValue(kReverbSize,       fx.reverbSize);
    setParamValue(kReverbDamping,    fx.reverbDamping);
    setParamValue(kReverbWidth,      fx.reverbWidth);
    setParamValue(kReverbMix,        fx.reverbMix);
    setParamValue(kReverbPredelay,   fx.reverbPredelay);

    // EQ
    setParamValue(kFxEqEn,           fx.eqEnable ? 1.0f : 0.0f);
    setParamValue(kEqLowFreq,        fx.eqLowFreq);
    setParamValue(kEqLowGain,        fx.eqLowGain);
    setParamValue(kEqMidFreq,        fx.eqMidFreq);
    setParamValue(kEqMidGain,        fx.eqMidGain);
    setParamValue(kEqMidQ,           fx.eqMidQ);
    setParamValue(kEqHighFreq,       fx.eqHighFreq);
    setParamValue(kEqHighGain,       fx.eqHighGain);

    // Chorus
    setParamValue(kFxChorusEn,       fx.chorusEnable ? 1.0f : 0.0f);
    setParamValue(kChorusRate,       fx.chorusRate);
    setParamValue(kChorusDepth,      fx.chorusDepth);
    setParamValue(kChorusMix,        fx.chorusMix);

    // Delay
    setParamValue(kFxDelayEn,        fx.delayEnable ? 1.0f : 0.0f);
    setParamValue(kDelayTime,        fx.delayTime);
    setParamValue(kDelayFeedback,    fx.delayFeedback);
    setParamValue(kDelayMix,         fx.delayMix);
    setParamValue(kDelaySync,        fx.delaySync ? 1.0f : 0.0f);
    setParamValue(kDelayNoteDiv,     static_cast<float>(fx.delayNoteDiv));

    // Limiter
    setParamValue(kFxLimiterEn,      fx.limiterEnable ? 1.0f : 0.0f);
    setParamValue(kLimiterThreshold, fx.limiterThreshold);
    setParamValue(kLimiterRelease,   fx.limiterRelease);

    // Enable toggles
    setParamValue(kFxReverbEn,       fx.reverbEnable ? 1.0f : 0.0f);
    setParamValue(kFxTransientEn,    fx.transientEnable ? 1.0f : 0.0f);
    setParamValue(kFxSaturatorEn,    fx.saturatorEnable ? 1.0f : 0.0f);
    setParamValue(kFxCompEn,         fx.compEnable ? 1.0f : 0.0f);

    // Velocity curve
    setParamValue(kVelocityCurve,    static_cast<float>(fx.velocityCurve));

    // LFO
    setParamValue(kLfoRate,          fx.lfoRate);
    setParamValue(kLfoDepth,         fx.lfoDepth);
    setParamValue(kLfoWave,          static_cast<float>(fx.lfoWave));

    // Humanize
    setParamValue(kHumanizeTiming,   fx.humanizeTimingMs);
    setParamValue(kHumanizeLevel,    fx.humanizeLevel);

    // Aux routing
    setParamValue(kAuxPostFx,        fx.auxPostFx ? 1.0f : 0.0f);

    currentPresetIndex = presetIndex;
    currentUserPresetFile = juce::File{};
    currentPresetEntry = makeFactoryPresetEntry(preset, presetIndex);
    updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
}

bool DrumSynthAudioProcessor::saveFactoryPreset(const int presetIndex)
{
    if (presetIndex < 0 || presetIndex >= static_cast<int>(factoryPresets.size()))
        return false;

    auto& preset = factoryPresets[static_cast<std::size_t>(presetIndex)];

    // Capture current parameter values back into the mutable preset
    for (int pad = 0; pad < mds::kNumPads; ++pad)
    {
        auto& s = preset.pads[static_cast<std::size_t>(pad)];
        s.level               = getParamValue(makePadParamId(pad, "level"));
        s.tuneSemitones       = getParamValue(makePadParamId(pad, "tune"));
        s.decaySeconds        = getParamValue(makePadParamId(pad, "decay"));
        s.attackSeconds       = getParamValue(makePadParamId(pad, "attack"));
        s.pitchDropSemitones  = getParamValue(makePadParamId(pad, "pitch_drop"));
        s.pitchDecaySeconds   = getParamValue(makePadParamId(pad, "pitch_decay"));
        s.noiseAmount         = getParamValue(makePadParamId(pad, "noise"));
        s.clickAmount         = getParamValue(makePadParamId(pad, "click"));
        s.drive               = getParamValue(makePadParamId(pad, "drive"));
        s.cutoffHz            = getParamValue(makePadParamId(pad, "cutoff"));
        s.pan                 = getParamValue(makePadParamId(pad, "pan"));
        s.clapSpread          = getParamValue(makePadParamId(pad, kClapSpreadSuffix));
        s.clapDensity         = getParamValue(makePadParamId(pad, kClapDensitySuffix));
        s.metallicDensity     = getParamValue(makePadParamId(pad, kMetallicDensitySuffix));
        s.openAmount          = getParamValue(makePadParamId(pad, kOpenAmountSuffix));
        s.bodyTone            = getParamValue(makePadParamId(pad, kBodyToneSuffix));
        s.modalRing           = getParamValue(makePadParamId(pad, kModalRingSuffix));
        s.fmIndex             = getParamValue(makePadParamId(pad, kFmIndexSuffix));
        s.fmSweep             = getParamValue(makePadParamId(pad, kFmSweepSuffix));
        // Audit Phase 5 D1/D3.
        s.velocityToClick     = getParamValue(makePadParamId(pad, kVelToClickSuffix));
        s.reverbSend          = getParamValue(makePadParamId(pad, kRevSendSuffix));
        s.delaySend           = getParamValue(makePadParamId(pad, kDlySendSuffix));
        preset.outputBuses[static_cast<std::size_t>(pad)] = juce::jlimit(
            0, kNumAuxOutputs,
            static_cast<int>(std::round(getParamValue(makePadParamId(pad, kPadOutputSuffix)))));
    }

    auto& fx = preset.fx;
    fx.outputGainDb      = getParamValue(kOutputGain);
    fx.macroPunch        = getParamValue(kMacroPunch);
    fx.macroWeight       = getParamValue(kMacroWeight);
    fx.macroAir          = getParamValue(kMacroAir);
    fx.macroDirt         = getParamValue(kMacroDirt);
    fx.compThreshold     = getParamValue(kCompThreshold);
    fx.compRatio         = getParamValue(kCompRatio);
    fx.compAttack        = getParamValue(kCompAttack);
    fx.compRelease       = getParamValue(kCompRelease);
    fx.compMakeup        = getParamValue(kCompMakeup);
    fx.compMix           = getParamValue(kCompMix);
    fx.satDrive          = getParamValue(kSatDrive);
    fx.satMix            = getParamValue(kSatMix);
    fx.transientAttack   = getParamValue(kTransientAttack);
    fx.transientSustain  = getParamValue(kTransientSustain);
    fx.transientMix      = getParamValue(kTransientMix);
    fx.reverbSize        = getParamValue(kReverbSize);
    fx.reverbDamping     = getParamValue(kReverbDamping);
    fx.reverbWidth       = getParamValue(kReverbWidth);
    fx.reverbMix         = getParamValue(kReverbMix);
    fx.reverbPredelay    = getParamValue(kReverbPredelay);
    fx.eqLowFreq         = getParamValue(kEqLowFreq);
    fx.eqLowGain         = getParamValue(kEqLowGain);
    fx.eqMidFreq         = getParamValue(kEqMidFreq);
    fx.eqMidGain         = getParamValue(kEqMidGain);
    fx.eqMidQ            = getParamValue(kEqMidQ);
    fx.eqHighFreq        = getParamValue(kEqHighFreq);
    fx.eqHighGain        = getParamValue(kEqHighGain);
    fx.eqEnable          = getParamValue(kFxEqEn) >= 0.5f;
    fx.chorusRate        = getParamValue(kChorusRate);
    fx.chorusDepth       = getParamValue(kChorusDepth);
    fx.chorusMix         = getParamValue(kChorusMix);
    fx.chorusEnable      = getParamValue(kFxChorusEn) >= 0.5f;
    fx.delayTime         = getParamValue(kDelayTime);
    fx.delayFeedback     = getParamValue(kDelayFeedback);
    fx.delayMix          = getParamValue(kDelayMix);
    fx.delaySync         = getParamValue(kDelaySync) >= 0.5f;
    fx.delayNoteDiv      = static_cast<int>(std::round(getParamValue(kDelayNoteDiv)));
    fx.delayEnable       = getParamValue(kFxDelayEn) >= 0.5f;
    fx.limiterThreshold  = getParamValue(kLimiterThreshold);
    fx.limiterRelease    = getParamValue(kLimiterRelease);
    fx.limiterEnable     = getParamValue(kFxLimiterEn) >= 0.5f;
    fx.auxPostFx         = getParamValue(kAuxPostFx) >= 0.5f;

    // Persist as XML override file
    auto dir = getFactoryOverridesDirectory();
    auto file = dir.getChildFile(juce::String(presetIndex) + ".xml");

    auto root = std::make_unique<juce::XmlElement>("FactoryPreset");
    root->setAttribute("format_version", kPresetFormatVersion);
    root->setAttribute("name", juce::String(preset.name));
    root->setAttribute("index", presetIndex);
    writeGlobalFxAttributes(*root, fx);

    for (int pad = 0; pad < mds::kNumPads; ++pad)
    {
        const auto& s = preset.pads[static_cast<std::size_t>(pad)];
        auto* padXml = root->createNewChildElement("Pad");
        padXml->setAttribute("index",       pad);
        padXml->setAttribute("level",       static_cast<double>(s.level));
        padXml->setAttribute("tune",        static_cast<double>(s.tuneSemitones));
        padXml->setAttribute("decay",       static_cast<double>(s.decaySeconds));
        padXml->setAttribute("attack",      static_cast<double>(s.attackSeconds));
        padXml->setAttribute("pitchDrop",   static_cast<double>(s.pitchDropSemitones));
        padXml->setAttribute("pitchDecay",  static_cast<double>(s.pitchDecaySeconds));
        padXml->setAttribute("noise",       static_cast<double>(s.noiseAmount));
        padXml->setAttribute("click",       static_cast<double>(s.clickAmount));
        padXml->setAttribute("drive",       static_cast<double>(s.drive));
        padXml->setAttribute("cutoff",      static_cast<double>(s.cutoffHz));
        padXml->setAttribute("pan",         static_cast<double>(s.pan));
        padXml->setAttribute("clapSpread",  static_cast<double>(s.clapSpread));
        padXml->setAttribute("clapDensity", static_cast<double>(s.clapDensity));
        padXml->setAttribute("metallicDensity", static_cast<double>(s.metallicDensity));
        padXml->setAttribute("openAmount",  static_cast<double>(s.openAmount));
        padXml->setAttribute("bodyTone",    static_cast<double>(s.bodyTone));
        padXml->setAttribute("modalRing",   static_cast<double>(s.modalRing));
        padXml->setAttribute("fmIndex",     static_cast<double>(s.fmIndex));
        padXml->setAttribute("fmSweep",     static_cast<double>(s.fmSweep));
        // Audit Phase 5 D1/D3.
        padXml->setAttribute("velToClick",  static_cast<double>(s.velocityToClick));
        padXml->setAttribute("revSend",     static_cast<double>(s.reverbSend));
        padXml->setAttribute("dlySend",     static_cast<double>(s.delaySend));
        padXml->setAttribute("output",      preset.outputBuses[static_cast<std::size_t>(pad)]);
    }

    const bool ok = root->writeTo(file);
    if (ok)
        currentPresetEntry = makeFactoryPresetEntry(preset, presetIndex);
    return ok;
}

void DrumSynthAudioProcessor::loadFactoryOverrides()
{
    auto dir = getFactoryOverridesDirectory();
    if (! dir.isDirectory()) return;

    for (int i = 0; i < static_cast<int>(factoryPresets.size()); ++i)
    {
        auto file = dir.getChildFile(juce::String(i) + ".xml");
        if (! file.existsAsFile()) continue;

        auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr || ! xml->hasTagName("FactoryPreset")) continue;

        auto& preset = factoryPresets[static_cast<std::size_t>(i)];
        readGlobalFxAttributes(*xml, preset.fx);

        for (auto* padXml : xml->getChildWithTagNameIterator("Pad"))
        {
            int padIdx = padXml->getIntAttribute("index", -1);
            if (padIdx < 0 || padIdx >= mds::kNumPads) continue;

            auto& s = preset.pads[static_cast<std::size_t>(padIdx)];
            s.level               = clamp01(static_cast<float>(padXml->getDoubleAttribute("level",      s.level)));
            s.tuneSemitones       = juce::jlimit(-24.0f, 24.0f, static_cast<float>(padXml->getDoubleAttribute("tune",       s.tuneSemitones)));
            s.decaySeconds        = juce::jlimit(0.03f, 2.5f, static_cast<float>(padXml->getDoubleAttribute("decay",      s.decaySeconds)));
            s.attackSeconds       = juce::jlimit(0.0f, 0.05f, static_cast<float>(padXml->getDoubleAttribute("attack",     s.attackSeconds)));
            s.pitchDropSemitones  = juce::jlimit(0.0f, 48.0f, static_cast<float>(padXml->getDoubleAttribute("pitchDrop",  s.pitchDropSemitones)));
            s.pitchDecaySeconds   = juce::jlimit(0.005f, 1.2f, static_cast<float>(padXml->getDoubleAttribute("pitchDecay", s.pitchDecaySeconds)));
            s.noiseAmount         = clamp01(static_cast<float>(padXml->getDoubleAttribute("noise",      s.noiseAmount)));
            s.clickAmount         = clamp01(static_cast<float>(padXml->getDoubleAttribute("click",      s.clickAmount)));
            s.drive               = juce::jlimit(1.0f, 12.0f, static_cast<float>(padXml->getDoubleAttribute("drive",      s.drive)));
            s.cutoffHz            = juce::jlimit(120.0f, 18000.0f, static_cast<float>(padXml->getDoubleAttribute("cutoff",     s.cutoffHz)));
            s.pan                 = juce::jlimit(-1.0f, 1.0f, static_cast<float>(padXml->getDoubleAttribute("pan",        s.pan)));
            s.clapSpread          = clamp01(static_cast<float>(padXml->getDoubleAttribute("clapSpread", s.clapSpread)));
            s.clapDensity         = clamp01(static_cast<float>(padXml->getDoubleAttribute("clapDensity", s.clapDensity)));
            s.metallicDensity     = clamp01(static_cast<float>(padXml->getDoubleAttribute("metallicDensity", s.metallicDensity)));
            s.openAmount          = clamp01(static_cast<float>(padXml->getDoubleAttribute("openAmount", s.openAmount)));
            s.bodyTone            = clamp01(static_cast<float>(padXml->getDoubleAttribute("bodyTone", s.bodyTone)));
            s.modalRing           = clamp01(static_cast<float>(padXml->getDoubleAttribute("modalRing", s.modalRing)));
            s.fmIndex             = clamp01(static_cast<float>(padXml->getDoubleAttribute("fmIndex", s.fmIndex)));
            s.fmSweep             = clamp01(static_cast<float>(padXml->getDoubleAttribute("fmSweep", s.fmSweep)));
            // Audit Phase 5 D1/D3.
            s.velocityToClick     = clamp01(static_cast<float>(padXml->getDoubleAttribute("velToClick", s.velocityToClick)));
            s.reverbSend          = clamp01(static_cast<float>(padXml->getDoubleAttribute("revSend",    s.reverbSend)));
            s.delaySend           = clamp01(static_cast<float>(padXml->getDoubleAttribute("dlySend",    s.delaySend)));
            preset.outputBuses[static_cast<std::size_t>(padIdx)] = juce::jlimit(
                0, kNumAuxOutputs, padXml->getIntAttribute("output", preset.outputBuses[static_cast<std::size_t>(padIdx)]));
        }
    }
}

juce::File DrumSynthAudioProcessor::getFactoryOverridesDirectory()
{
    const auto preferred = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("MusiqueDrumSynth")
                               .getChildFile("FactoryOverrides_StudioKit");
    return findWritableDirectory(preferred, "MusiqueDrumSynth/FactoryOverrides_StudioKit");
}

// =============================================================================
// User Preset Management
// =============================================================================
juce::File DrumSynthAudioProcessor::getUserPresetsDirectory()
{
    const auto preferred = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("MusiqueDrumSynth")
                               .getChildFile("Presets");
    return findWritableDirectory(preferred, "MusiqueDrumSynth/Presets");
}

juce::Array<juce::File> DrumSynthAudioProcessor::scanUserPresets() const
{
    juce::Array<juce::File> results;
    for (const auto& entry : scanPresetLibrary())
    {
        if (!entry.isFactory && entry.presetFile.existsAsFile())
            results.add(entry.presetFile);
    }
    results.sort();
    return results;
}

juce::Array<DrumSynthAudioProcessor::PresetLibraryEntry> DrumSynthAudioProcessor::scanPresetLibrary() const
{
    juce::Array<PresetLibraryEntry> entries;

    for (int i = 0; i < static_cast<int>(factoryPresets.size()); ++i)
        entries.add(makeFactoryPresetEntry(factoryPresets[static_cast<std::size_t>(i)], i));

    juce::Array<juce::File> userPresetFiles;
    auto dir = getUserPresetsDirectory();
    if (dir.isDirectory())
        dir.findChildFiles(userPresetFiles, juce::File::findFiles, false, "*.xml");
    userPresetFiles.sort();

    for (const auto& file : userPresetFiles)
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr || !xml->hasTagName(parameters.state.getType()))
        {
            juce::Logger::writeToLog("[DrumPreset] Ignoring invalid preset file: " + file.getFullPathName());
            continue;
        }

        auto state = juce::ValueTree::fromXml(*xml);
        auto entry = makePresetEntryFromState(state, file, factoryPresets);
        auto manifestEntry = entry;
        const bool manifestLoaded = readPresetManifestFile(entry.manifestFile, manifestEntry);
        if (manifestLoaded)
        {
            if (manifestEntry.name.isNotEmpty()) entry.name = manifestEntry.name;
            if (manifestEntry.familyLabel.isNotEmpty()) entry.familyLabel = manifestEntry.familyLabel;
            if (manifestEntry.mixRole.isNotEmpty()) entry.mixRole = manifestEntry.mixRole;
            if (manifestEntry.description.isNotEmpty()) entry.description = manifestEntry.description;
            if (manifestEntry.outputProfile.isNotEmpty()) entry.outputProfile = manifestEntry.outputProfile;
            if (!manifestEntry.tags.isEmpty()) entry.tags = manifestEntry.tags;
            if (std::isfinite(manifestEntry.nominalPeakDb)) entry.nominalPeakDb = manifestEntry.nominalPeakDb;
        }
        entries.add(entry);
    }

    return entries;
}

void DrumSynthAudioProcessor::backfillMissingPresetManifests() const
{
    juce::Array<juce::File> userPresetFiles;
    auto dir = getUserPresetsDirectory();
    if (!dir.isDirectory())
        return;

    dir.findChildFiles(userPresetFiles, juce::File::findFiles, false, "*.xml");
    userPresetFiles.sort();

    for (const auto& file : userPresetFiles)
    {
        auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr || !xml->hasTagName(parameters.state.getType()))
            continue;

        const auto manifestFile = getPresetManifestFile(file);
        if (manifestFile.existsAsFile())
            continue;

        auto state = juce::ValueTree::fromXml(*xml);
        auto entry = makePresetEntryFromState(state, file, factoryPresets);
        writePresetManifestFile(manifestFile, entry);
    }
}

juce::StringArray DrumSynthAudioProcessor::getPresetFamilyChoices() const
{
    juce::StringArray choices;
    choices.add("All");
    for (const auto& entry : scanPresetLibrary())
        if (entry.familyLabel.isNotEmpty())
            choices.addIfNotAlreadyThere(entry.familyLabel);
    choices.sort(true);
    if (choices[0] != "All")
        choices.move(choices.indexOf("All"), 0);
    return choices;
}

juce::StringArray DrumSynthAudioProcessor::getPresetMixRoleChoices() const
{
    juce::StringArray choices;
    choices.add("All");
    for (const auto& entry : scanPresetLibrary())
        if (entry.mixRole.isNotEmpty())
            choices.addIfNotAlreadyThere(entry.mixRole);
    choices.sort(true);
    if (choices[0] != "All")
        choices.move(choices.indexOf("All"), 0);
    return choices;
}

juce::StringArray DrumSynthAudioProcessor::getPresetTagChoices() const
{
    juce::StringArray choices;
    choices.add("All");
    for (const auto& entry : scanPresetLibrary())
        for (const auto& tag : entry.tags)
            choices.addIfNotAlreadyThere(tag);
    choices.sort(true);
    if (choices[0] != "All")
        choices.move(choices.indexOf("All"), 0);
    return choices;
}

DrumSynthAudioProcessor::PresetLibraryEntry DrumSynthAudioProcessor::getCurrentPresetEntry() const
{
    return currentPresetEntry;
}

bool DrumSynthAudioProcessor::saveUserPreset(const juce::String& name)
{
    if (name.isEmpty())
        return false;

    auto file = getUserPresetsDirectory().getChildFile(
        juce::File::createLegalFileName(name) + ".xml");

    auto state = parameters.copyState();
    auto entry = currentPresetEntry;
    entry.name = name;
    entry.isFactory = false;
    entry.factoryIndex = -1;
    entry.presetFile = file;
    entry.manifestFile = getPresetManifestFile(file);
    if (entry.familyLabel.isEmpty()) entry.familyLabel = "User";
    if (entry.mixRole.isEmpty()) entry.mixRole = "custom";
    if (entry.description.isEmpty()) entry.description = "Preset utilisateur.";
    if (entry.outputProfile.isEmpty()) entry.outputProfile = "master-ready";

    state.setProperty(kUserPresetNameProperty, name, nullptr);
    state.setProperty(kUserPresetFileProperty, file.getFullPathName(), nullptr);
    state.setProperty("preset_version", mds::kPresetVersion, nullptr);
    writePresetMetadataProperties(state, entry);

    if (auto xml = state.createXml())
    {
        if (xml->writeTo(file))
        {
            currentUserPresetFile = file;
            currentPresetIndex = -1;
            currentPresetEntry = entry;
            writePresetManifestFile(entry.manifestFile, entry);
            return true;
        }
    }
    return false;
}

bool DrumSynthAudioProcessor::updateUserPreset(const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    auto state = parameters.copyState();
    auto existingName = file.getFileNameWithoutExtension();
    auto entry = currentPresetEntry;
    entry.name = existingName;
    entry.isFactory = false;
    entry.factoryIndex = -1;
    entry.presetFile = file;
    entry.manifestFile = getPresetManifestFile(file);
    if (entry.familyLabel.isEmpty()) entry.familyLabel = "User";
    if (entry.mixRole.isEmpty()) entry.mixRole = "custom";
    if (entry.description.isEmpty()) entry.description = "Preset utilisateur mis a jour.";
    if (entry.outputProfile.isEmpty()) entry.outputProfile = "master-ready";
    state.setProperty(kUserPresetNameProperty, existingName, nullptr);
    state.setProperty(kUserPresetFileProperty, file.getFullPathName(), nullptr);
    state.setProperty("preset_version", mds::kPresetVersion, nullptr);
    writePresetMetadataProperties(state, entry);

    if (auto xml = state.createXml())
    {
        if (xml->writeTo(file))
        {
            currentUserPresetFile = file;
            currentPresetIndex = -1;
            currentPresetEntry = entry;
            writePresetManifestFile(entry.manifestFile, entry);
            return true;
        }
    }
    return false;
}

bool DrumSynthAudioProcessor::deleteUserPreset(const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    if (currentUserPresetFile == file)
        currentUserPresetFile = juce::File{};

    getPresetManifestFile(file).deleteFile();
    return file.deleteFile();
}

bool DrumSynthAudioProcessor::loadUserPreset(const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return false;

    if (! xml->hasTagName(parameters.state.getType()))
        return false;

    auto restoredState = juce::ValueTree::fromXml(*xml);
    parameters.replaceState(restoredState);
    const int savedVersion = static_cast<int>(restoredState.getProperty("preset_version", 0));
    applyPresetMigrationDefaults(parameters, savedVersion);
    sanitizeParameterState(parameters);
    restorePresetMetadata(restoredState, currentUserPresetFile, currentPresetIndex,
                          currentPadPresetIndices, factoryPresets);

    currentUserPresetFile = file;
    currentPresetIndex = -1;
    currentPresetEntry = makePresetEntryFromState(restoredState, file, factoryPresets);
    readPresetManifestFile(getPresetManifestFile(file), currentPresetEntry);
    writePresetManifestFile(getPresetManifestFile(file), currentPresetEntry);
    updateHostDisplay(juce::AudioProcessor::ChangeDetails().withProgramChanged(true));
    return true;
}

void DrumSynthAudioProcessor::queuePadTrigger(const int padIndex, const float velocity)
{
    if (padIndex < 0 || padIndex >= mds::kNumPads)
        return;

    const auto scope = triggerFifo.write(1);
    if (scope.blockSize1 > 0)
        triggerFifoBuffer[static_cast<std::size_t>(scope.startIndex1)] = { padIndex, juce::jlimit(0.0f, 1.0f, velocity), 0 };
    else if (scope.blockSize2 > 0)
        triggerFifoBuffer[static_cast<std::size_t>(scope.startIndex2)] = { padIndex, juce::jlimit(0.0f, 1.0f, velocity), 0 };
}

float DrumSynthAudioProcessor::consumePadTriggerActivity(int padIndex) noexcept
{
    if (padIndex < 0 || padIndex >= mds::kNumPads)
        return 0.0f;
    // Atomic exchange: hand the latest stamped value to the editor and reset
    // to zero so we only report each trigger once.
    return padTriggerActivity[static_cast<std::size_t>(padIndex)].exchange(0.0f, std::memory_order_relaxed);
}

void DrumSynthAudioProcessor::renderActiveVoicesForRange(juce::AudioBuffer<float>& buffer,
                                                         const int startSample,
                                                         const int numSamples)
{
    if (numSamples <= 0)
        return;

    const auto outputBusCount = getBusCount(false);
    // Audit Phase 5 D3: detect whether any pad currently has a non-zero send.
    // When all sends are zero we can skip the per-voice scratch path entirely
    // (no behavioural change vs. the pre-D3 code; tests rely on this).
    bool anySend = false;
    for (int p = 0; p < mds::kNumPads && ! anySend; ++p)
        anySend = (currentPadReverbSend[(std::size_t) p] > 0.0001f
                || currentPadDelaySend[(std::size_t) p]  > 0.0001f);

    for (int i = 0; i < activeVoiceCount; ++i)
    {
        auto& av = activeVoices[static_cast<std::size_t>(i)];
        if (av.voice == nullptr)
            continue;

        av.voice->setPitchBendFactor(pitchBend.pitchBendFactor);

        auto busIndex = av.outputBus;
        if (busIndex >= outputBusCount || getChannelCountOfBus(false, busIndex) <= 0)
            busIndex = 0;

        auto busBuffer = getBusBuffer(buffer, false, busIndex);

        const float revSend = (av.padIndex >= 0 && av.padIndex < mds::kNumPads)
                                ? currentPadReverbSend[(std::size_t) av.padIndex] : 0.0f;
        const float dlySend = (av.padIndex >= 0 && av.padIndex < mds::kNumPads)
                                ? currentPadDelaySend[(std::size_t) av.padIndex] : 0.0f;
        const bool useScratch = anySend
                                && voiceScratchBuffer.getNumSamples() >= startSample + numSamples
                                && voiceScratchBuffer.getNumChannels() >= busBuffer.getNumChannels();

        if (useScratch)
        {
            // Render into scratch then sum into destination bus (dry) and
            // optionally into the global send buses.
            for (int ch = 0; ch < busBuffer.getNumChannels(); ++ch)
                voiceScratchBuffer.clear(ch, startSample, numSamples);

            av.voice->render(voiceScratchBuffer, startSample, numSamples);

            if (av.fadeOutSamples > 0)
            {
                for (int s = 0; s < numSamples && av.fadeOutSamples > 0; ++s, --av.fadeOutSamples)
                {
                    av.fadeOutGain -= 1.0f / static_cast<float>(
                        static_cast<int>(preparedSampleRate * 0.005));
                    if (av.fadeOutGain < 0.0f)
                        av.fadeOutGain = 0.0f;

                    const auto sampleIndex = startSample + s;
                    for (int ch = 0; ch < busBuffer.getNumChannels(); ++ch)
                        voiceScratchBuffer.getWritePointer(ch)[sampleIndex] *= av.fadeOutGain;
                }
            }

            for (int ch = 0; ch < busBuffer.getNumChannels(); ++ch)
                busBuffer.addFrom(ch, startSample, voiceScratchBuffer, ch, startSample, numSamples);

            // Only tap sends from voices routed to the master bus, so the
            // global wet returns mix into the master output (not aux).
            if (busIndex == 0 && reverbSendBuffer.getNumSamples() >= startSample + numSamples)
            {
                if (revSend > 0.0001f)
                    for (int ch = 0; ch < juce::jmin(2, busBuffer.getNumChannels()); ++ch)
                        reverbSendBuffer.addFrom(ch, startSample, voiceScratchBuffer,
                                                 ch, startSample, numSamples, revSend);
                if (dlySend > 0.0001f)
                    for (int ch = 0; ch < juce::jmin(2, busBuffer.getNumChannels()); ++ch)
                        delaySendBuffer.addFrom(ch, startSample, voiceScratchBuffer,
                                                ch, startSample, numSamples, dlySend);
            }
        }
        else
        {
            av.voice->render(busBuffer, startSample, numSamples);

            if (av.fadeOutSamples > 0)
            {
                for (int s = 0; s < numSamples && av.fadeOutSamples > 0; ++s, --av.fadeOutSamples)
                {
                    av.fadeOutGain -= 1.0f / static_cast<float>(
                        static_cast<int>(preparedSampleRate * 0.005));
                    if (av.fadeOutGain < 0.0f)
                        av.fadeOutGain = 0.0f;

                    const auto sampleIndex = startSample + s;
                    for (int ch = 0; ch < busBuffer.getNumChannels(); ++ch)
                        busBuffer.getWritePointer(ch)[sampleIndex] *= av.fadeOutGain;
                }
            }
        }
    }
}

void DrumSynthAudioProcessor::enqueuePendingTrigger(const int padIndex,
                                                    const float velocity,
                                                    const int sampleOffset)
{
    if (padIndex < 0 || padIndex >= mds::kNumPads || sampleOffset < 0)
        return;

    if (pendingTriggerCount >= static_cast<int>(pendingTriggers.size()))
    {
        triggerBatch.push_back({ padIndex, juce::jlimit(0.0f, 1.0f, velocity), 0 });
        return;
    }

    pendingTriggers[static_cast<std::size_t>(pendingTriggerCount++)] = {
        padIndex,
        juce::jlimit(0.0f, 1.0f, velocity),
        sampleOffset
    };
}

int DrumSynthAudioProcessor::mapMidiNoteToPad(const int midiNote) const
{
    if (isSingleNoteModeEnabled())
        return getSelectedPadIndex();

    constexpr int padRootNote = 36;
    if (midiNote >= padRootNote && midiNote < padRootNote + mds::kNumPads)
        return midiNote - padRootNote;

    if (midiNote >= padRootNote + 12 && midiNote < padRootNote + 24)
        return midiNote - (padRootNote + 12);

    if (midiNote >= 24 && midiNote < 24 + mds::kNumPads)
        return midiNote - 24;

    return -1;
}

bool DrumSynthAudioProcessor::isSingleNoteModeEnabled() const
{
    return getParamValue(kSingleNoteMode) > 0.5f;
}

int DrumSynthAudioProcessor::getSelectedPadIndex() const
{
    return juce::jlimit(0, mds::kNumPads - 1, static_cast<int>(std::round(getParamValue(kSelectedPad))));
}

int DrumSynthAudioProcessor::getPadOutputBus(const int padIndex) const
{
    const auto choice = juce::jlimit(
        0,
        kNumAuxOutputs,
        static_cast<int>(std::round(getParamValue(makePadParamId(padIndex, kPadOutputSuffix)))));

    if (choice <= 0)
        return 0;

    return choice;
}

int DrumSynthAudioProcessor::getChokeGroupForPad(const int padIndex) const
{
    return mds::getPadChokeGroup(padIndex);
}

float DrumSynthAudioProcessor::getParamValue(const juce::String& paramId) const
{
    if (const auto* raw = parameters.getRawParameterValue(paramId))
        return raw->load();

    return 0.0f;
}

mds::PadSettings DrumSynthAudioProcessor::snapshotPadSettings(const int padIndex) const
{
    mds::PadSettings settings;

    settings.level = getParamValue(makePadParamId(padIndex, "level"));
    settings.tuneSemitones = getParamValue(makePadParamId(padIndex, "tune"));
    settings.decaySeconds = getParamValue(makePadParamId(padIndex, "decay"));
    settings.attackSeconds = getParamValue(makePadParamId(padIndex, "attack"));
    settings.pitchDropSemitones = getParamValue(makePadParamId(padIndex, "pitch_drop"));
    settings.pitchDecaySeconds = getParamValue(makePadParamId(padIndex, "pitch_decay"));
    settings.noiseAmount = getParamValue(makePadParamId(padIndex, "noise"));
    settings.clickAmount = getParamValue(makePadParamId(padIndex, "click"));
    settings.drive = getParamValue(makePadParamId(padIndex, "drive"));
    settings.cutoffHz = getParamValue(makePadParamId(padIndex, "cutoff"));
    settings.pan = getParamValue(makePadParamId(padIndex, "pan"));
    settings.clapSpread = getParamValue(makePadParamId(padIndex, kClapSpreadSuffix));
    settings.clapDensity = getParamValue(makePadParamId(padIndex, kClapDensitySuffix));
    settings.metallicDensity = getParamValue(makePadParamId(padIndex, kMetallicDensitySuffix));
    settings.openAmount = getParamValue(makePadParamId(padIndex, kOpenAmountSuffix));
    settings.bodyTone = getParamValue(makePadParamId(padIndex, kBodyToneSuffix));
    settings.modalRing = getParamValue(makePadParamId(padIndex, kModalRingSuffix));
    settings.fmIndex = getParamValue(makePadParamId(padIndex, kFmIndexSuffix));
    settings.fmSweep = getParamValue(makePadParamId(padIndex, kFmSweepSuffix));
    // Audit Phase 5 D1/D3.
    settings.velocityToClick = getParamValue(makePadParamId(padIndex, kVelToClickSuffix));
    settings.reverbSend      = getParamValue(makePadParamId(padIndex, kRevSendSuffix));
    settings.delaySend       = getParamValue(makePadParamId(padIndex, kDlySendSuffix));
    settings.baseFrequencyHz = mds::getPadBaseFrequency(padIndex);
    settings.voiceModel = mds::getPadVoiceModel(padIndex);

    applyPerformanceMacros(padIndex, settings);
    return settings;
}

void DrumSynthAudioProcessor::applyPerformanceMacros(const int padIndex, mds::PadSettings& settings) const
{
    const auto punch = (macroPunchValue - 0.5f) * 2.0f;
    const auto weight = (macroWeightValue - 0.5f) * 2.0f;
    const auto air = (macroAirValue - 0.5f) * 2.0f;
    const auto dirt = (macroDirtValue - 0.5f) * 2.0f;

    const auto isLow = (padIndex <= 1 || padIndex == 8);
    const auto isHat = (padIndex == 4 || padIndex == 5);
    const auto isCrash = (padIndex == 10);
    const auto isHatLike = (isHat || isCrash);
    const auto isPercLike = (padIndex == 6 || padIndex == 7);
    const auto isBody = (padIndex <= 3 || padIndex == 8 || padIndex == 9 || padIndex == 11);
    const auto voiceModel = settings.voiceModel;

    const float clickPunch = isLow ? 0.16f : (isPercLike ? 0.06f : (isHatLike ? 0.04f : 0.10f));
    const float attackPunch = isPercLike ? 0.22f : (isHatLike ? 0.16f : 0.45f);
    const float pitchPunch = (isLow || isPercLike || padIndex == 8 || padIndex == 9) ? 0.38f : 0.10f;

    settings.clickAmount = clamp01(settings.clickAmount + punch * clickPunch);
    settings.attackSeconds = juce::jlimit(0.0f, 0.05f, settings.attackSeconds * (1.0f - punch * attackPunch));
    settings.pitchDropSemitones = juce::jlimit(0.0f, 48.0f, settings.pitchDropSemitones * (1.0f + punch * pitchPunch));

    if (isLow || isBody)
        settings.decaySeconds = juce::jlimit(0.03f, 2.5f, settings.decaySeconds * (1.0f + weight * 0.33f));
    else if (isPercLike)
        settings.decaySeconds = juce::jlimit(0.03f, 1.2f, settings.decaySeconds * (1.0f + weight * 0.12f));

    if (isLow)
    {
        settings.baseFrequencyHz = juce::jlimit(
            20.0f,
            1200.0f,
            settings.baseFrequencyHz * std::pow(2.0f, -weight * 0.28f));
    }

    const float airScale = isHatLike ? 0.45f : (isPercLike ? 0.18f : 0.8f);
    const float cutoffMin = isHatLike ? 3500.0f : (isPercLike ? 500.0f : 120.0f);
    settings.cutoffHz = juce::jlimit(cutoffMin, 18000.0f, settings.cutoffHz * std::pow(2.0f, air * airScale));
    if (isHatLike)
    {
        settings.noiseAmount = juce::jlimit(0.0f, 0.82f, settings.noiseAmount + air * 0.08f);
        settings.openAmount = clamp01(settings.openAmount + air * 0.14f);
    }
    else if (voiceModel == mds::PadVoiceModel::Clap)
    {
        settings.clapSpread = clamp01(settings.clapSpread + air * 0.10f);
    }

    const float dirtDrive = isHatLike ? 0.14f : (isPercLike ? 0.16f : 0.45f);
    const float dirtNoise = isHatLike ? 0.05f : (isPercLike ? 0.04f : 0.12f);
    const float driveMax = isHatLike ? 4.0f : (isPercLike ? 3.5f : 12.0f);
    settings.drive = juce::jlimit(1.0f, driveMax, settings.drive * (1.0f + dirt * dirtDrive));
    settings.noiseAmount = clamp01(settings.noiseAmount + dirt * dirtNoise);
    settings.level = clamp01(settings.level * (1.0f - std::max(0.0f, dirt) * (isHatLike ? 0.05f : 0.09f)));

    if (voiceModel == mds::PadVoiceModel::Clap)
        settings.clapDensity = clamp01(settings.clapDensity + dirt * 0.10f);
    else if (voiceModel == mds::PadVoiceModel::Hat || voiceModel == mds::PadVoiceModel::Crash)
        settings.metallicDensity = clamp01(settings.metallicDensity + dirt * 0.12f);
    else if (voiceModel == mds::PadVoiceModel::PercWood || voiceModel == mds::PadVoiceModel::PercMetal)
        settings.modalRing = clamp01(settings.modalRing + weight * 0.10f);
    else if (voiceModel == mds::PadVoiceModel::Fx)
        settings.fmIndex = clamp01(settings.fmIndex + dirt * 0.12f);
}

void DrumSynthAudioProcessor::triggerPadNow(const int padIndex, const float velocity)
{
    if (padIndex < 0 || padIndex >= mds::kNumPads)
        return;

    if (preparedSampleRate <= 0.0)
        return;

    // Audit Phase 4.4a: stamp pad activity for the editor mini-VU.
    // Stored as max() so concurrent triggers don't lose the loudest hit.
    {
        auto& slot = padTriggerActivity[static_cast<std::size_t>(padIndex)];
        float prev = slot.load(std::memory_order_relaxed);
        const float v = juce::jlimit(0.0f, 1.5f, velocity);
        while (v > prev && !slot.compare_exchange_weak(prev, v, std::memory_order_relaxed)) {}
    }

    const auto voiceModel = mds::getPadVoiceModel(padIndex);
    const auto chokeGroup = getChokeGroupForPad(padIndex);
    const int fadeOutLen = static_cast<int>(preparedSampleRate * 0.005); // 5ms crossfade

    // Choke: fade out voices in the same choke group instead of hard cut
    if (chokeGroup > 0)
    {
        for (int i = 0; i < activeVoiceCount; ++i)
        {
            auto& av = activeVoices[static_cast<std::size_t>(i)];
            if (av.chokeGroup == chokeGroup && av.fadeOutSamples <= 0)
            {
                av.fadeOutSamples = fadeOutLen;
                av.fadeOutGain = 1.0f;
            }
        }
    }

    // Audit Phase 3.3: anti-overlap kick ducking.
    // When a new Kick (model Kick) triggers and a previous kick voice is still
    // ringing with significant amplitude, dim the previous voice by -3 dB
    // (×0.707) instantaneously. Prevents low-end accumulation in fast kick
    // patterns (typical 808 + dub-step / trap rolls). Threshold 0.30 is the
    // amplitude envelope value, not the raw sample peak. No exposed parameter.
    if (voiceModel == mds::PadVoiceModel::Kick)
    {
        constexpr float kAntiOverlapThreshold = 0.30f;
        constexpr float kAntiOverlapDuckGain  = 0.707f;  // -3 dB
        for (int i = 0; i < activeVoiceCount; ++i)
        {
            auto& av = activeVoices[static_cast<std::size_t>(i)];
            if (av.voiceModel == mds::PadVoiceModel::Kick
                && av.fadeOutSamples <= 0
                && av.voice != nullptr
                && av.voice->getCurrentAmplitude() > kAntiOverlapThreshold)
            {
                av.voice->duckAmplitude(kAntiOverlapDuckGain);
            }
        }
    }

    // Voice stealing: fade out the oldest voice if at capacity
    if (activeVoiceCount >= mds::kMaxActiveVoices)
    {
        int oldestIdx = 0;
        uint64_t oldestAge = activeVoices[0].activationAge;
        for (int i = 1; i < activeVoiceCount; ++i)
        {
            if (activeVoices[static_cast<std::size_t>(i)].activationAge < oldestAge)
            {
                oldestAge = activeVoices[static_cast<std::size_t>(i)].activationAge;
                oldestIdx = i;
            }
        }
        auto& stolen = activeVoices[static_cast<std::size_t>(oldestIdx)];
        if (stolen.fadeOutSamples <= 0)
        {
            stolen.fadeOutSamples = fadeOutLen;
            stolen.fadeOutGain = 1.0f;
        }
        // If already fading, just release it immediately to make room
        else
        {
            voicePool.release(stolen.voiceModel, stolen.voice);
            stolen = activeVoices[static_cast<std::size_t>(--activeVoiceCount)];
            activeVoices[static_cast<std::size_t>(activeVoiceCount)] = {};
        }
    }

    auto* voice = voicePool.acquire(voiceModel);
    if (voice == nullptr)
        return;

    auto& slot = activeVoices[static_cast<std::size_t>(activeVoiceCount++)];
    slot.voice = voice;
    slot.voiceModel = voiceModel;
    slot.padIndex = padIndex;
    slot.chokeGroup = chokeGroup;
    slot.outputBus = getPadOutputBus(padIndex);
    slot.activationAge = ++voiceAgeCounter;

    const auto velocitySeed = static_cast<juce::int64>(std::llround(juce::jlimit(0.0f, 1.0f, velocity) * 1000.0f));
    const auto seed = static_cast<juce::int64>(0x4D445356u)
        ^ (static_cast<juce::int64>(padIndex + 1) * 0x9E3779B9LL)
        ^ (static_cast<juce::int64>(slot.outputBus + 1) * 0x85EBCA6BLL)
        ^ (static_cast<juce::int64>(slot.activationAge) * 0xC2B2AE35LL)
        ^ velocitySeed;
    voice->setDeterministicSeed(seed);
    voice->start(snapshotPadSettings(padIndex), velocity, preparedSampleRate);
}

void DrumSynthAudioProcessor::setParamValue(const juce::String& paramId, const float value)
{
    if (auto* parameter = parameters.getParameter(paramId))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void DrumSynthAudioProcessor::updateGlobalEffectParameters()
{
    const auto threshold = getParamValue(kCompThreshold);
    const auto ratio     = getParamValue(kCompRatio);
    const auto attack    = getParamValue(kCompAttack);
    const auto release   = getParamValue(kCompRelease);

    if (threshold != compCache.threshold) { compressor.setThreshold(threshold); compCache.threshold = threshold; }
    if (ratio     != compCache.ratio)     { compressor.setRatio(ratio);          compCache.ratio     = ratio; }
    if (attack    != compCache.attack)    { compressor.setAttack(attack);         compCache.attack    = attack; }
    if (release   != compCache.release)   { compressor.setRelease(release);       compCache.release   = release; }
}

void DrumSynthAudioProcessor::processGlobalTransient(juce::AudioBuffer<float>& mainBuffer)
{
    if (getParamValue(kFxTransientEn) < 0.5f)
        return;

    transientMixSmoother.setTargetValue(clamp01(getParamValue(kTransientMix)));
    transientAttackSmoother.setTargetValue(juce::jlimit(-1.0f, 1.0f, getParamValue(kTransientAttack)));
    transientSustainSmoother.setTargetValue(juce::jlimit(-1.0f, 1.0f, getParamValue(kTransientSustain)));

    const auto mix = transientMixSmoother.getTargetValue();
    const auto attack = transientAttackSmoother.getTargetValue();
    const auto sustain = transientSustainSmoother.getTargetValue();

    if (mix <= 0.0001f || (std::abs(attack) <= 0.0001f && std::abs(sustain) <= 0.0001f))
        return;

    const auto sampleRate = static_cast<float>(std::max(1.0, preparedSampleRate));
    const auto fastCoeff = std::exp(-1.0f / (0.0018f * sampleRate));
    const auto slowCoeff = std::exp(-1.0f / (0.055f * sampleRate));
    const auto numChannels = juce::jmin(2, mainBuffer.getNumChannels());
    float* channelData[2] = {
        numChannels > 0 ? mainBuffer.getWritePointer(0) : nullptr,
        numChannels > 1 ? mainBuffer.getWritePointer(1) : nullptr
    };

    for (int i = 0; i < mainBuffer.getNumSamples(); ++i)
    {
        const auto smoothedMix = transientMixSmoother.getNextValue();
        const auto smoothedAttack = transientAttackSmoother.getNextValue();
        const auto smoothedSustain = transientSustainSmoother.getNextValue();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto& fast = transientFastEnv[static_cast<std::size_t>(channel)];
            auto& slow = transientSlowEnv[static_cast<std::size_t>(channel)];
            const auto dry = channelData[channel][i];
            const auto absSample = std::abs(dry);

            fast = fastCoeff * fast + (1.0f - fastCoeff) * absSample;
            slow = slowCoeff * slow + (1.0f - slowCoeff) * absSample;

            const auto transient = fast - slow;
            const auto transientPos = std::max(0.0f, transient);
            const auto transientNeg = std::max(0.0f, -transient);
            const auto gain = juce::jlimit(0.2f, 4.0f, 1.0f + smoothedAttack * transientPos * 7.0f + smoothedSustain * transientNeg * 5.0f);

            const auto wet = dry * gain;
            channelData[channel][i] = dry + (wet - dry) * smoothedMix;
        }
    }
}

void DrumSynthAudioProcessor::processGlobalSaturator(juce::AudioBuffer<float>& mainBuffer)
{
    if (getParamValue(kFxSaturatorEn) < 0.5f)
        return;

    satMixSmoother.setTargetValue(clamp01(getParamValue(kSatMix)));
    satDriveSmoother.setTargetValue(juce::jlimit(1.0f, 16.0f, getParamValue(kSatDrive)));
    const auto mix = satMixSmoother.getTargetValue();
    if (mix <= 0.0001f)
        return;
    const bool studioQuality = getQualityMode() == QualityMode::Studio;
    const auto numChannels = juce::jmin(2, mainBuffer.getNumChannels());
    float* channelData[2] = {
        numChannels > 0 ? mainBuffer.getWritePointer(0) : nullptr,
        numChannels > 1 ? mainBuffer.getWritePointer(1) : nullptr
    };

    for (int i = 0; i < mainBuffer.getNumSamples(); ++i)
    {
        const auto smoothedDrive = satDriveSmoother.getNextValue();
        const auto smoothedMix = satMixSmoother.getNextValue();
        const auto normalizer = 1.0f / std::max(0.0001f, std::tanh(smoothedDrive));

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto& prevSample = saturatorPrevSample[static_cast<std::size_t>(channel)];
            const auto dry = channelData[channel][i];
            float wet = std::tanh(dry * smoothedDrive) * normalizer;

            if (studioQuality)
            {
                wet = 0.0f;
                for (int os = 1; os <= 4; ++os)
                {
                    const auto frac = static_cast<float>(os) * 0.25f;
                    const auto interp = prevSample + (dry - prevSample) * frac;
                    wet += std::tanh(interp * smoothedDrive) * normalizer;
                }
                wet *= 0.25f;
            }

            channelData[channel][i] = dry + (wet - dry) * smoothedMix;
            prevSample = dry;
        }
    }
}

void DrumSynthAudioProcessor::processGlobalCompressor(juce::AudioBuffer<float>& mainBuffer)
{
    if (getParamValue(kFxCompEn) < 0.5f)
        return;

    const auto mix = clamp01(getParamValue(kCompMix));
    const auto makeupGain = juce::Decibels::decibelsToGain(getParamValue(kCompMakeup));

    if (mix <= 0.0001f && std::abs(makeupGain - 1.0f) <= 0.0001f)
        return;

    updateGlobalEffectParameters();

    fxDryBuffer.makeCopyOf(mainBuffer, true);

    juce::dsp::AudioBlock<float> block(mainBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);

    mainBuffer.applyGain(makeupGain);

    if (mix < 0.9999f)
    {
        for (int channel = 0; channel < mainBuffer.getNumChannels(); ++channel)
        {
            auto* wet = mainBuffer.getWritePointer(channel);
            const auto* dry = fxDryBuffer.getReadPointer(channel);

            for (int i = 0; i < mainBuffer.getNumSamples(); ++i)
                wet[i] = dry[i] + (wet[i] - dry[i]) * mix;
        }
    }
}

// =============================================================================
// Per-pad preset management
// =============================================================================
juce::StringArray DrumSynthAudioProcessor::getFactoryPadPresetNames(const int padIndex) const
{
    juce::StringArray names;
    for (const auto& p : mds::getFactoryPadPresets(padIndex))
        names.add(p.name);
    return names;
}

int DrumSynthAudioProcessor::getCurrentPadPresetIndex(const int padIndex) const noexcept
{
    if (padIndex < 0 || padIndex >= mds::kNumPads) return -1;
    return currentPadPresetIndices[static_cast<std::size_t>(padIndex)];
}

void DrumSynthAudioProcessor::applyFactoryPadPreset(const int padIndex, const int presetIndex)
{
    if (padIndex < 0 || padIndex >= mds::kNumPads) return;
    const auto& presets = mds::getFactoryPadPresets(padIndex);
    if (presetIndex < 0 || presetIndex >= static_cast<int>(presets.size())) return;

    const auto& s = presets[static_cast<std::size_t>(presetIndex)].settings;
    setParamValue(makePadParamId(padIndex, "level"),       s.level);
    setParamValue(makePadParamId(padIndex, "tune"),        s.tuneSemitones);
    setParamValue(makePadParamId(padIndex, "decay"),       s.decaySeconds);
    setParamValue(makePadParamId(padIndex, "attack"),      s.attackSeconds);
    setParamValue(makePadParamId(padIndex, "pitch_drop"),  s.pitchDropSemitones);
    setParamValue(makePadParamId(padIndex, "pitch_decay"), s.pitchDecaySeconds);
    setParamValue(makePadParamId(padIndex, "noise"),       s.noiseAmount);
    setParamValue(makePadParamId(padIndex, "click"),       s.clickAmount);
    setParamValue(makePadParamId(padIndex, "drive"),       s.drive);
    setParamValue(makePadParamId(padIndex, "cutoff"),      s.cutoffHz);
    setParamValue(makePadParamId(padIndex, "pan"),         s.pan);
    setParamValue(makePadParamId(padIndex, kClapSpreadSuffix), s.clapSpread);
    setParamValue(makePadParamId(padIndex, kClapDensitySuffix), s.clapDensity);
    setParamValue(makePadParamId(padIndex, kMetallicDensitySuffix), s.metallicDensity);
    setParamValue(makePadParamId(padIndex, kOpenAmountSuffix), s.openAmount);
    setParamValue(makePadParamId(padIndex, kBodyToneSuffix), s.bodyTone);
    setParamValue(makePadParamId(padIndex, kModalRingSuffix), s.modalRing);
    setParamValue(makePadParamId(padIndex, kFmIndexSuffix), s.fmIndex);
    setParamValue(makePadParamId(padIndex, kFmSweepSuffix), s.fmSweep);
    // Audit Phase 5 D1/D3.
    setParamValue(makePadParamId(padIndex, kVelToClickSuffix), s.velocityToClick);
    setParamValue(makePadParamId(padIndex, kRevSendSuffix),    s.reverbSend);
    setParamValue(makePadParamId(padIndex, kDlySendSuffix),    s.delaySend);
    currentPadPresetIndices[static_cast<std::size_t>(padIndex)] = presetIndex;
}

juce::File DrumSynthAudioProcessor::getUserPadPresetsDirectory(const int padIndex)
{
    const auto preferred = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("MusiqueDrumSynth")
                               .getChildFile("PadPresets")
                               .getChildFile(juce::String(padIndex));
    return findWritableDirectory(preferred,
                                 "MusiqueDrumSynth/PadPresets/" + juce::String(padIndex));
}

juce::Array<juce::File> DrumSynthAudioProcessor::scanUserPadPresets(const int padIndex) const
{
    juce::Array<juce::File> results;
    auto dir = getUserPadPresetsDirectory(padIndex);
    if (dir.isDirectory())
        dir.findChildFiles(results, juce::File::findFiles, false, "*.xml");
    results.sort();
    return results;
}

bool DrumSynthAudioProcessor::saveUserPadPreset(const int padIndex, const juce::String& name)
{
    if (padIndex < 0 || padIndex >= mds::kNumPads || name.isEmpty()) return false;

    auto file = getUserPadPresetsDirectory(padIndex)
                    .getChildFile(juce::File::createLegalFileName(name) + ".xml");

    auto* root = new juce::XmlElement("PadPreset");
    root->setAttribute("format_version", kPresetFormatVersion);
    root->setAttribute("name",       name);
    root->setAttribute("pad",        padIndex);
    root->setAttribute("level",      static_cast<double>(getParamValue(makePadParamId(padIndex, "level"))));
    root->setAttribute("tune",       static_cast<double>(getParamValue(makePadParamId(padIndex, "tune"))));
    root->setAttribute("decay",      static_cast<double>(getParamValue(makePadParamId(padIndex, "decay"))));
    root->setAttribute("attack",     static_cast<double>(getParamValue(makePadParamId(padIndex, "attack"))));
    root->setAttribute("pitchDrop",  static_cast<double>(getParamValue(makePadParamId(padIndex, "pitch_drop"))));
    root->setAttribute("pitchDecay", static_cast<double>(getParamValue(makePadParamId(padIndex, "pitch_decay"))));
    root->setAttribute("noise",      static_cast<double>(getParamValue(makePadParamId(padIndex, "noise"))));
    root->setAttribute("click",      static_cast<double>(getParamValue(makePadParamId(padIndex, "click"))));
    root->setAttribute("drive",      static_cast<double>(getParamValue(makePadParamId(padIndex, "drive"))));
    root->setAttribute("cutoff",     static_cast<double>(getParamValue(makePadParamId(padIndex, "cutoff"))));
    root->setAttribute("pan",        static_cast<double>(getParamValue(makePadParamId(padIndex, "pan"))));
    root->setAttribute("clapSpread", static_cast<double>(getParamValue(makePadParamId(padIndex, kClapSpreadSuffix))));
    root->setAttribute("clapDensity", static_cast<double>(getParamValue(makePadParamId(padIndex, kClapDensitySuffix))));
    root->setAttribute("metallicDensity", static_cast<double>(getParamValue(makePadParamId(padIndex, kMetallicDensitySuffix))));
    root->setAttribute("openAmount", static_cast<double>(getParamValue(makePadParamId(padIndex, kOpenAmountSuffix))));
    root->setAttribute("bodyTone", static_cast<double>(getParamValue(makePadParamId(padIndex, kBodyToneSuffix))));
    root->setAttribute("modalRing", static_cast<double>(getParamValue(makePadParamId(padIndex, kModalRingSuffix))));
    root->setAttribute("fmIndex", static_cast<double>(getParamValue(makePadParamId(padIndex, kFmIndexSuffix))));
    root->setAttribute("fmSweep", static_cast<double>(getParamValue(makePadParamId(padIndex, kFmSweepSuffix))));
    // Audit Phase 5 D1/D3.
    root->setAttribute("velToClick", static_cast<double>(getParamValue(makePadParamId(padIndex, kVelToClickSuffix))));
    root->setAttribute("revSend",    static_cast<double>(getParamValue(makePadParamId(padIndex, kRevSendSuffix))));
    root->setAttribute("dlySend",    static_cast<double>(getParamValue(makePadParamId(padIndex, kDlySendSuffix))));

    std::unique_ptr<juce::XmlElement> xml(root);
    return xml->writeTo(file);
}

bool DrumSynthAudioProcessor::loadUserPadPreset(const int padIndex, const juce::File& file)
{
    if (padIndex < 0 || padIndex >= mds::kNumPads || ! file.existsAsFile()) return false;

    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || ! xml->hasTagName("PadPreset")) return false;

    int warningCount = 0;
    auto getF = [&](const char* attr, float def, float minValue, float maxValue) {
        return readValidatedXmlFloat(*xml, attr, def, minValue, maxValue, warningCount);
    };

    setParamValue(makePadParamId(padIndex, "level"),       clamp01(getF("level",      0.8f,    0.0f,    1.0f)));
    setParamValue(makePadParamId(padIndex, "tune"),        getF("tune",       0.0f,  -24.0f,   24.0f));
    setParamValue(makePadParamId(padIndex, "decay"),       getF("decay",      0.35f,   0.03f,   2.5f));
    setParamValue(makePadParamId(padIndex, "attack"),      getF("attack",     0.001f,  0.0f,    0.05f));
    setParamValue(makePadParamId(padIndex, "pitch_drop"),  getF("pitchDrop",  0.0f,    0.0f,   48.0f));
    setParamValue(makePadParamId(padIndex, "pitch_decay"), getF("pitchDecay", 0.06f,   0.005f,  1.2f));
    setParamValue(makePadParamId(padIndex, "noise"),       clamp01(getF("noise",      0.2f,    0.0f,    1.0f)));
    setParamValue(makePadParamId(padIndex, "click"),       clamp01(getF("click",      0.1f,    0.0f,    1.0f)));
    setParamValue(makePadParamId(padIndex, "drive"),       getF("drive",      1.0f,    1.0f,   12.0f));
    setParamValue(makePadParamId(padIndex, "cutoff"),      getF("cutoff",   9000.0f, 120.0f, 18000.0f));
    setParamValue(makePadParamId(padIndex, "pan"),         getF("pan",        0.0f,   -1.0f,    1.0f));
    setParamValue(makePadParamId(padIndex, kClapSpreadSuffix), clamp01(getF("clapSpread", 0.5f, 0.0f, 1.0f)));
    setParamValue(makePadParamId(padIndex, kClapDensitySuffix), clamp01(getF("clapDensity", 0.5f, 0.0f, 1.0f)));
    setParamValue(makePadParamId(padIndex, kMetallicDensitySuffix), clamp01(getF("metallicDensity", 0.5f, 0.0f, 1.0f)));
    setParamValue(makePadParamId(padIndex, kOpenAmountSuffix), clamp01(getF("openAmount", 0.5f, 0.0f, 1.0f)));
    setParamValue(makePadParamId(padIndex, kBodyToneSuffix), clamp01(getF("bodyTone", 0.5f, 0.0f, 1.0f)));
    setParamValue(makePadParamId(padIndex, kModalRingSuffix), clamp01(getF("modalRing", 0.5f, 0.0f, 1.0f)));
    setParamValue(makePadParamId(padIndex, kFmIndexSuffix), clamp01(getF("fmIndex", 0.5f, 0.0f, 1.0f)));
    setParamValue(makePadParamId(padIndex, kFmSweepSuffix), clamp01(getF("fmSweep", 0.5f, 0.0f, 1.0f)));
    // Audit Phase 5 D1/D3.
    setParamValue(makePadParamId(padIndex, kVelToClickSuffix), clamp01(getF("velToClick", 0.6f, 0.0f, 1.0f)));
    setParamValue(makePadParamId(padIndex, kRevSendSuffix),    clamp01(getF("revSend",    0.0f, 0.0f, 1.0f)));
    setParamValue(makePadParamId(padIndex, kDlySendSuffix),    clamp01(getF("dlySend",    0.0f, 0.0f, 1.0f)));
    if (warningCount > 0)
        juce::Logger::writeToLog("[DrumPadPreset] Sanitization warnings=" + juce::String(warningCount));
    currentPadPresetIndices[static_cast<std::size_t>(padIndex)] = -1;
    return true;
}

bool DrumSynthAudioProcessor::deleteUserPadPreset(const int padIndex, const juce::File& file)
{
    if (! file.existsAsFile()) return false;
    (void) padIndex;
    return file.deleteFile();
}

// =============================================================================
// Global reverb (DattorroPlateReverb replaces juce::Reverb)
// =============================================================================
void DrumSynthAudioProcessor::processGlobalReverb(juce::AudioBuffer<float>& mainBuffer)
{
    if (getParamValue(kFxReverbEn) < 0.5f)
        return;

    const auto mix = clamp01(getParamValue(kReverbMix));
    if (mix <= 0.0001f)
        return;

    mds::fx::DattorroPlateReverb::Params rp;
    rp.decay      = clamp01(getParamValue(kReverbSize));
    rp.damping    = clamp01(getParamValue(kReverbDamping));
    rp.width      = clamp01(getParamValue(kReverbWidth));
    rp.mix        = mix;
    rp.preDelayMs = juce::jlimit(0.0f, 100.0f, getParamValue(kReverbPredelay));

    const auto numSamples = mainBuffer.getNumSamples();
    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() >= 2 ? mainBuffer.getWritePointer(1) : nullptr;

    dattorroReverb.process(left, right, numSamples, rp);
}

// =============================================================================
// Global EQ (ParametricEQ3Band)
// =============================================================================
void DrumSynthAudioProcessor::processGlobalEQ(juce::AudioBuffer<float>& mainBuffer)
{
    if (getParamValue(kFxEqEn) < 0.5f)
        return;

    mds::fx::ParametricEQ3Band::Params ep;
    ep.lowFreq   = getParamValue(kEqLowFreq);
    ep.lowGainDb = getParamValue(kEqLowGain);
    ep.midFreq   = getParamValue(kEqMidFreq);
    ep.midGainDb = getParamValue(kEqMidGain);
    ep.midQ      = getParamValue(kEqMidQ);
    ep.highFreq  = getParamValue(kEqHighFreq);
    ep.highGainDb= getParamValue(kEqHighGain);

    const auto numSamples = mainBuffer.getNumSamples();
    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() >= 2 ? mainBuffer.getWritePointer(1) : nullptr;

    fxEq.process(left, right, numSamples, ep);
}

// =============================================================================
// Audit Phase 5 D3: per-pad FX sends
// -----------------------------------------------------------------------------
// renderActiveVoicesForRange has accumulated each voice's contribution into
// reverbSendBuffer / delaySendBuffer, scaled by the pad's send amount. Here we
// apply a wet-only pass through the dedicated send instances and sum the
// result back into the master buffer. When all sends are zero (or the matching
// global FX is disabled), the buses are silent and we early-out.
// =============================================================================
void DrumSynthAudioProcessor::processPadSends(juce::AudioBuffer<float>& mainBuffer)
{
    const int numSamples = mainBuffer.getNumSamples();
    if (numSamples <= 0)
        return;

    // ---- Reverb send return ----
    if (reverbSendBuffer.getNumChannels() > 0
        && reverbSendBuffer.getNumSamples() >= numSamples
        && getParamValue(kFxReverbEn) >= 0.5f)
    {
        // Cheap silence detection so we skip when no pad is sending.
        const float magL = reverbSendBuffer.getMagnitude(0, 0, numSamples);
        const float magR = reverbSendBuffer.getNumChannels() > 1
                           ? reverbSendBuffer.getMagnitude(1, 0, numSamples) : 0.0f;
        if (magL > 1.0e-5f || magR > 1.0e-5f)
        {
            mds::fx::DattorroPlateReverb::Params rp;
            rp.decay      = clamp01(getParamValue(kReverbSize));
            rp.damping    = clamp01(getParamValue(kReverbDamping));
            rp.width      = clamp01(getParamValue(kReverbWidth));
            rp.mix        = 1.0f; // pure wet — dry is already in mainBuffer
            rp.preDelayMs = juce::jlimit(0.0f, 100.0f, getParamValue(kReverbPredelay));

            auto* l = reverbSendBuffer.getWritePointer(0);
            auto* r = reverbSendBuffer.getNumChannels() >= 2 ? reverbSendBuffer.getWritePointer(1) : nullptr;
            sendReverb.process(l, r, numSamples, rp);

            for (int ch = 0; ch < juce::jmin(2, mainBuffer.getNumChannels()); ++ch)
                mainBuffer.addFrom(ch, 0, reverbSendBuffer, ch, 0, numSamples);
        }
    }

    // ---- Delay send return ----
    if (delaySendBuffer.getNumChannels() > 0
        && delaySendBuffer.getNumSamples() >= numSamples
        && getParamValue(kFxDelayEn) >= 0.5f)
    {
        const float magL = delaySendBuffer.getMagnitude(0, 0, numSamples);
        const float magR = delaySendBuffer.getNumChannels() > 1
                           ? delaySendBuffer.getMagnitude(1, 0, numSamples) : 0.0f;
        if (magL > 1.0e-5f || magR > 1.0e-5f)
        {
            mds::fx::StereoDelay::Params dp;
            dp.timeMs    = getParamValue(kDelayTime);
            dp.feedback  = getParamValue(kDelayFeedback);
            dp.mix       = 1.0f; // pure wet
            dp.syncToBpm = getParamValue(kDelaySync) >= 0.5f;
            if (dp.syncToBpm)
            {
                if (auto* ph = getPlayHead())
                {
                    auto pos = ph->getPosition();
                    if (pos.hasValue())
                    {
                        auto bpm = pos->getBpm();
                        if (bpm.hasValue())
                            dp.bpm = static_cast<float>(*bpm);
                    }
                }
                dp.noteDiv = static_cast<int>(getParamValue(kDelayNoteDiv));
            }

            auto* l = delaySendBuffer.getWritePointer(0);
            auto* r = delaySendBuffer.getNumChannels() >= 2 ? delaySendBuffer.getWritePointer(1) : nullptr;
            sendDelay.process(l, r, numSamples, dp);

            for (int ch = 0; ch < juce::jmin(2, mainBuffer.getNumChannels()); ++ch)
                mainBuffer.addFrom(ch, 0, delaySendBuffer, ch, 0, numSamples);
        }
    }
}

// =============================================================================
// Global Chorus (StereoChorus)
// =============================================================================
void DrumSynthAudioProcessor::processGlobalChorus(juce::AudioBuffer<float>& mainBuffer)
{
    if (getParamValue(kFxChorusEn) < 0.5f)
        return;

    mds::fx::StereoChorus::Params cp;
    cp.rateHz = getParamValue(kChorusRate);
    cp.depth  = getParamValue(kChorusDepth);
    cp.mix    = clamp01(getParamValue(kChorusMix));

    if (cp.mix <= 0.0001f)
        return;

    const auto numSamples = mainBuffer.getNumSamples();
    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() >= 2 ? mainBuffer.getWritePointer(1) : nullptr;

    fxChorus.process(left, right, numSamples, cp);
}

// =============================================================================
// Global Delay (StereoDelay with BPM sync)
// =============================================================================
void DrumSynthAudioProcessor::processGlobalDelay(juce::AudioBuffer<float>& mainBuffer)
{
    if (getParamValue(kFxDelayEn) < 0.5f)
        return;

    mds::fx::StereoDelay::Params dp;
    dp.timeMs   = getParamValue(kDelayTime);
    dp.feedback = getParamValue(kDelayFeedback);
    dp.mix      = clamp01(getParamValue(kDelayMix));

    if (dp.mix <= 0.0001f)
        return;

    dp.syncToBpm = getParamValue(kDelaySync) >= 0.5f;
    if (dp.syncToBpm)
    {
        auto* audioPlayHead = getPlayHead();
        if (audioPlayHead != nullptr)
        {
            auto posInfo = audioPlayHead->getPosition();
            if (posInfo.hasValue())
            {
                auto bpmOpt = posInfo->getBpm();
                if (bpmOpt.hasValue())
                {
                    dp.bpm = static_cast<float>(*bpmOpt);
                    lastHostBpm.store(dp.bpm, std::memory_order_relaxed);
                    delaySyncActive.store(true, std::memory_order_relaxed);
                }
            }
        }
        dp.noteDiv = static_cast<int>(getParamValue(kDelayNoteDiv));
    }

    const auto numSamples = mainBuffer.getNumSamples();
    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() >= 2 ? mainBuffer.getWritePointer(1) : nullptr;

    fxDelay.process(left, right, numSamples, dp);
}

// =============================================================================
// Global LFO (Tremolo / Auto-pan)
// =============================================================================
void DrumSynthAudioProcessor::processGlobalLfo(juce::AudioBuffer<float>& mainBuffer)
{
    const float depth = clamp01(getParamValue(kLfoDepth));
    if (depth <= 0.0001f) return;

    const float rateHz = getParamValue(kLfoRate);
    const int   wave   = static_cast<int>(std::round(getParamValue(kLfoWave)));
    const int   numCh  = mainBuffer.getNumChannels();
    const int   numSamples = mainBuffer.getNumSamples();
    if (numCh <= 0 || numSamples <= 0) return;

    const float phaseInc = rateHz / static_cast<float>(juce::jmax(1.0, preparedSampleRate));
    constexpr float kTremDepth = 0.65f;
    constexpr float kPanDepth  = 0.50f;

    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = numCh > 1 ? mainBuffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        float lfo = 0.0f;
        switch (wave)
        {
            case 1:  lfo = 1.0f - 4.0f * std::abs(lfoPhase - 0.5f); break;   // triangle
            case 2:  lfo = lfoPhase * 2.0f - 1.0f;                   break;   // saw
            case 3:  lfo = lfoPhase < 0.5f ? 1.0f : -1.0f;           break;   // square
            default: lfo = std::sin(lfoPhase * juce::MathConstants<float>::twoPi); break; // sine
        }

        const float tremAmt = depth * kTremDepth;
        const float trem = 1.0f - tremAmt * 0.5f + lfo * tremAmt * 0.5f;

        if (right != nullptr)
        {
            const float pan = lfo * depth * kPanDepth;
            const float gL = std::sqrt(0.5f * (1.0f - pan)) * trem;
            const float gR = std::sqrt(0.5f * (1.0f + pan)) * trem;
            left[i]  *= gL;
            right[i] *= gR;
        }
        else
        {
            left[i] *= trem;
        }

        lfoPhase += phaseInc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    }
}

// =============================================================================
// Global Limiter (OutputLimiter)
// =============================================================================
void DrumSynthAudioProcessor::processGlobalLimiter(juce::AudioBuffer<float>& mainBuffer)
{
    if (getParamValue(kFxLimiterEn) < 0.5f)
        return;

    mds::fx::OutputLimiter::Params lp;
    lp.thresholdDb = getParamValue(kLimiterThreshold);
    lp.releaseMs   = getParamValue(kLimiterRelease);

    const auto numSamples = mainBuffer.getNumSamples();
    auto* left  = mainBuffer.getWritePointer(0);
    auto* right = mainBuffer.getNumChannels() >= 2 ? mainBuffer.getWritePointer(1) : nullptr;

    fxLimiter.process(left, right, numSamples, lp);
}

void DrumSynthAudioProcessor::processAuxBusSafety(juce::AudioBuffer<float>& busBuffer)
{
    constexpr float kAuxTrimDb = -2.5f;
    const float trim = juce::Decibels::decibelsToGain(kAuxTrimDb);
    const float normalizer = 1.0f / std::max(0.0001f, std::tanh(1.15f));

    for (int channel = 0; channel < busBuffer.getNumChannels(); ++channel)
    {
        auto* data = busBuffer.getWritePointer(channel);
        for (int i = 0; i < busBuffer.getNumSamples(); ++i)
        {
            const float trimmed = data[i] * trim;
            const float protectedSample = std::tanh(trimmed * 1.15f) * normalizer * 0.90f;
            data[i] = std::abs(trimmed) > 0.80f
                ? protectedSample
                : trimmed;
        }
    }
}

void DrumSynthAudioProcessor::updateOutputMeters(const juce::AudioBuffer<float>& buffer, const bool isAuxBus)
{
    float peak = 0.0f;
    double sumSquares = 0.0;
    int sampleCount = 0;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* data = buffer.getReadPointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto sample = data[i];
            peak = std::max(peak, std::abs(sample));
            sumSquares += static_cast<double>(sample) * static_cast<double>(sample);
        }
        sampleCount += buffer.getNumSamples();
    }

    const float rms = sampleCount > 0
        ? static_cast<float>(std::sqrt(sumSquares / static_cast<double>(sampleCount)))
        : 0.0f;

    if (peak >= 0.988f)
        clipLatched.store(true, std::memory_order_relaxed);

    if (isAuxBus)
    {
        auxPeakMeter.store(std::max(auxPeakMeter.load(std::memory_order_relaxed) * 0.82f, peak), std::memory_order_relaxed);
        auxRmsMeter.store(std::max(auxRmsMeter.load(std::memory_order_relaxed) * 0.82f, rms), std::memory_order_relaxed);
    }
    else
    {
        mainPeakMeter.store(std::max(mainPeakMeter.load(std::memory_order_relaxed) * 0.82f, peak), std::memory_order_relaxed);
        mainRmsMeter.store(std::max(mainRmsMeter.load(std::memory_order_relaxed) * 0.82f, rms), std::memory_order_relaxed);
    }
}

void DrumSynthAudioProcessor::resetRuntimeTelemetry()
{
    mainPeakMeter.store(0.0f, std::memory_order_relaxed);
    mainRmsMeter.store(0.0f, std::memory_order_relaxed);
    auxPeakMeter.store(0.0f, std::memory_order_relaxed);
    auxRmsMeter.store(0.0f, std::memory_order_relaxed);
    clipLatched.store(false, std::memory_order_relaxed);
    lastHostBpm.store(0.0f, std::memory_order_relaxed);
    delaySyncActive.store(false, std::memory_order_relaxed);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumSynthAudioProcessor();
}
