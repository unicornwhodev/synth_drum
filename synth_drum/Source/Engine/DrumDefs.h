#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace mds
{
// =========================================================================
// Pad count & names
// =========================================================================
constexpr int kNumPads = 12;

constexpr std::array<const char*, kNumPads> kPadNames = {
    "Kick A", "Kick B", "Snare", "Clap", "Hat Closed", "Hat Open",
    "Perc 1", "Perc 2", "Tom Low", "Tom High", "Crash", "FX"
};

// =========================================================================
// Drum family —  logical groupings for UI, macros, and choke groups
// =========================================================================
enum class DrumFamily : uint8_t
{
    Kick,
    Snare,
    Clap,
    Hat,
    Perc,
    Tom,
    Crash,
    FX
};

constexpr std::array<DrumFamily, kNumPads> kPadFamily = {
    DrumFamily::Kick,   // 0  Kick A
    DrumFamily::Kick,   // 1  Kick B
    DrumFamily::Snare,  // 2  Snare
    DrumFamily::Clap,   // 3  Clap
    DrumFamily::Hat,    // 4  Hat Closed
    DrumFamily::Hat,    // 5  Hat Open
    DrumFamily::Perc,   // 6  Perc 1
    DrumFamily::Perc,   // 7  Perc 2
    DrumFamily::Tom,    // 8  Tom Low
    DrumFamily::Tom,    // 9  Tom High
    DrumFamily::Crash,  // 10 Crash
    DrumFamily::FX      // 11 FX
};

// =========================================================================
// Synthesis mode — determines the primary render pipeline
// =========================================================================
enum class SynthesisMode : uint8_t
{
    Tonal,          // sine + sub + pitch envelope (kick, tom)
    NoiseBurst,     // multi-burst noise (clap)
    Metallic,       // inharmonic partials (hat, crash)
    Modal,          // body resonator dominant (perc wood/metal)
    FM              // FM sweep (FX)
};

// =========================================================================
// Per-pad voice model (unchanged enum, used by voice factory)
// =========================================================================
enum class PadVoiceModel : uint8_t
{
    Kick,
    Snare,
    Clap,
    Hat,
    PercWood,
    PercMetal,
    Tom,
    Crash,
    Fx
};

// =========================================================================
// Pad characteristics — defines the sonic identity structure per pad
// =========================================================================
struct PadCharacteristics
{
    PadVoiceModel   voiceModel;
    DrumFamily      family;
    SynthesisMode   synthesisMode;
    float           baseFrequencyHz;
    int             chokeGroup;       // 0 = none, >0 = mutual choke
    const char*     name;
};

constexpr std::array<PadCharacteristics, kNumPads> kPadCharacteristics = {{
    { PadVoiceModel::Kick,      DrumFamily::Kick,   SynthesisMode::Tonal,       90.0f,  0, "Kick A"     },
    { PadVoiceModel::Kick,      DrumFamily::Kick,   SynthesisMode::Tonal,       96.0f,  0, "Kick B"     },
    { PadVoiceModel::Snare,     DrumFamily::Snare,  SynthesisMode::Tonal,      248.0f,  0, "Snare"      },
    { PadVoiceModel::Clap,      DrumFamily::Clap,   SynthesisMode::NoiseBurst, 300.0f,  0, "Clap"       },
    { PadVoiceModel::Hat,       DrumFamily::Hat,    SynthesisMode::Metallic,  5500.0f,  1, "Hat Closed" },
    { PadVoiceModel::Hat,       DrumFamily::Hat,    SynthesisMode::Metallic,  4800.0f,  1, "Hat Open"   },
    { PadVoiceModel::PercWood,  DrumFamily::Perc,   SynthesisMode::Modal,      480.0f,  0, "Perc 1"     },
    { PadVoiceModel::PercMetal, DrumFamily::Perc,   SynthesisMode::Modal,      650.0f,  0, "Perc 2"     },
    { PadVoiceModel::Tom,       DrumFamily::Tom,    SynthesisMode::Tonal,      175.0f,  0, "Tom Low"    },
    { PadVoiceModel::Tom,       DrumFamily::Tom,    SynthesisMode::Tonal,      250.0f,  0, "Tom High"   },
    { PadVoiceModel::Crash,     DrumFamily::Crash,  SynthesisMode::Metallic,  6400.0f,  0, "Crash"      },
    { PadVoiceModel::Fx,        DrumFamily::FX,     SynthesisMode::FM,         560.0f,  0, "FX"         },
}};

// =========================================================================
// Voice pool sizing
// =========================================================================
constexpr int kNumVoiceModels    = 9;   // one per PadVoiceModel value
constexpr int kMaxVoicesPerModel = 8;   // max simultaneous voices of same type
constexpr int kMaxActiveVoices   = 72;  // global polyphony cap (= kNumVoiceModels * kMaxVoicesPerModel)

// =========================================================================
// Metallic partial ratios — inharmonic frequency ratios for hats/crash
// Based on real cymbal modal analysis (approximate)
// =========================================================================
constexpr int kMaxMetallicPartials = 12;

struct MetallicPartialSet
{
    int   count;
    float ratios[kMaxMetallicPartials];
    float amplitudes[kMaxMetallicPartials];
};

// Hat: tight inharmonic cluster — 12 modes based on cymbal modal analysis (Rossing/Fletcher)
constexpr MetallicPartialSet kHatPartials = {
    12,
    { 1.0f, 1.506f, 1.741f, 2.0f, 2.414f, 2.828f,
      3.162f, 3.606f, 4.0f, 4.414f, 4.899f, 5.657f },
    { 0.22f, 0.20f, 0.16f, 0.12f, 0.09f, 0.07f,
      0.05f, 0.035f, 0.025f, 0.018f, 0.012f, 0.008f }
};

// Crash: wider, denser inharmonic set — 12 modes
constexpr MetallicPartialSet kCrashPartials = {
    12,
    { 1.0f, 1.414f, 1.848f, 2.236f, 2.646f, 3.0f,
      3.464f, 3.873f, 4.359f, 4.899f, 5.385f, 5.916f },
    { 0.12f, 0.10f, 0.08f, 0.065f, 0.05f, 0.04f,
      0.03f, 0.022f, 0.016f, 0.012f, 0.008f, 0.005f }
};

// =========================================================================
// Body resonator presets per voice model
// =========================================================================
struct BodyResonatorDefaults
{
    float feedback;
    float damping;
    float frequencyRatio;
};

constexpr BodyResonatorDefaults kBodyResonatorNone = { 0.0f, 0.5f, 1.0f };

constexpr std::array<BodyResonatorDefaults, 9> kBodyResonators = {{
    { 0.35f, 0.6f,  0.85f },  // Kick
    { 0.12f, 0.4f,  1.5f  },  // Snare
    { 0.0f,  0.5f,  1.0f  },  // Clap   (none)
    { 0.0f,  0.5f,  1.0f  },  // Hat    (none)
    { 0.25f, 0.3f,  1.2f  },  // PercWood
    { 0.20f, 0.4f,  1.1f  },  // PercMetal
    { 0.30f, 0.55f, 1.0f  },  // Tom
    { 0.0f,  0.5f,  1.0f  },  // Crash  (none)
    { 0.0f,  0.5f,  1.0f  },  // Fx     (none)
}};

// =========================================================================
// Voice life-scale and noise-scale constants per voice model
// =========================================================================
struct VoiceEnvelopeScale
{
    float lifeScale;     // multiplier on decay for maximum voice lifetime
    float noiseScale;    // base noise decay ratio
    float holdSeconds;   // sustain hold time at peak amplitude
};

constexpr std::array<VoiceEnvelopeScale, 9> kVoiceEnvelopes = {{
    { 2.2f, 0.06f, 0.005f },  // Kick
    { 2.0f, 0.22f, 0.002f },  // Snare
    { 1.8f, 0.12f, 0.0f   },  // Clap
    { 1.7f, 0.18f, 0.0f   },  // Hat
    { 2.0f, 0.02f, 0.0f   },  // PercWood
    { 2.05f, 0.05f, 0.0f  },  // PercMetal
    { 2.2f, 0.08f, 0.004f },  // Tom
    { 2.4f, 0.22f, 0.0f   },  // Crash
    { 2.0f, 0.10f, 0.0f   },  // Fx
}};

// =========================================================================
// Per-voice-model SVF filter Q
// =========================================================================
constexpr std::array<float, kNumVoiceModels> kVoiceFilterQ = {{
    1.4f,    // Kick     — body emphasis without resonant ringing
    0.9f,    // Snare    — moderate, avoids ringing
    0.707f,  // Clap     — neutral Butterworth
    0.707f,  // Hat      — neutral
    1.2f,    // PercWood — slight resonance
    1.5f,    // PercMetal — metallic ring
    1.2f,    // Tom      — moderate resonance without honk
    0.707f,  // Crash    — neutral
    1.0f,    // Fx       — neutral
}};

// =========================================================================
// Convenience helpers
// =========================================================================
inline const PadCharacteristics& getPadInfo(int padIndex)
{
    return kPadCharacteristics[static_cast<std::size_t>(
        padIndex < 0 ? 0 : (padIndex >= kNumPads ? kNumPads - 1 : padIndex))];
}

inline float getPadBaseFrequency(int padIndex)
{
    return getPadInfo(padIndex).baseFrequencyHz;
}

inline PadVoiceModel getPadVoiceModel(int padIndex)
{
    return getPadInfo(padIndex).voiceModel;
}

inline int getPadChokeGroup(int padIndex)
{
    return getPadInfo(padIndex).chokeGroup;
}

inline DrumFamily getPadFamily(int padIndex)
{
    return getPadInfo(padIndex).family;
}

std::string makePadName(int padIndex);

} // namespace mds
