#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <random>
#include <vector>

#include "Engine/DrumSynthVoice.h"
#include "Engine/FxProcessors.h"
#include "Engine/FactoryPresets.h"
#include "../../Shared/PitchBendState.h"

class DrumSynthAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int kNumAuxOutputs = mds::kNumPads;
    static constexpr const char* kProcessorName = "UWdeVST_drum";

    enum class QualityMode : int
    {
        Live = 0,
        Studio = 1
    };

    struct PresetLibraryEntry
    {
        juce::String name;
        juce::String familyLabel;
        juce::String mixRole;
        juce::String description;
        juce::String outputProfile;
        juce::StringArray tags;
        float nominalPeakDb = -6.0f;
        bool isFactory = false;
        int factoryIndex = -1;
        juce::File presetFile;
        juce::File manifestFile;
    };

    DrumSynthAudioProcessor();
    ~DrumSynthAudioProcessor() override = default;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::String makePadParamId(int padIndex, const juce::String& suffix);
    static auto createBusLayout() -> BusesProperties;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
#if defined(JucePlugin_Name)
        return JucePlugin_Name;
#else
        return kProcessorName;
#endif
    }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return parameters; }
    juce::StringArray getFactoryPresetNames() const;
    int getCurrentFactoryPresetIndex() const noexcept { return currentPresetIndex; }
    void applyFactoryPreset(int presetIndex);
    bool saveFactoryPreset(int presetIndex);

    // Kit user preset management
    static juce::File getUserPresetsDirectory();
    static juce::File getFactoryOverridesDirectory();
    juce::Array<juce::File> scanUserPresets() const;
    juce::Array<PresetLibraryEntry> scanPresetLibrary() const;
    juce::StringArray getPresetFamilyChoices() const;
    juce::StringArray getPresetMixRoleChoices() const;
    juce::StringArray getPresetTagChoices() const;
    PresetLibraryEntry getCurrentPresetEntry() const;
    bool saveUserPreset(const juce::String& name);
    bool updateUserPreset(const juce::File& file);
    bool deleteUserPreset(const juce::File& file);
    bool loadUserPreset(const juce::File& file);
    bool isCurrentPresetUser() const noexcept { return currentUserPresetFile.existsAsFile(); }
    juce::File getCurrentUserPresetFile() const noexcept { return currentUserPresetFile; }

    // Per-pad preset management
    juce::StringArray getFactoryPadPresetNames(int padIndex) const;
    int getCurrentPadPresetIndex(int padIndex) const noexcept;
    void applyFactoryPadPreset(int padIndex, int presetIndex);
    static juce::File getUserPadPresetsDirectory(int padIndex);
    juce::Array<juce::File> scanUserPadPresets(int padIndex) const;
    bool saveUserPadPreset(int padIndex, const juce::String& name);
    bool loadUserPadPreset(int padIndex, const juce::File& file);
    bool deleteUserPadPreset(int padIndex, const juce::File& file);

    void queuePadTrigger(int padIndex, float velocity);

    // MIDI CC page system
    static constexpr int kNumCCPages = 7;
    int  getMidiCCPage()  const noexcept  { return midiCCPage.load(std::memory_order_relaxed); }
    const char* getCCPageName(int page) const;

    // MIDI Learn system
    void startMidiLearn(const juce::String& paramId);
    void cancelMidiLearn();
    bool isMidiLearning() const noexcept { return midiLearnActive.load(); }
    juce::String getMidiLearnParamId() const noexcept { return midiLearnParamId; }
    int getMidiLearnCcNumber() const noexcept { return midiLearnCc.load(); }
    void clearMidiLearn(const juce::String& paramId);
    void clearAllMidiLearn();
    juce::StringArray getMidiLearnedParams() const;
    int getMidiCcForParam(const juce::String& paramId) const;
    juce::StringArray getMidiLearnTargetIds() const;
    juce::String getParameterDisplayName(const juce::String& paramId) const;

    float getMainPeakMeter() const noexcept { return mainPeakMeter.load(std::memory_order_relaxed); }
    float getMainRmsMeter() const noexcept { return mainRmsMeter.load(std::memory_order_relaxed); }
    float getAuxPeakMeter() const noexcept { return auxPeakMeter.load(std::memory_order_relaxed); }
    float getAuxRmsMeter() const noexcept { return auxRmsMeter.load(std::memory_order_relaxed); }

    // Audit Phase 4.4a: per-pad trigger envelope (latest velocity stamped at
    // trigger time). Editor reads this to drive the pad mini-VU. The value is
    // the last triggered velocity (0..~1.2); editor handles smoothing/decay.
    float consumePadTriggerActivity(int padIndex) noexcept;
    bool isClipLatched() const noexcept { return clipLatched.load(std::memory_order_relaxed); }
    void clearClipLatch() noexcept { clipLatched.store(false, std::memory_order_relaxed); }
    float getLastHostBpm() const noexcept { return lastHostBpm.load(std::memory_order_relaxed); }
    bool isDelaySyncActive() const noexcept { return delaySyncActive.load(std::memory_order_relaxed); }
    QualityMode getQualityMode() const noexcept;

