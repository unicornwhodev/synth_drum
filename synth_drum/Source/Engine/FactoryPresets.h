#pragma once

#include <array>
#include <string>
#include <vector>

#include "DrumDefs.h"

namespace mds
{

struct PadSettings
{
    float level = 0.8f;
    float tuneSemitones = 0.0f;
    float decaySeconds = 0.35f;
    float attackSeconds = 0.001f;
    float pitchDropSemitones = 0.0f;
    float pitchDecaySeconds = 0.06f;
    float noiseAmount = 0.2f;
    float clickAmount = 0.1f;
    float drive = 2.0f;
    float cutoffHz = 9000.0f;
    float pan = 0.0f;
    float clapSpread = 0.5f;
    float clapDensity = 0.5f;
    float metallicDensity = 0.5f;
    float openAmount = 0.5f;
    float bodyTone = 0.5f;
    float modalRing = 0.5f;
    float fmIndex = 0.5f;
    float fmSweep = 0.5f;
    // Audit Phase 5 D1: per-pad velocity → click amount sensitivity (0..1).
    // 0 = velocity-independent (raw click), 1 = full attenuation by velocity.
    // Default 0.6 mirrors the previous hard-coded Snare/Hat behaviour.
    float velocityToClick = 0.6f;
    // Audit Phase 5 D3: per-pad FX sends (0..1). Routed in addition to the
    // dry pad bus output. 0 = no send, 1 = unity send into the global FX bus.
    float reverbSend = 0.0f;
    float delaySend  = 0.0f;
    float baseFrequencyHz = 180.0f;
    PadVoiceModel voiceModel = PadVoiceModel::PercWood;

    // Internal engine identity. These fields are not serialized and do not
    // affect APVTS/state IDs; they let renderers and voices choose a dedicated
    // 12-pad V2 path without string comparisons in the audio loop.
    int padIndex = 0;
    DrumInstrumentAlgorithm instrumentAlgorithm = DrumInstrumentAlgorithm::KickA;
    DrumRenderEngineMode renderMode = DrumRenderEngineMode::V2;
};

struct GlobalFxSettings
{
    float outputGainDb    = -6.5f;
    float macroPunch      = 0.48f;
    float macroWeight     = 0.52f;
    float macroAir        = 0.46f;
    float macroDirt       = 0.06f;

    float compThreshold   = -11.0f;
    float compRatio       = 1.8f;
    float compAttack      = 20.0f;
    float compRelease     = 160.0f;
    float compMakeup      = 0.0f;
    float compMix         = 0.32f;

    float satDrive        = 1.08f;
    float satMix          = 0.04f;

    float transientAttack  = 0.04f;
    float transientSustain = -0.02f;
    float transientMix     = 0.12f;

    // Reverb (DattorroPlateReverb)
    float reverbSize       = 0.35f;
    float reverbDamping    = 0.70f;
    float reverbWidth      = 0.80f;
    float reverbMix        = 0.15f;
    float reverbPredelay   = 12.0f;

    // EQ
    float eqLowFreq   = 120.0f;
    float eqLowGain   = 0.0f;
    float eqMidFreq   = 1200.0f;
    float eqMidGain   = 0.0f;
    float eqMidQ      = 1.0f;
    float eqHighFreq  = 6000.0f;
    float eqHighGain  = 0.0f;
    bool  eqEnable    = false;

    // Chorus
    float chorusRate  = 1.0f;
    float chorusDepth = 0.5f;
    float chorusMix   = 0.0f;
    bool  chorusEnable = false;

    // Delay
    float delayTime     = 300.0f;
    float delayFeedback = 0.30f;
    float delayMix      = 0.0f;
    bool  delaySync     = false;
    int   delayNoteDiv  = 0;
    bool  delayEnable   = false;

    // Limiter
    float limiterThreshold = -0.3f;
    float limiterRelease   = 50.0f;
    bool  limiterEnable    = true;

    // Enable toggles for always-on FX
    bool  reverbEnable     = true;
    bool  transientEnable  = true;
    bool  saturatorEnable  = true;
    bool  compEnable       = true;

    // Aux routing
    bool  auxPostFx        = false;

    // Velocity curve (0=Linear, 1=Soft, 2=Softer, 3=Hard, 4=Harder, 5=Fixed, 6=Touch)
    int   velocityCurve    = 0;

    // Global LFO
    float lfoRate          = 2.0f;
    float lfoDepth         = 0.0f;
    int   lfoWave          = 0;  // 0=Sine, 1=Triangle, 2=Saw, 3=Square

    // Humanize
    float humanizeTimingMs = 0.0f;
    float humanizeLevel    = 0.0f;
};

// Preset version tag — increment when adding new PadSettings/GlobalFxSettings fields.
// loadUserPreset() and setStateInformation() use this to apply sensible defaults
// and sanitize older states without breaking existing parameter IDs.
static constexpr int kPresetVersion = 6;

struct KitPreset
{
    std::string name;
    std::string familyLabel;
    std::string mixRole;
    std::string description;
    std::string outputProfile;
    std::vector<std::string> tags;
    float nominalPeakDb = -6.0f;
    std::array<PadSettings, kNumPads> pads;
    GlobalFxSettings fx;
    std::array<int, kNumPads> outputBuses {};
};

// Sound design matrix helpers
struct TargetCell
{
    float level;
    float density;
};

struct TargetRow
{
    TargetCell kick;
    TargetCell snare;
    TargetCell hat;
    TargetCell crash;
    TargetCell fx;
};

enum class KitFamily : int
{
    Classique    = 0,
    Acoustique   = 1,
    Ambient      = 2,
    Cinematique  = 3,
    Moderne      = 4
};

TargetRow    getTargetRow(KitFamily family);
void         applyTargetMatrix(KitPreset& kit, KitFamily family);

struct PadPreset
{
    std::string     name;
    PadSettings     settings;
    GlobalFxSettings fx;
};

PadSettings getDefaultPadSettings(int padIndex);
const std::vector<KitPreset>&  getFactoryPresets();
const std::vector<PadPreset>&  getFactoryPadPresets(int padIndex);
} // namespace mds
