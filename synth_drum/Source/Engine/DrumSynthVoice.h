#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

#include "DrumDefs.h"
#include "DrumConstants.h"
#include "FactoryPresets.h"

namespace mds
{
class DrumVoice
{
public:
    virtual ~DrumVoice() = default;

    void start(const PadSettings& settingsToUse, float noteVelocity, double currentSampleRate);
    void render(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    bool isActive() const noexcept { return active; }
    float debugBodyDampingForTests() const noexcept { return bodyDamping; }

    // Audit Phase 3.3: queries/mutators for anti-overlap kick ducking.
    // Used by the host processor to attenuate an already-ringing kick voice
    // when a new kick triggers, preventing low-end accumulation.
    float getCurrentAmplitude() const noexcept { return amplitude; }
    void  duckAmplitude(float gainMultiplier) noexcept
    {
        amplitude *= juce::jlimit(0.0f, 1.0f, gainMultiplier);
    }

    void setPitchBendFactor(float f) noexcept { pitchBendFactor = f; }
    // Poly/channel aftertouch: live gain on the ringing voice (1.0 = neutral).
    void setAftertouchGain(float g) noexcept { aftertouchGain = juce::jlimit(0.5f, 1.5f, g); }
    void setDeterministicSeed(juce::int64 seed) noexcept { random.setSeed(seed); }
    int getCachedPadIndexForTests() const noexcept { return padIndex; }
    DrumInstrumentAlgorithm getCachedAlgorithmForTests() const noexcept { return instrumentAlgorithm; }
    DrumRenderEngineMode getRenderEngineModeForTests() const noexcept { return renderMode; }

protected:
    struct BodyResonatorConfig
    {
        float feedback = 0.0f;
        float damping = 0.5f;
        float frequencyRatio = 1.0f;
    };

    static float computeLowPassAlpha(float cutoffHz, double sampleRate);
    static float computeHighPassAlpha(float cutoffHz, double sampleRate);
    float safePhaseIncForFrequency(float frequencyHz) const noexcept;
    float harmonicNyquistScale(float ratio) const noexcept;
    void advanceWrappedPhase(float& targetPhase, float frequencyHz) const noexcept;
    float readBody(float delaySamples) const;
    bool usesDedicatedV2() const noexcept { return renderMode != DrumRenderEngineMode::LegacyFamily; }
    void applyDedicatedStartShaping();

    virtual float getLifeScale() const = 0;
    virtual float getNoiseScale() const = 0;
    virtual BodyResonatorConfig getBodyResonatorConfig() const { return {}; }
    virtual float getHoldSeconds() const { return 0.0f; }
    virtual void renderModel(float& body, float& noise, float& click) = 0;

    PadSettings settings;
    double sampleRate = 44100.0;
    float velocity = 1.0f;
    int padIndex = 0;
    DrumInstrumentAlgorithm instrumentAlgorithm = DrumInstrumentAlgorithm::KickA;
    DrumRenderEngineMode renderMode = DrumRenderEngineMode::V2;

    float phase      = 0.0f;
    float subPhase   = 0.0f;   // Kick sub oscillator (0.5× pitch)
    float metalPhase = 0.0f;   // Hat/Crash metallic oscillators (audible range)
    float metalBaseHz = 900.0f; // computed in start()
    float amplitude = 0.0f;
    float amplitudeDecayCoeff = 1.0f;
    float noiseEnvelope = 0.0f;
    float noiseDecayCoeff = 1.0f;
    float clickEnvelope = 0.0f;
    float clickDecayCoeff = 1.0f;
    float clickPhase = 0.0f;
    float pitchCurrentHz = 120.0f;
    float pitchTargetHz = 120.0f;
    float aftertouchGain = 1.0f;
    float pitchDecayCoeff = 1.0f;
    float pitchDecayCoeff2 = 1.0f;  // slow stage for 2-phase pitch
    float pitchMidHz = 120.0f;      // midpoint between start and target
    int   pitchStage = 0;           // 0 = fast, 1 = slow
    bool  twoStagePitch = false;

    // SVF filter state
    float svfBand = 0.0f;
    float svfLow = 0.0f;
    float filterF = 0.0f;
    float filterQinv = 1.0f;

    // Noise shaping filters (one-pole, pre-computed in start)
    float noiseLowPassAlpha = 0.5f;
    float noiseHighPassAlpha = 0.99f;
    float clickBandAlpha = 0.5f;
    float noiseLowPassState = 0.0f;
    float noiseHighPassState = 0.0f;
    float noiseHighPassInput = 0.0f;
    float clickBandState = 0.0f;

    float panLeft = constants::kSqrtHalf;
    float panRight = constants::kSqrtHalf;

    // DC blocker state (y[n] = x[n] - x[n-1] + R * y[n-1])
    float dcBlockX1 = 0.0f;
    float dcBlockY1 = 0.0f;
    static constexpr float dcBlockR = constants::kDcBlockR;

    // Body resonator
    static constexpr int kBodyBufSize = 2048;
    std::array<float, kBodyBufSize> bodyBuf{};
    int bodyWritePos = 0;
    float bodyDelay = 0.0f;
    float bodyFeedback = 0.0f;
    float bodyDampState = 0.0f;
    float bodyDamping = 0.5f;
    int activeMetallicPartials = kMaxMetallicPartials;
    float outputTrim = 0.9f;
    float stereoSideAmount = 0.0f;
    float stereoPhase = 0.0f;
    float stereoPhaseInc = 0.0f;

    float pitchBendFactor = 1.0f;

    int ageSamples = 0;
    int attackSamples = 1;
    int holdSamples = 0;
    int clickSamples = 0;
    int clickLengthSamples = 1;
    int maxAgeSamples = 0;
    bool active = false;

    juce::Random random;
};

// =========================================================================
// TonalDrumVoice — sine + sub + harmonics + pitch envelope
// Used by: Kick, Snare, Tom
// =========================================================================
class TonalDrumVoice : public DrumVoice
{
public:
    struct TonalConfig
    {
        int   modelIndex;          // index into kVoiceEnvelopes / kBodyResonators
        float sineLevel;           // main sine amplitude
        float subLevel;            // sub oscillator (0.5×), 0 = disabled
        float harmonic2;           // 2nd harmonic (2×)
        float harmonic3;           // 3rd harmonic (3×)
        float goldenRatioLevel;    // golden ratio (φ×), 0 = disabled
        bool  pitchContourOnH2;    // multiply h2 by pitch contour envelope
        float clickScale;
        float noiseModScale;
    };

protected:
    float getLifeScale() const override;
    float getNoiseScale() const override;
    BodyResonatorConfig getBodyResonatorConfig() const override;
    float getHoldSeconds() const override;
    void renderModel(float& body, float& noise, float& click) override;

