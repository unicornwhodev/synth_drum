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

class DrumSynthAudioProcessor : public juce::AudioProcessor,
                                 private juce::AsyncUpdater
{
public:
    static constexpr int kNumAuxOutputs = mds::kNumPads;
    static constexpr const char* kProcessorName = "UWdeVST_Drum";

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
    void flushPendingAsyncUpdatesForTests();

    float getMainPeakMeter() const noexcept { return mainPeakMeter.load(std::memory_order_relaxed); }
    float getMainRmsMeter() const noexcept { return mainRmsMeter.load(std::memory_order_relaxed); }
    float getAuxPeakMeter() const noexcept { return auxPeakMeter.load(std::memory_order_relaxed); }
    float getAuxRmsMeter() const noexcept { return auxRmsMeter.load(std::memory_order_relaxed); }

    // Audit Phase 4.4a: per-pad trigger envelope (latest velocity stamped at
    // trigger time). Editor reads this to drive the pad mini-VU. The value is
    // the last triggered velocity (0..1.0, clamped at trigger input);
    // editor handles smoothing/decay.
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
        bool fadeOutActive = false;        // fade engaged; once fadeOutSamples hits 0
                                           // the voice stays silent until release
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
    bool isPadAudibleInBlock(int pad) const noexcept;
    void renderActiveVoicesForRange(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void resetTriggerBatch() noexcept;
    bool appendTriggerToBatch(const TriggerEvent& trigger) noexcept;
    void enqueuePendingTrigger(int padIndex, float velocity, int sampleOffset);
    void setParamValue(const juce::String& paramId, float value);
    struct CompressorCache
    {
        float threshold =  1.0f;   // out of [-60,0] range: forces first update
        float ratio     = -1.0f;   // out of [1,20] range
        float attack    = -1.0f;   // out of [0.1,100] range
        float release   = -1.0f;   // out of [5,500] range
    };

    // Full per-bus FX chain state. The master bus owns one instance and every
    // aux bus owns its own: post-FX aux processing no longer shares (and
    // corrupts) stateful processors across buses (aux_post_fx crosstalk fix).
    struct FxBusState
    {
        juce::dsp::Compressor<float> compressor;
        mds::fx::DattorroPlateReverb reverb;
        mds::fx::ParametricEQ3Band   eq;
        mds::fx::StereoChorus        chorus;
        mds::fx::StereoDelay         delay;
        mds::fx::OutputLimiter       limiter;
        juce::dsp::Oversampling<float> satOversamplingMono {
            1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false
        };
        juce::dsp::Oversampling<float> satOversamplingStereo {
            2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false
        };
        // Studio saturator: the oversampled wet path lags by the oversampler
        // filter latency, so the dry reference is delayed by the same amount
        // to keep the dry/wet mix phase-aligned (avoids comb colouration).
        juce::dsp::DelayLine<float> satDryDelay { 256 };
        bool satDryDelayPrimed = false;
        juce::AudioBuffer<float> dryBuffer;
        std::array<float, 2> transientFastEnv { 0.0f, 0.0f };
        std::array<float, 2> transientSlowEnv { 0.0f, 0.0f };
        juce::SmoothedValue<float> transientAttackSmoother;
        juce::SmoothedValue<float> transientSustainSmoother;
        juce::SmoothedValue<float> transientMixSmoother;
        juce::SmoothedValue<float> satDriveSmoother;
        juce::SmoothedValue<float> satMixSmoother;
        float lfoPhase = 0.0f;
        CompressorCache compCache;
        bool isMaster = false;
        // Silent-bus skip: blocks an aux chain keeps processing after its
        // input went silent, so delay/reverb tails can ring out.
        int tailBlocksLeft = 0;
    };

    void prepareFxBusState(FxBusState& chain, const juce::dsp::ProcessSpec& spec);
    void updateGlobalEffectParameters(FxBusState& chain);
    void processGlobalTransient(juce::AudioBuffer<float>& mainBuffer, FxBusState& chain);
    void processGlobalSaturator(juce::AudioBuffer<float>& mainBuffer, FxBusState& chain);
    void processGlobalCompressor(juce::AudioBuffer<float>& mainBuffer, FxBusState& chain);
    void processGlobalReverb(juce::AudioBuffer<float>& mainBuffer, FxBusState& chain);
    void processGlobalEQ(juce::AudioBuffer<float>& mainBuffer, FxBusState& chain);
    void processGlobalChorus(juce::AudioBuffer<float>& mainBuffer, FxBusState& chain);
    void processGlobalDelay(juce::AudioBuffer<float>& mainBuffer, FxBusState& chain);
    void processGlobalLimiter(juce::AudioBuffer<float>& mainBuffer, FxBusState& chain);
    void processGlobalLfo(juce::AudioBuffer<float>& mainBuffer, FxBusState& chain);
    // Audit Phase 5 D3: process the per-pad send buses (reverb + delay) and
    // mix the wet result into mainBuffer. Idempotent when sends are zero.
    void processPadSends(juce::AudioBuffer<float>& mainBuffer);
    void processAuxBusSafety(juce::AudioBuffer<float>& busBuffer);
    void handleMidiCC(int ccNumber, int ccValue);
    void handleAsyncUpdate() override;
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

    FxBusState masterFx;
    std::array<FxBusState, kNumAuxOutputs> auxFx;

    // Audit Phase 5 D3: dedicated send instances + scratch buses for the
    // per-pad reverb/delay sends. They run AFTER the master FX bus so dry/wet
    // contributions stay independent. mix is forced to 1.0 (pure wet).
    mds::fx::DattorroPlateReverb sendReverb;
    mds::fx::StereoDelay         sendDelay;
    juce::AudioBuffer<float>     reverbSendBuffer;
    juce::AudioBuffer<float>     delaySendBuffer;
    juce::AudioBuffer<float>     voiceScratchBuffer;
    std::array<float, mds::kNumPads> currentPadReverbSend{};
    std::array<float, mds::kNumPads> currentPadDelaySend{};
    std::array<float, mds::kNumPads> currentPadMute{};
    std::array<float, mds::kNumPads> currentPadSolo{};
    bool anyPadSoloActive = false;
    std::array<float, mds::kNumPads> padAftertouch{};
    double preparedSampleRate = 44100.0;
    juce::SmoothedValue<float> outputGainSmoother;
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
    static constexpr int kMaxTriggerBatchSize = kTriggerFifoSize * 2;
    std::array<TriggerEvent, kTriggerFifoSize> triggerFifoBuffer{};
    juce::AbstractFifo triggerFifo { kTriggerFifoSize };
    std::array<TriggerEvent, kMaxTriggerBatchSize> triggerBatch{};
    int triggerBatchCount = 0;
    std::array<TriggerEvent, kTriggerFifoSize> pendingTriggers{};
    int pendingTriggerCount = 0;
    std::minstd_rand humanizeRng { 0x13579BDFu };

    PitchBendState pitchBend;
    VelocityCurve  velocityCurve = VelocityCurve::Linear;
    std::atomic<int> midiCCPage { 0 };
    std::atomic<bool> midiLearnActive { false };
    std::atomic<int> midiLearnCc { -1 };
    juce::String midiLearnParamId;
    std::map<int, juce::String> midiLearnMap;
    juce::CriticalSection midiLearnLock;
    std::atomic<juce::RangedAudioParameter*> midiLearnArmedParam { nullptr };
    std::array<std::atomic<juce::RangedAudioParameter*>, 128> midiLearnParamSnapshot {};
    std::atomic<int> pendingMidiLearnCc { -1 };
    std::atomic<juce::RangedAudioParameter*> pendingMidiLearnParam { nullptr };
    std::atomic<float> pendingMidiLearnValue { 0.0f };
    void rebuildMidiLearnSnapshot();

    struct PendingParamUpdate
    {
        juce::RangedAudioParameter* param = nullptr;
        float normalisedValue = 0.0f;
    };
    static constexpr int kPendingParamQueueSize = 64;
    juce::AbstractFifo pendingParamFifo { kPendingParamQueueSize };
    std::array<PendingParamUpdate, kPendingParamQueueSize> pendingParamQueue;
    void queueParamUpdate(juce::RangedAudioParameter* param, float normalisedValue);

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
