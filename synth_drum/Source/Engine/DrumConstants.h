#pragma once

// =============================================================================
// DrumConstants.h — Named constants for the Drum synth DSP engine.
//
// Replaces hardcoded magic numbers throughout DrumSynthVoice.cpp with
// documented, self-describing constants.
// =============================================================================

namespace mds::constants
{

// ---- Click transient ----
constexpr float kClickLengthSec         = 0.0025f;   // 2.5 ms click burst
constexpr float kClickDecayMinSec       = 0.0006f;   // shortest click decay
constexpr float kClickDecayMaxSec       = 0.008f;    // longest click decay
constexpr float kClickDecayBaseSec      = 0.0010f;   // base click decay
constexpr float kClickDecayAmountScale  = 0.0020f;   // per click-amount scaling
constexpr float kClickBaseFreqMin       = 900.0f;    // minimum click carrier Hz
constexpr float kClickCutoffRatio       = 0.15f;     // click freq = max(900, cutoff * this)
constexpr float kClickNoiseBlend        = 0.35f;     // noise proportion in click
constexpr float kClickCarrierBlend      = 0.65f;     // sine proportion in click

// ---- Noise shaping ----
constexpr float kNoiseLPCutoffScale     = 1.15f;     // noise LP = cutoff * 1.15
constexpr float kNoiseLPMaxRatio        = 0.40f;     // max LP cutoff relative to SR
constexpr float kNoiseHPMinHz           = 250.0f;    // minimum noise HP cutoff
constexpr float kNoiseHPMaxHz           = 6000.0f;   // maximum noise HP cutoff
constexpr float kNoiseHPCutoffScale     = 0.18f;     // noise HP = cutoff * 0.18
constexpr float kClickBandCutoffScale   = 1.4f;      // click band = cutoff * 1.4
constexpr float kClickBandMinHz         = 1200.0f;   // minimum click band Hz
constexpr float kClickBandMaxRatio      = 0.35f;     // max click band relative to SR
constexpr float kNoiseDecayMinSec       = 0.003f;    // minimum noise envelope decay
constexpr float kNoiseDecayMaxSec       = 0.45f;     // maximum noise envelope decay

// ---- Voice lifetime ----
constexpr float kMinDecaySec            = 0.03f;     // minimum voice lifetime
constexpr float kAmpDeathThreshold      = 0.00025f;  // amplitude below this = silent
constexpr float kNoiseDeathThreshold    = 0.0006f;   // noise envelope below this = silent

// ---- SVF filter ----
constexpr float kSVFStabilityMargin     = 0.95f;     // Jury stability safety factor
constexpr float kMinFilterQ             = 0.5f;      // minimum allowed Q

// ---- DC blocker ----
constexpr float kDcBlockR               = 0.9995f;   // ~5 Hz HPF @ 44.1 kHz

// ---- Body resonator ----
constexpr float kBodyMinFreqHz          = 30.0f;     // min body resonance Hz
constexpr float kBodyMaxFreqRatio       = 0.4f;      // max body freq relative to SR
constexpr float kBodyFeedbackThreshold  = 0.001f;    // below this = disabled
constexpr float kBodyResBlend           = 0.5f;      // body resonance mix-back ratio

// ---- Denormal flushing ----
constexpr float kDenormalFloor          = 1e-15f;

// ---- Drive / saturation ----
constexpr float kDriveScaleFactor       = 0.75f;     // output drive range compression

// ---- Velocity scaling ----
constexpr float kVelCutoffBase          = 0.65f;     // cutoff = cutoff * (0.65 + 0.35*vel)
constexpr float kVelCutoffRange         = 0.35f;     // was 0.25: +10% range for better vel dynamics
constexpr float kPitchDropVelThreshold  = 3.0f;      // semitones above which vel scales drop
constexpr float kPitchDropVelBase       = 0.78f;
constexpr float kPitchDropVelRange      = 0.22f;

// ---- Kick two-stage pitch ----
constexpr float kKickPitchMidRatio      = 0.4f;      // fast stage reaches 40% of drop
constexpr float kKickPitchFastScale     = 0.15f;     // fast stage = 15% of decay time
constexpr float kKickPitchSlowScale     = 1.2f;      // slow stage = 120% of decay time
constexpr float kKickPitchStageThresh   = 1.01f;     // switch when within 1% of mid

// ---- Metallic partials ----
constexpr float kMetalBaseMinHz         = 1200.0f;   // minimum metallic base frequency
constexpr float kMetalBaseFreqDivisor   = 3.0f;      // metalBase = max(1200, startFreq/3)

// ---- Clap (NoiseBurst) ----
constexpr float kClapBurstDecayRate     = 90.0f;     // exponential decay rate per burst
constexpr float kClapBurstMaxEnv        = 1.8f;      // clamp burst envelope
constexpr float kClapMidNoiseBlend      = 0.55f;     // proportion of LP noise in clap
constexpr float kClapBodyScale          = 0.24f;     // body mix
constexpr float kClapNoiseDirectScale   = 0.24f;     // direct noise mix
constexpr float kClapNoiseMidScale      = 0.76f;     // LP noise mix

// ---- Modal voice ----
constexpr float kGoldenRatio            = 1.6180339f;
constexpr float kSqrt6Approx            = 2.4142135f; // actually 1+sqrt(2)
constexpr float kModalGoldenPartialAmp  = 0.010f;
constexpr float kModalSqrt6PartialAmp   = 0.002f;

// ---- FM voice ----
constexpr float kFmBaseModDepth         = 0.5f;
constexpr float kFmModEnvScale          = 1.5f;
constexpr float kFmOutputScale          = 0.72f;

// ---- Pan ----
constexpr float kSqrtHalf               = 0.7071067f;

} // namespace mds::constants