    virtual const TonalConfig& tonalConfig() const = 0;
};

class KickVoice final : public TonalDrumVoice
{
    const TonalConfig& tonalConfig() const override;
};

class SnareVoice final : public TonalDrumVoice
{
    const TonalConfig& tonalConfig() const override;
};

class TomVoice final : public TonalDrumVoice
{
    const TonalConfig& tonalConfig() const override;
};

// =========================================================================
// MetallicDrumVoice — inharmonic partial synthesis
// Used by: Hat, Crash
// =========================================================================
class MetallicDrumVoice : public DrumVoice
{
public:
    struct MetallicConfig
    {
        int   modelIndex;
        bool  useCrashPartials;    // false = kHatPartials, true = kCrashPartials
        float bodyBase;            // minimum metallic body level
        float bodyModulation;      // noiseEnvelope * this added to body
        float noiseModScale;
        float clickScale;
    };

protected:
    float getLifeScale() const override;
    float getNoiseScale() const override;
    void renderModel(float& body, float& noise, float& click) override;

    virtual const MetallicConfig& metallicConfig() const = 0;
};

class HatVoice final : public MetallicDrumVoice
{
    const MetallicConfig& metallicConfig() const override;
};

class CrashVoice final : public MetallicDrumVoice
{
    const MetallicConfig& metallicConfig() const override;
};

// =========================================================================
// ClapVoice — multi-burst noise synthesis (NoiseBurst)
// =========================================================================
class ClapVoice final : public DrumVoice
{
protected:
    float getLifeScale() const override;
    float getNoiseScale() const override;
    void renderModel(float& body, float& noise, float& click) override;
};

// =========================================================================
// ModalDrumVoice — knock/impact + body resonance
// Used by: PercWood, PercMetal
// =========================================================================
class ModalDrumVoice : public DrumVoice
{
public:
    struct ModalConfig
    {
        int   modelIndex;
        float sineLevel;
        float harmonic2;
        float knockRateMultiplier;
        float minKnockRate;
        float impactLevel;
        bool  hasMetalPartials;    // adds golden ratio + sqrt(2) partials
        float noiseKnockScale;
        float clickKnockScale;
    };

protected:
    float getLifeScale() const override;
    float getNoiseScale() const override;
    BodyResonatorConfig getBodyResonatorConfig() const override;
    void renderModel(float& body, float& noise, float& click) override;

    virtual const ModalConfig& modalConfig() const = 0;
};

class PercWoodVoice final : public ModalDrumVoice
{
    const ModalConfig& modalConfig() const override;
};

class PercMetalVoice final : public ModalDrumVoice
{
    const ModalConfig& modalConfig() const override;
};

// =========================================================================
// FxVoice — FM sweep synthesis
// =========================================================================
class FxVoice final : public DrumVoice
{
protected:
    float getLifeScale() const override;
    float getNoiseScale() const override;
    void renderModel(float& body, float& noise, float& click) override;
};

// =========================================================================
// VoicePool — pre-allocated bank of voices (zero RT heap allocation)
// 9 models × 8 slots = 72 voices created once in constructor
// =========================================================================
class VoicePool
{
public:
    VoicePool();

    /** Acquire a free voice of the given model. Returns nullptr if all slots are in use. */
    DrumVoice* acquire(PadVoiceModel model);

    /** Release a voice back to the pool. O(kMaxVoicesPerModel) pointer scan. */
    void release(PadVoiceModel model, DrumVoice* voice) noexcept;

private:
    struct Slot
    {
        std::unique_ptr<DrumVoice> voice;
        bool inUse = false;
    };

    std::array<std::array<Slot, kMaxVoicesPerModel>, kNumVoiceModels> bank;
};

// =========================================================================
// Voice factory (used by renderer and pool constructor)
// =========================================================================
std::unique_ptr<DrumVoice> createVoiceForModel(PadVoiceModel voiceModel);
std::unique_ptr<DrumVoice> createVoiceForPad(int padIndex);
} // namespace mds