private:
    struct TriggerEvent
    {
        int padIndex = 0;
        float velocity = 1.0f;
        int sampleOffset = 0;
    };

    struct ActiveVoice
    {
        mds::DrumVoice* voice = nullptr;   // raw ptr, owned by voicePool
        mds::PadVoiceModel voiceModel{};   // needed for pool release
        int padIndex = 0;
        int chokeGroup = 0;
        int outputBus = 0;
        uint64_t activationAge = 0;
        int fadeOutSamples = 0;            // >0 = fading out before release
        float fadeOutGain = 1.0f;          // current fade gain
    };

    int mapMidiNoteToPad(int midiNote) const;
    bool isSingleNoteModeEnabled() const;
    int getSelectedPadIndex() const;
    int getPadOutputBus(int padIndex) const;
    int getChokeGroupForPad(int padIndex) const;
    float getParamValue(const juce::String& paramId) const;
    mds::PadSettings snapshotPadSettings(int padIndex) const;
    void applyPerformanceMacros(int padIndex, mds::PadSettings& settings) const;
    void triggerPadNow(int padIndex, float velocity);
    void renderActiveVoicesForRange(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void enqueuePendingTrigger(int padIndex, float velocity, int sampleOffset);
    void setParamValue(const juce::String& paramId, float value);
    void updateGlobalEffectParameters();
    void processGlobalTransient(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalSaturator(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalCompressor(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalReverb(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalEQ(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalChorus(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalDelay(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalLimiter(juce::AudioBuffer<float>& mainBuffer);
    void processGlobalLfo(juce::AudioBuffer<float>& mainBuffer);
    void processAuxBusSafety(juce::AudioBuffer<float>& busBuffer);
    void handleMidiCC(int ccNumber, int ccValue);
    void updateOutputMeters(const juce::AudioBuffer<float>& buffer, bool isAuxBus);
    void resetRuntimeTelemetry();

    void loadFactoryOverrides();
    void backfillMissingPresetManifests() const;

    juce::AudioProcessorValueTreeState parameters;
    std::vector<mds::KitPreset> factoryPresets;

    mds::VoicePool voicePool;
    std::array<ActiveVoice, mds::kMaxActiveVoices> activeVoices{};
    int activeVoiceCount = 0;
    uint64_t voiceAgeCounter = 0;

    juce::dsp::Compressor<float> compressor;
    mds::fx::DattorroPlateReverb dattorroReverb;
    mds::fx::ParametricEQ3Band   fxEq;
    mds::fx::StereoChorus        fxChorus;
    mds::fx::StereoDelay         fxDelay;
    mds::fx::OutputLimiter       fxLimiter;
    juce::AudioBuffer<float> fxDryBuffer;
    std::array<float, 2> transientFastEnv = { 0.0f, 0.0f };
    std::array<float, 2> transientSlowEnv = { 0.0f, 0.0f };
    std::array<float, 2> saturatorPrevSample = { 0.0f, 0.0f };
    double preparedSampleRate = 44100.0;
    juce::SmoothedValue<float> outputGainSmoother;
    juce::SmoothedValue<float> transientAttackSmoother;
    juce::SmoothedValue<float> transientSustainSmoother;
    juce::SmoothedValue<float> transientMixSmoother;
    juce::SmoothedValue<float> satDriveSmoother;
    juce::SmoothedValue<float> satMixSmoother;
    juce::SmoothedValue<float> macroPunchSmoother;
    juce::SmoothedValue<float> macroWeightSmoother;
    juce::SmoothedValue<float> macroAirSmoother;
    juce::SmoothedValue<float> macroDirtSmoother;
    float macroPunchValue = 0.5f;
    float macroWeightValue = 0.5f;
    float macroAirValue = 0.5f;
    float macroDirtValue = 0.5f;

    // Lock-free FIFO for GUI→audio trigger passing (avoids priority inversion)
    static constexpr int kTriggerFifoSize = 128;
    std::array<TriggerEvent, kTriggerFifoSize> triggerFifoBuffer{};
    juce::AbstractFifo triggerFifo { kTriggerFifoSize };
    std::vector<TriggerEvent> triggerBatch;
    std::array<TriggerEvent, kTriggerFifoSize> pendingTriggers{};
    int pendingTriggerCount = 0;
    std::minstd_rand humanizeRng { 0x13579BDFu };

    struct CompressorCache
    {
        float threshold =  1.0f;   // out of [-60,0] range: forces first update
        float ratio     = -1.0f;   // out of [1,20] range
        float attack    = -1.0f;   // out of [0.1,100] range
        float release   = -1.0f;   // out of [5,500] range
    } compCache;

    PitchBendState pitchBend;
    VelocityCurve  velocityCurve = VelocityCurve::Linear;
    float lfoPhase = 0.0f;
    std::atomic<int> midiCCPage { 0 };
    std::atomic<bool> midiLearnActive { false };
    std::atomic<int> midiLearnCc { -1 };
    juce::String midiLearnParamId;
    std::map<int, juce::String> midiLearnMap;
    juce::CriticalSection midiLearnLock;

    std::atomic<float> mainPeakMeter { 0.0f };
    std::atomic<float> mainRmsMeter { 0.0f };
    std::atomic<float> auxPeakMeter { 0.0f };
    std::atomic<float> auxRmsMeter { 0.0f };
    std::array<std::atomic<float>, mds::kNumPads> padTriggerActivity {};
    std::atomic<bool> clipLatched { false };
    std::atomic<float> lastHostBpm { 0.0f };
    std::atomic<bool> delaySyncActive { false };

    int currentPresetIndex = -1;
    juce::File currentUserPresetFile;
    std::array<int, mds::kNumPads> currentPadPresetIndices {};
    PresetLibraryEntry currentPresetEntry;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSynthAudioProcessor)
};
