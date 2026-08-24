#include "FactoryPresets.h"
#include "DrumDefs.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace mds
{
namespace
{

// =========================================================================
// Low-level pad builder
// =========================================================================
PadSettings makePad(const float level,
                    const float tuneSemitones,
                    const float decaySeconds,
                    const float attackSeconds,
                    const float pitchDropSemitones,
                    const float pitchDecaySeconds,
                    const float noiseAmount,
                    const float clickAmount,
                    const float drive,
                    const float cutoffHz,
                    const float pan,
                    const float baseFrequencyHz,
                    const PadVoiceModel voiceModel)
{
    PadSettings pad;
    pad.level               = level;
    pad.tuneSemitones       = tuneSemitones;
    pad.decaySeconds        = decaySeconds;
    pad.attackSeconds       = attackSeconds;
    pad.pitchDropSemitones  = pitchDropSemitones;
    pad.pitchDecaySeconds   = pitchDecaySeconds;
    pad.noiseAmount         = noiseAmount;
    pad.clickAmount         = clickAmount;
    pad.drive               = drive;
    pad.cutoffHz            = cutoffHz;
    pad.pan                 = pan;
    pad.baseFrequencyHz     = baseFrequencyHz;
    pad.voiceModel          = voiceModel;

    switch (voiceModel)
    {
        case PadVoiceModel::Clap:
            pad.clapSpread = 0.42f;
            pad.clapDensity = std::clamp(0.28f + noiseAmount * 0.75f, 0.0f, 1.0f);
            break;
        case PadVoiceModel::Hat:
            pad.metallicDensity = std::clamp((cutoffHz - 4200.0f) / 7600.0f, 0.18f, 1.0f);
            pad.openAmount = std::clamp((decaySeconds - 0.015f) / 0.11f, 0.0f, 1.0f);
            break;
        case PadVoiceModel::Crash:
            pad.metallicDensity = std::clamp((cutoffHz - 5000.0f) / 7000.0f, 0.35f, 1.0f);
            pad.openAmount = std::clamp((decaySeconds - 0.08f) / 0.26f, 0.25f, 1.0f);
            break;
        case PadVoiceModel::PercWood:
            pad.bodyTone = 0.30f;
            pad.modalRing = std::clamp((decaySeconds - 0.04f) / 0.16f, 0.0f, 1.0f);
            break;
        case PadVoiceModel::PercMetal:
            pad.bodyTone = 0.68f;
            pad.modalRing = std::clamp((decaySeconds - 0.04f) / 0.16f, 0.0f, 1.0f);
            break;
        case PadVoiceModel::Fx:
            pad.fmIndex = std::clamp(pitchDropSemitones / 16.0f, 0.15f, 1.0f);
            pad.fmSweep = std::clamp(cutoffHz / 9000.0f, 0.15f, 1.0f);
            break;
        default:
            break;
    }

    return pad;
}

PadSettings withPadIdentity(PadSettings pad, const int padIndex)
{
    const int clamped = std::clamp(padIndex, 0, kNumPads - 1);
    pad.padIndex = clamped;
    pad.instrumentAlgorithm = getPadAlgorithm(clamped);
    pad.renderMode = DrumRenderEngineMode::V2;
    return pad;
}

void stampPadIdentity(PadSettings& pad, const int padIndex)
{
    pad = withPadIdentity(pad, padIndex);
}

void stampKitIdentities(KitPreset& kit)
{
    for (int pad = 0; pad < kNumPads; ++pad)
        stampPadIdentity(kit.pads[static_cast<std::size_t>(pad)], pad);
}

std::vector<PadPreset> stampPadPresetIdentities(std::vector<PadPreset> presets, const int padIndex)
{
    for (auto& preset : presets)
        stampPadIdentity(preset.settings, padIndex);
    return presets;
}

// =========================================================================
// Sound design matrix
// | Family     | Kick Level | Kick Density | Snare Level | Snare Density |
// | Hat Level  | Hat Density | Crash Level | Crash Density | FX Level | FX Density |
// =========================================================================
static const TargetRow kTargetMatrix[5] = {
    // Classique
    { {0.86f, 0.58f}, {0.73f, 0.66f}, {0.55f, 0.52f}, {0.46f, 0.43f}, {0.50f, 0.68f} },
    // Acoustique
    { {0.84f, 0.44f}, {0.71f, 0.50f}, {0.54f, 0.38f}, {0.46f, 0.34f}, {0.52f, 0.50f} },
    // Ambient
    { {0.78f, 0.40f}, {0.66f, 0.48f}, {0.52f, 0.37f}, {0.42f, 0.34f}, {0.48f, 0.52f} },
    // Cinematique
    { {0.87f, 0.64f}, {0.72f, 0.63f}, {0.54f, 0.46f}, {0.47f, 0.50f}, {0.54f, 0.72f} },
    // Moderne
    { {0.85f, 0.60f}, {0.71f, 0.61f}, {0.56f, 0.49f}, {0.46f, 0.40f}, {0.53f, 0.66f} },
};

// Density encoding: density 0..1 maps to (drive, noise, click, cutoffHz) nudges.
// Higher density → more drive, noise, click, lower cutoff (more character).
static void applyDensityToPad(PadSettings& p, float density)
{
    density = std::clamp(density, 0.0f, 1.0f);

    switch (p.voiceModel)
    {
        case PadVoiceModel::Kick:
        case PadVoiceModel::Tom:
        {
            const float densityDrive = 1.0f + density * 3.0f;
            p.drive = std::clamp(p.drive * (0.74f + density * 0.42f) + densityDrive * 0.12f, 1.0f, 8.0f);
            if (p.noiseAmount > 0.02f)
                p.noiseAmount = std::clamp(p.noiseAmount * 0.75f + density * 0.10f, 0.0f, 0.35f);
            p.cutoffHz = std::clamp(p.cutoffHz * (1.0f - density * 0.14f), 500.0f, 12000.0f);
            break;
        }

        case PadVoiceModel::Snare:
        case PadVoiceModel::Clap:
        case PadVoiceModel::Fx:
        {
            const float densityDrive = 1.0f + density * 3.5f;
            p.drive = std::clamp(p.drive * (0.70f + density * 0.55f) + densityDrive * 0.16f, 1.0f, 8.5f);
            if (p.noiseAmount > 0.03f)
                p.noiseAmount = std::clamp(p.noiseAmount * 0.62f + density * 0.22f, 0.0f, 0.90f);
            p.clickAmount = std::clamp(p.clickAmount * (0.90f + density * 0.16f), 0.0f, 1.0f);
            p.cutoffHz = std::clamp(p.cutoffHz * (1.0f - density * 0.16f), 700.0f, 16000.0f);
            if (p.voiceModel == PadVoiceModel::Clap)
            {
                p.clapDensity = std::clamp(p.clapDensity * 0.65f + density * 0.55f, 0.0f, 1.0f);
                p.clapSpread = std::clamp(p.clapSpread * (0.90f + density * 0.10f), 0.0f, 1.0f);
            }
            else if (p.voiceModel == PadVoiceModel::Fx)
            {
                p.fmIndex = std::clamp(p.fmIndex * 0.70f + density * 0.48f, 0.0f, 1.0f);
                p.fmSweep = std::clamp(p.fmSweep * 0.72f + density * 0.42f, 0.0f, 1.0f);
            }
            break;
        }

        case PadVoiceModel::Hat:
        case PadVoiceModel::Crash:
        {
            const float densityDrive = 1.0f + density * 1.2f;
            p.drive = std::clamp(p.drive * (0.90f + density * 0.16f) + densityDrive * 0.04f, 1.0f, 3.5f);
            p.noiseAmount = std::clamp(p.noiseAmount * 0.82f + density * 0.10f, 0.0f, 0.82f);
            p.clickAmount = std::clamp(p.clickAmount * (0.94f + density * 0.10f), 0.0f, 0.25f);
            p.cutoffHz = std::clamp(p.cutoffHz * (1.02f - density * 0.08f), 5000.0f, 18000.0f);
            p.metallicDensity = std::clamp(p.metallicDensity * 0.60f + density * 0.60f, 0.0f, 1.0f);
            p.openAmount = std::clamp(p.openAmount * (p.voiceModel == PadVoiceModel::Crash ? 0.78f : 0.92f)
                                      + density * (p.voiceModel == PadVoiceModel::Crash ? 0.28f : 0.06f),
                                      0.0f, 1.0f);
            break;
        }

        case PadVoiceModel::PercWood:
        {
            const float densityDrive = 1.0f + density * 1.8f;
            p.drive = std::clamp(p.drive * (0.90f + density * 0.18f) + densityDrive * 0.05f, 1.0f, 3.4f);
            p.noiseAmount = std::clamp(p.noiseAmount * 0.80f + density * 0.06f, 0.0f, 0.24f);
            p.clickAmount = std::clamp(p.clickAmount * (0.94f + density * 0.10f), 0.0f, 0.16f);
            p.pitchDropSemitones = std::clamp(p.pitchDropSemitones * (0.96f + density * 0.08f), 0.0f, 18.0f);
            p.cutoffHz = std::clamp(p.cutoffHz * (1.00f + density * 0.02f), 800.0f, 7000.0f);
            p.modalRing = std::clamp(p.modalRing * 0.72f + density * 0.35f, 0.0f, 1.0f);
            p.bodyTone = std::clamp(p.bodyTone * (0.96f + density * 0.05f), 0.0f, 1.0f);
            break;
        }

        case PadVoiceModel::PercMetal:
        {
            const float densityDrive = 1.0f + density * 1.6f;
            p.drive = std::clamp(p.drive * (0.92f + density * 0.16f) + densityDrive * 0.04f, 1.0f, 3.0f);
            p.noiseAmount = std::clamp(p.noiseAmount * 0.82f + density * 0.07f, 0.0f, 0.28f);
            p.clickAmount = std::clamp(p.clickAmount * (0.95f + density * 0.10f), 0.0f, 0.18f);
            p.pitchDropSemitones = std::clamp(p.pitchDropSemitones * (0.95f + density * 0.08f), 0.0f, 20.0f);
            p.cutoffHz = std::clamp(p.cutoffHz * (1.01f + density * 0.03f), 1400.0f, 12000.0f);
            p.modalRing = std::clamp(p.modalRing * 0.72f + density * 0.38f, 0.0f, 1.0f);
            p.bodyTone = std::clamp(p.bodyTone * 0.84f + density * 0.18f, 0.0f, 1.0f);
            break;
        }

        default:
            break;
    }
}

// =========================================================================
// Helper: build a default kit from defaults
// =========================================================================
static KitPreset buildDefaultKit()
{
    KitPreset k;
    for (int i = 0; i < kNumPads; ++i)
        k.pads[static_cast<std::size_t>(i)] = getDefaultPadSettings(i);
    return k;
}

// =========================================================================
// Kit builders — one per family variant
// =========================================================================

// ---- Classique -----------------------------------------------------------
// 3 variants: Standard, Tight, Open

static KitPreset makeKit_Classique_Standard()
{
    auto k = buildDefaultKit();
    k.name = "Classique Standard";

    // Kick A: punchy, medium decay
    k.pads[0] = makePad(0.88f, 0.0f,  0.32f, 0.0004f,  6.0f, 0.035f, 0.005f, 0.10f, 1.05f, 2800.0f,  0.0f,  132.0f, PadVoiceModel::Kick);
    // Kick B: tighter, higher tune
    k.pads[1] = makePad(0.70f, 5.0f,  0.14f, 0.0004f,  3.0f, 0.025f, 0.004f, 0.18f, 1.02f, 2400.0f,  0.0f,   96.0f, PadVoiceModel::Kick);
    // Snare: crisp, medium noise
    k.pads[2] = makePad(0.76f, 7.0f,  0.10f, 0.0002f,  0.4f, 0.012f, 0.65f,  0.06f, 1.02f, 6000.0f, -0.02f, 248.0f, PadVoiceModel::Snare);
    // Clap: natural
    k.pads[3] = makePad(0.68f,-5.0f,  0.20f, 0.0002f,  0.0f, 0.016f, 0.62f,  0.02f, 1.00f, 3400.0f,  0.04f, 300.0f, PadVoiceModel::Clap);
    // Hat Closed
    k.pads[4] = makePad(0.52f, 0.0f,  0.024f, 0.0f,    0.0f, 0.007f, 0.70f,  0.12f, 1.00f, 8500.0f, -0.12f,5500.0f, PadVoiceModel::Hat);
    // Hat Open
    k.pads[5] = makePad(0.46f, 0.0f,  0.090f, 0.0f,    0.0f, 0.009f, 0.62f,  0.08f, 1.00f, 6600.0f,  0.12f,4800.0f, PadVoiceModel::Hat);
    // Perc 1 & 2 (wood/metal timbales) — separated from Clap (300 Hz)
    k.pads[6] = makePad(0.96f,-8.0f,  0.085f, 0.0001f,  8.0f, 0.020f, 0.12f,  0.07f, 1.00f, 1400.0f, -0.18f, 480.0f, PadVoiceModel::PercWood);
    k.pads[7] = makePad(0.90f,-12.0f, 0.095f, 0.0001f, 10.0f, 0.018f, 0.12f,  0.05f, 1.00f, 1800.0f,  0.16f, 650.0f, PadVoiceModel::PercMetal);
    // Toms — separated from Kicks (90–132 Hz)
    k.pads[8] = makePad(0.60f, 0.0f,  0.125f, 0.0004f,  3.5f, 0.030f, 0.006f, 0.09f, 1.01f, 3000.0f, -0.08f, 175.0f, PadVoiceModel::Tom);
    k.pads[9] = makePad(0.56f, 0.0f,  0.105f, 0.0004f,  2.8f, 0.026f, 0.006f, 0.08f, 1.01f, 3800.0f,  0.08f, 250.0f, PadVoiceModel::Tom);
    // Crash
    k.pads[10]= makePad(0.46f, 0.0f,  0.25f,  0.0f,     0.0f, 0.012f, 0.54f,  0.06f, 1.00f, 7600.0f,  0.20f,6400.0f, PadVoiceModel::Crash);
    // FX
    k.pads[11]= makePad(0.50f, 5.0f,  0.09f,  0.0002f,  9.0f, 0.022f, 0.03f,  0.10f, 1.02f, 4000.0f, -0.14f, 720.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.40f; k.fx.reverbDamping=0.68f; k.fx.reverbWidth=0.82f; k.fx.reverbMix=0.16f; k.fx.reverbPredelay=10.0f;
    k.fx.compThreshold=-12.0f; k.fx.compRatio=2.0f; k.fx.compAttack=18.0f; k.fx.compRelease=150.0f; k.fx.compMix=0.35f;
    k.fx.satDrive=1.10f; k.fx.satMix=0.06f;
    k.fx.transientAttack=0.06f; k.fx.transientSustain=0.0f; k.fx.transientMix=0.14f;  // was -0.03f: negative only reduces, not musical
    k.fx.limiterEnable=true; k.fx.limiterThreshold=-0.5f; k.fx.limiterRelease=50.0f;
    applyTargetMatrix(k, KitFamily::Classique);
    return k;
}

static KitPreset makeKit_Classique_Tight()
{
    auto k = makeKit_Classique_Standard();
    k.name = "Classique Tight";
    for (int i = 0; i < 2; ++i)      // kicks shorter
        k.pads[static_cast<std::size_t>(i)].decaySeconds *= 0.68f;
    k.pads[2].decaySeconds *= 0.70f; // snare tighter
    k.pads[4].decaySeconds = 0.016f; // hat very tight
    k.pads[5].decaySeconds = 0.050f;
    k.fx.transientAttack = 0.12f;
    k.fx.compAttack = 8.0f;
    applyTargetMatrix(k, KitFamily::Classique);
    return k;
}

static KitPreset makeKit_Classique_Open()
{
    auto k = makeKit_Classique_Standard();
    k.name = "Classique Open";
    // Audit Phase 2.1: previous Open differed from Standard mostly by reverb;
    // signature was "same kit + more wet". Reinforce the OPEN identity by:
    //  - longer kick/snare/hat-open/crash decays (already present)
    //  - drier, less squashed compression (mix 0.20→0.10, slower attack)
    //  - wider, less damped reverb to convey larger stage
    //  - softer transient processor so cymbals breathe
    k.pads[0].decaySeconds = 0.48f;
    k.pads[2].decaySeconds = 0.145f;
    k.pads[5].decaySeconds = 0.160f;
    k.pads[10].decaySeconds= 0.42f;  // was 0.38: even more open crash
    k.fx.reverbMix     = 0.24f;  // was 0.22
    k.fx.reverbSize    = 0.62f;  // was 0.55: bigger room
    k.fx.reverbDamping = 0.55f;  // was 0.68: less HF absorption
    k.fx.reverbWidth   = 0.92f;  // was 0.82: wider stereo image
    k.fx.reverbPredelay= 22.0f;  // was 18
    k.fx.compMix       = 0.10f;  // was 0.20: dry/uncompressed feel
    k.fx.compAttack    = 28.0f;  // was 18: lets transients through
    k.fx.transientAttack = 0.04f;  // was 0.06 (inherits from Standard): softer
    k.fx.transientMix  = 0.10f;  // was 0.14: less aggressive
    applyTargetMatrix(k, KitFamily::Classique);
    return k;
}

// ---- Acoustique ----------------------------------------------------------
// 4 variants: Room, Studio, Brush, Jazz

static KitPreset makeKit_Acoustique_Room()
{
    KitPreset k = buildDefaultKit();
    k.name = "Acoustique Room";

    k.pads[0] = makePad(0.86f, 0.0f,  0.38f, 0.0005f,  4.0f, 0.040f, 0.010f, 0.12f, 1.01f, 2200.0f,  0.0f,  132.0f, PadVoiceModel::Kick);
    k.pads[1] = makePad(0.72f, 3.0f,  0.18f, 0.0005f,  2.0f, 0.030f, 0.008f, 0.22f, 1.00f, 1800.0f,  0.0f,   96.0f, PadVoiceModel::Kick);
    k.pads[2] = makePad(0.74f, 5.0f,  0.12f, 0.0003f,  0.2f, 0.014f, 0.55f,  0.05f, 1.00f, 5400.0f, -0.02f, 248.0f, PadVoiceModel::Snare);
    k.pads[3] = makePad(0.66f,-4.0f,  0.22f, 0.0003f,  0.0f, 0.018f, 0.52f,  0.02f, 1.00f, 2800.0f,  0.04f, 300.0f, PadVoiceModel::Clap);
    k.pads[4] = makePad(0.52f, 0.0f,  0.028f, 0.0f,    0.0f, 0.008f, 0.68f,  0.12f, 1.00f, 7800.0f, -0.12f,5500.0f, PadVoiceModel::Hat);
    k.pads[5] = makePad(0.48f, 0.0f,  0.11f,  0.0f,    0.0f, 0.010f, 0.60f,  0.08f, 1.00f, 6000.0f,  0.12f,4800.0f, PadVoiceModel::Hat);
    k.pads[6] = makePad(0.92f,-7.0f,  0.09f,  0.0001f, 7.0f, 0.022f, 0.10f,  0.08f, 1.00f, 1500.0f, -0.18f, 480.0f, PadVoiceModel::PercWood);
    k.pads[7] = makePad(0.88f,-11.0f, 0.10f,  0.0001f, 9.0f, 0.020f, 0.10f,  0.06f, 1.00f, 1740.0f,  0.16f, 650.0f, PadVoiceModel::PercMetal);
    k.pads[8] = makePad(0.62f, 0.0f,  0.14f,  0.0005f, 3.0f, 0.032f, 0.008f, 0.10f, 1.00f, 2800.0f, -0.08f, 175.0f, PadVoiceModel::Tom);
    k.pads[9] = makePad(0.58f, 0.0f,  0.12f,  0.0005f, 2.5f, 0.028f, 0.008f, 0.09f, 1.00f, 3400.0f,  0.08f, 250.0f, PadVoiceModel::Tom);
    k.pads[10]= makePad(0.48f, 0.0f,  0.30f,  0.0f,    0.0f, 0.014f, 0.52f,  0.06f, 1.00f, 7000.0f,  0.20f,6400.0f, PadVoiceModel::Crash);
    k.pads[11]= makePad(0.52f, 4.0f,  0.10f,  0.0002f, 7.0f, 0.024f, 0.04f,  0.09f, 1.00f, 3600.0f, -0.14f, 720.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.52f; k.fx.reverbDamping=0.62f; k.fx.reverbWidth=0.78f; k.fx.reverbMix=0.20f; k.fx.reverbPredelay=14.0f;
    k.fx.compThreshold=-14.0f; k.fx.compRatio=1.8f; k.fx.compAttack=22.0f; k.fx.compRelease=180.0f; k.fx.compMix=0.28f;
    k.fx.satDrive=1.04f; k.fx.satMix=0.03f;
    k.fx.transientAttack=0.04f; k.fx.transientSustain=0.0f; k.fx.transientMix=0.10f;  // was -0.02f: negative only reduces, not musical
    k.fx.limiterEnable=true; k.fx.limiterThreshold=-0.5f;
    applyTargetMatrix(k, KitFamily::Acoustique);
    return k;
}

static KitPreset makeKit_Acoustique_Studio()
{
    auto k = makeKit_Acoustique_Room();
    k.name = "Acoustique Studio";
    // Audit Phase 2.1: Studio was too close to Room (just less reverb).
    // Push the "produced/mixed" identity: tighter room, harder bus comp,
    // crisper transients, gentle high-shelf for sheen, presence boost on snare.
    k.fx.reverbSize=0.36f; k.fx.reverbDamping=0.78f; k.fx.reverbMix=0.13f; k.fx.reverbPredelay=8.0f;
    k.fx.compThreshold=-10.0f; k.fx.compRatio=2.5f;  // was 2.2: more glue
    k.fx.compAttack=14.0f;  // was 22: catches transients
    k.fx.compMix=0.42f;  // was 0.40: slightly more bus comp
    k.fx.transientAttack=0.10f;  // was 0.08: more snap
    k.fx.transientMix=0.18f;  // already 0.16, push to 0.18
    k.fx.satDrive=1.10f; k.fx.satMix=0.05f;  // touch of analog warmth
    // High-shelf sheen for studio mix character
    k.fx.eqEnable=true;
    k.fx.eqHighFreq=8500.0f; k.fx.eqHighGain=2.0f;
    k.fx.eqLowFreq=80.0f;    k.fx.eqLowGain=1.0f;
    // Bring snare up a touch with extra body
    k.pads[2].level *= 1.05f;
    k.pads[2].noiseAmount += 0.05f;
    k.pads[2].clickAmount += 0.04f;  // tighter attack for studio snare
    applyTargetMatrix(k, KitFamily::Acoustique);
    return k;
}

static KitPreset makeKit_Acoustique_Brush()
{
    auto k = makeKit_Acoustique_Room();
    k.name = "Acoustique Brush";
    // Audit Phase 1.4: previous Brush snare was inaudible in groove playback
    // (level 0.40, click 0.02, drive 1.00). Recalibrated for usable brush
    // articulation while keeping the airy texture characteristic of brushes.
    k.pads[2].level       = 0.78f;  // was 0.40 (effective after target matrix): now audible
    k.pads[2].noiseAmount = 0.58f;  // was 0.78: reduced, retains brush texture but keeps body
    k.pads[2].clickAmount = 0.10f;  // was 0.02: enough attack to register in mix
    k.pads[2].cutoffHz    = 4600.0f; // brighter
    k.pads[2].drive       = 1.04f;  // light saturation for warmth
    k.pads[2].decaySeconds = 0.16f; // longer for brush sweep feel
    // Softer kick
    k.pads[0].level = 0.78f; k.pads[0].drive = 1.00f;
    // Quieter hats (relative to snare which is now louder)
    k.pads[4].level = 0.46f; k.pads[5].level = 0.42f;
    k.fx.reverbMix=0.26f; k.fx.satMix=0.02f; k.fx.compMix=0.22f;
    applyTargetMatrix(k, KitFamily::Acoustique);
    return k;
}

static KitPreset makeKit_Acoustique_Jazz()
{
    auto k = makeKit_Acoustique_Room();
    k.name = "Acoustique Jazz";
    // Jazz: fast decay, light, airy
    k.pads[0].decaySeconds=0.22f; k.pads[0].level=0.78f; k.pads[0].pitchDropSemitones=2.0f;
    k.pads[2].decaySeconds=0.09f; k.pads[2].level=0.68f;
    k.pads[5].decaySeconds=0.14f; k.pads[5].level=0.50f;
    k.pads[4].level=0.54f;
    k.fx.reverbSize=0.60f; k.fx.reverbMix=0.24f; k.fx.reverbPredelay=20.0f;
    k.fx.compMix=0.15f; k.fx.satMix=0.01f;
    applyTargetMatrix(k, KitFamily::Acoustique);
    return k;
}

// ---- Ambient -------------------------------------------------------------
// 3 variants: Pad, Dark, Sparse

static KitPreset makeKit_Ambient_Pad()
{
    KitPreset k = buildDefaultKit();
    k.name = "Ambient Pad";

    k.pads[0] = makePad(0.80f, 0.0f,  0.42f, 0.0006f,  3.0f, 0.055f, 0.008f, 0.06f, 1.00f, 1800.0f,  0.0f,  132.0f, PadVoiceModel::Kick);
    k.pads[1] = makePad(0.65f, 4.0f,  0.22f, 0.0006f,  1.5f, 0.040f, 0.006f, 0.14f, 1.00f, 1400.0f,  0.0f,   96.0f, PadVoiceModel::Kick);
    k.pads[2] = makePad(0.68f, 6.0f,  0.16f, 0.0003f,  0.1f, 0.018f, 0.62f,  0.03f, 1.00f, 4800.0f, -0.04f, 248.0f, PadVoiceModel::Snare);
    k.pads[3] = makePad(0.60f,-3.0f,  0.28f, 0.0003f,  0.0f, 0.020f, 0.58f,  0.01f, 1.00f, 2400.0f,  0.04f, 300.0f, PadVoiceModel::Clap);
    k.pads[4] = makePad(0.48f, 0.0f,  0.032f, 0.0f,    0.0f, 0.008f, 0.65f,  0.10f, 1.00f, 7400.0f, -0.14f,5500.0f, PadVoiceModel::Hat);
    k.pads[5] = makePad(0.44f, 0.0f,  0.14f,  0.0f,    0.0f, 0.010f, 0.58f,  0.07f, 1.00f, 5800.0f,  0.14f,4800.0f, PadVoiceModel::Hat);
    k.pads[6] = makePad(0.88f,-6.0f,  0.12f,  0.0002f, 6.0f, 0.028f, 0.10f,  0.08f, 1.00f, 1580.0f, -0.20f, 480.0f, PadVoiceModel::PercWood);
    k.pads[7] = makePad(0.82f,-10.0f, 0.13f,  0.0002f, 8.0f, 0.024f, 0.10f,  0.06f, 1.00f, 1800.0f,  0.18f, 650.0f, PadVoiceModel::PercMetal);
    k.pads[8] = makePad(0.56f, 0.0f,  0.18f,  0.0006f, 2.5f, 0.038f, 0.008f, 0.08f, 1.00f, 2600.0f, -0.10f, 175.0f, PadVoiceModel::Tom);
    k.pads[9] = makePad(0.52f, 0.0f,  0.14f,  0.0005f, 2.0f, 0.032f, 0.008f, 0.07f, 1.00f, 3200.0f,  0.10f, 250.0f, PadVoiceModel::Tom);
    k.pads[10]= makePad(0.44f, 0.0f,  0.42f,  0.0f,    0.0f, 0.016f, 0.50f,  0.06f, 1.00f, 6600.0f,  0.22f,6400.0f, PadVoiceModel::Crash);
    k.pads[11]= makePad(0.48f, 6.0f,  0.14f,  0.0003f, 6.0f, 0.028f, 0.05f,  0.09f, 1.00f, 3200.0f, -0.16f, 720.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.68f; k.fx.reverbDamping=0.55f; k.fx.reverbWidth=0.90f; k.fx.reverbMix=0.28f; k.fx.reverbPredelay=22.0f;
    k.fx.compThreshold=-16.0f; k.fx.compRatio=1.6f; k.fx.compAttack=28.0f; k.fx.compRelease=200.0f; k.fx.compMix=0.22f;
    k.fx.satDrive=1.02f; k.fx.satMix=0.02f;
    k.fx.transientAttack=0.02f; k.fx.transientSustain=0.0f; k.fx.transientMix=0.08f;  // was -0.04f: negative sustain only attenuates, not musical
    k.fx.limiterEnable=true; k.fx.limiterThreshold=-0.5f;
    applyTargetMatrix(k, KitFamily::Ambient);
    return k;
}

static KitPreset makeKit_Ambient_Dark()
{
    auto k = makeKit_Ambient_Pad();
    k.name = "Ambient Dark";
    // Darker tones, pitched down, longer decays
    for (int i = 0; i < 2; ++i) { k.pads[static_cast<std::size_t>(i)].tuneSemitones -= 3.0f; k.pads[static_cast<std::size_t>(i)].cutoffHz *= 0.75f; }
    k.pads[2].cutoffHz *= 0.72f; k.pads[2].noiseAmount += 0.05f;
    k.pads[10].decaySeconds = 0.60f; k.pads[10].level = 0.40f;
    k.fx.reverbSize=0.60f; k.fx.reverbDamping=0.50f; k.fx.reverbMix=0.26f; k.fx.reverbPredelay=24.0f;  // was 0.75/0.45/0.32: too long, smudged transients
    k.fx.eqEnable=true; k.fx.eqLowFreq=80.0f; k.fx.eqLowGain=2.0f;
    k.fx.eqHighFreq=8000.0f; k.fx.eqHighGain=-2.5f;
    k.fx.transientSustain=0.0f;  // was -0.04f: negative only attenuates, not intentional
    applyTargetMatrix(k, KitFamily::Ambient);
    return k;
}

static KitPreset makeKit_Ambient_Sparse()
{
    auto k = makeKit_Ambient_Pad();
    k.name = "Ambient Sparse";
    // Very minimal energy — all levels pulled back, reverb dominant
    for (int i = 0; i < kNumPads; ++i)
        k.pads[static_cast<std::size_t>(i)].level *= 0.82f;
    k.fx.reverbSize=0.65f; k.fx.reverbMix=0.28f; k.fx.reverbPredelay=28.0f;  // was 0.80/0.35: less smearing
    k.fx.compMix=0.12f;
    k.fx.transientSustain=0.0f;  // was -0.04f: negative only attenuates
    applyTargetMatrix(k, KitFamily::Ambient);
    return k;
}

// ---- Cinematique ---------------------------------------------------------
// 4 variants: Epic, Tension, Hybrid, Percussion

static KitPreset makeKit_Cinematique_Epic()
{
    KitPreset k = buildDefaultKit();
    k.name = "Cinematique Epic";

    k.pads[0] = makePad(0.90f,-2.0f,  0.40f, 0.0003f,  8.0f, 0.040f, 0.006f, 0.12f, 1.08f, 2200.0f,  0.0f,  132.0f, PadVoiceModel::Kick);
    k.pads[1] = makePad(0.75f, 3.0f,  0.22f, 0.0003f,  4.0f, 0.032f, 0.005f, 0.20f, 1.04f, 1800.0f,  0.0f,   96.0f, PadVoiceModel::Kick);
    k.pads[2] = makePad(0.76f, 8.0f,  0.11f, 0.0002f,  0.5f, 0.012f, 0.60f,  0.08f, 1.04f, 5800.0f, -0.02f, 248.0f, PadVoiceModel::Snare);
    k.pads[3] = makePad(0.70f,-5.0f,  0.22f, 0.0002f,  0.0f, 0.016f, 0.58f,  0.02f, 1.00f, 3000.0f,  0.04f, 300.0f, PadVoiceModel::Clap);
    k.pads[4] = makePad(0.56f, 0.0f,  0.022f, 0.0f,    0.0f, 0.007f, 0.66f,  0.12f, 1.00f, 8400.0f, -0.12f,5500.0f, PadVoiceModel::Hat);
    k.pads[5] = makePad(0.50f, 0.0f,  0.10f,  0.0f,    0.0f, 0.008f, 0.58f,  0.08f, 1.00f, 6400.0f,  0.12f,4800.0f, PadVoiceModel::Hat);
    k.pads[6] = makePad(1.00f,-8.0f,  0.09f,  0.0001f, 9.0f, 0.022f, 0.10f,  0.10f, 1.02f, 1280.0f, -0.18f, 480.0f, PadVoiceModel::PercWood);
    k.pads[7] = makePad(0.96f,-12.0f, 0.10f,  0.0001f,12.0f, 0.020f, 0.10f,  0.08f, 1.02f, 1500.0f,  0.16f, 650.0f, PadVoiceModel::PercMetal);
    k.pads[8] = makePad(0.65f,-2.0f,  0.15f,  0.0004f, 4.0f, 0.034f, 0.008f, 0.11f, 1.02f, 2600.0f, -0.10f, 175.0f, PadVoiceModel::Tom);
    k.pads[9] = makePad(0.60f,-1.0f,  0.12f,  0.0004f, 3.0f, 0.028f, 0.008f, 0.10f, 1.02f, 3600.0f,  0.10f, 250.0f, PadVoiceModel::Tom);
    k.pads[10]= makePad(0.50f, 0.0f,  0.30f,  0.0f,    0.0f, 0.013f, 0.50f,  0.06f, 1.00f, 7200.0f,  0.22f,6400.0f, PadVoiceModel::Crash);
    k.pads[11]= makePad(0.56f, 8.0f,  0.10f,  0.0002f,10.0f, 0.025f, 0.04f,  0.12f, 1.04f, 3800.0f, -0.14f, 720.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.62f; k.fx.reverbDamping=0.50f; k.fx.reverbWidth=0.92f; k.fx.reverbMix=0.22f; k.fx.reverbPredelay=20.0f;
    k.fx.compThreshold=-10.0f; k.fx.compRatio=2.5f; k.fx.compAttack=12.0f; k.fx.compRelease=140.0f; k.fx.compMix=0.45f;
    k.fx.satDrive=1.20f; k.fx.satMix=0.10f;
    k.fx.transientAttack=0.10f; k.fx.transientSustain=0.0f; k.fx.transientMix=0.18f;  // was -0.04f: negative only reduces, not musical
    k.fx.eqEnable=true; k.fx.eqLowFreq=90.0f; k.fx.eqLowGain=2.5f; k.fx.eqHighFreq=7000.0f; k.fx.eqHighGain=1.2f;
    k.fx.limiterEnable=true; k.fx.limiterThreshold=-0.5f;
    applyTargetMatrix(k, KitFamily::Cinematique);
    return k;
}

static KitPreset makeKit_Cinematique_Tension()
{
    auto k = makeKit_Cinematique_Epic();
    k.name = "Cinematique Tension";
    // Sharp, aggressive — high drive, fast attack compressor
    k.pads[2].drive=1.12f; k.pads[2].noiseAmount+=0.06f;
    k.pads[0].drive=1.18f; k.pads[0].pitchDropSemitones=10.0f;
    k.fx.compThreshold=-8.0f; k.fx.compAttack=6.0f; k.fx.compMix=0.55f;
    k.fx.satDrive=1.40f; k.fx.satMix=0.15f;
    k.fx.reverbMix=0.14f; k.fx.reverbSize=0.45f;
    applyTargetMatrix(k, KitFamily::Cinematique);
    return k;
}

static KitPreset makeKit_Cinematique_Hybrid()
{
    auto k = makeKit_Cinematique_Epic();
    k.name = "Cinematique Hybrid";
    // Audit Phase 2.1: Hybrid was barely distinguishable from Epic (just a
    // delay added). Reposition as the "reverb-dominant / electronic-blend"
    // member of the family: large lush reverb, softer transients, electronic
    // kick layer, longer crash tail.
    // Electronic kick blended
    k.pads[1].decaySeconds=0.14f; k.pads[1].pitchDropSemitones=8.0f; k.pads[1].noiseAmount=0.002f;
    k.pads[1].cutoffHz=1600.0f;
    // Layered snare: extra click + softer body for hybrid feel
    k.pads[2].clickAmount=0.14f;
    k.pads[2].decaySeconds=0.13f;
    k.pads[10].decaySeconds=0.42f;  // longer crash tail
    // Reverb-dominant signature
    k.fx.reverbSize=0.72f;       // was 0.62: larger hall
    k.fx.reverbDamping=0.42f;    // was 0.50: brighter tail
    k.fx.reverbWidth=0.95f;
    k.fx.reverbMix=0.42f;        // was 0.18: dominant wet
    k.fx.reverbPredelay=26.0f;
    // Soften transients/comp/sat to let reverb breathe
    k.fx.transientAttack=0.06f;  // was 0.10
    k.fx.transientMix=0.12f;     // was 0.18
    k.fx.compMix=0.30f;          // was 0.45
    k.fx.satMix=0.05f;           // was 0.10
    // Tempo-locked delay for hybrid texture
    k.fx.delayEnable=true; k.fx.delayTime=250.0f; k.fx.delayFeedback=0.28f; k.fx.delayMix=0.16f;
    applyTargetMatrix(k, KitFamily::Cinematique);
    return k;
}

static KitPreset makeKit_Cinematique_Percussion()
{
    auto k = makeKit_Cinematique_Epic();
    k.name = "Cinematique Percussion";
    // Perc-forward, toms louder
    k.pads[6].level=1.00f; k.pads[7].level=1.00f;
    k.pads[8].level=0.70f; k.pads[8].decaySeconds=0.18f;
    k.pads[9].level=0.66f; k.pads[9].decaySeconds=0.15f;
    k.pads[0].level=0.80f; k.pads[1].level=0.65f;
    k.fx.reverbSize=0.58f; k.fx.reverbMix=0.25f;
    k.fx.eqLowFreq=100.0f; k.fx.eqLowGain=3.0f;
    applyTargetMatrix(k, KitFamily::Cinematique);
    return k;
}

// ---- Moderne -------------------------------------------------------------
// 4 variants: Club, Lo-Fi, Trap, Electro

static KitPreset makeKit_Moderne_Club()
{
    KitPreset k = buildDefaultKit();
    k.name = "Moderne Club";

    k.pads[0] = makePad(0.88f, 0.0f,  0.28f, 0.0003f,  7.0f, 0.030f, 0.003f, 0.10f, 1.06f, 2600.0f,  0.0f,  132.0f, PadVoiceModel::Kick);
    k.pads[1] = makePad(0.70f, 6.0f,  0.12f, 0.0003f,  3.5f, 0.022f, 0.003f, 0.22f, 1.02f, 2000.0f,  0.0f,   96.0f, PadVoiceModel::Kick);
    k.pads[2] = makePad(0.74f, 9.0f,  0.08f, 0.0002f,  0.3f, 0.010f, 0.66f,  0.07f, 1.02f, 6400.0f, -0.02f, 248.0f, PadVoiceModel::Snare);
    k.pads[3] = makePad(0.66f,-6.0f,  0.18f, 0.0002f,  0.0f, 0.014f, 0.64f,  0.01f, 1.00f, 3600.0f,  0.04f, 300.0f, PadVoiceModel::Clap);
    k.pads[4] = makePad(0.54f, 0.0f,  0.020f, 0.0f,    0.0f, 0.007f, 0.72f,  0.12f, 1.00f, 8800.0f, -0.12f,5500.0f, PadVoiceModel::Hat);
    k.pads[5] = makePad(0.48f, 0.0f,  0.080f, 0.0f,    0.0f, 0.008f, 0.64f,  0.08f, 1.00f, 7000.0f,  0.12f,4800.0f, PadVoiceModel::Hat);
    k.pads[6] = makePad(0.96f,-10.0f, 0.075f, 0.0001f, 8.0f, 0.018f, 0.12f,  0.08f, 1.00f, 1230.0f, -0.18f, 480.0f, PadVoiceModel::PercWood);
    k.pads[7] = makePad(0.92f,-14.0f, 0.085f, 0.0001f,10.0f, 0.016f, 0.12f,  0.06f, 1.00f, 1440.0f,  0.16f, 650.0f, PadVoiceModel::PercMetal);
    k.pads[8] = makePad(0.58f, 0.0f,  0.11f,  0.0004f, 3.5f, 0.026f, 0.005f, 0.09f, 1.01f, 3000.0f, -0.08f, 175.0f, PadVoiceModel::Tom);
    k.pads[9] = makePad(0.54f, 0.0f,  0.09f,  0.0004f, 3.0f, 0.022f, 0.005f, 0.08f, 1.01f, 3800.0f,  0.08f, 250.0f, PadVoiceModel::Tom);
    k.pads[10]= makePad(0.44f, 0.0f,  0.22f,  0.0f,    0.0f, 0.012f, 0.55f,  0.06f, 1.00f, 7800.0f,  0.20f,6400.0f, PadVoiceModel::Crash);
    k.pads[11]= makePad(0.50f, 7.0f,  0.07f,  0.0002f, 9.0f, 0.020f, 0.02f,  0.10f, 1.02f, 4400.0f, -0.14f, 720.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.34f; k.fx.reverbDamping=0.74f; k.fx.reverbWidth=0.82f; k.fx.reverbMix=0.13f; k.fx.reverbPredelay=8.0f;
    k.fx.compThreshold=-11.0f; k.fx.compRatio=2.2f; k.fx.compAttack=14.0f; k.fx.compRelease=130.0f; k.fx.compMix=0.42f;
    k.fx.satDrive=1.18f; k.fx.satMix=0.08f;
    k.fx.transientAttack=0.09f; k.fx.transientSustain=0.0f; k.fx.transientMix=0.16f;  // was -0.04f: negative only reduces, not musical
    k.fx.limiterEnable=true; k.fx.limiterThreshold=-0.5f;
    applyTargetMatrix(k, KitFamily::Moderne);
    return k;
}

static KitPreset makeKit_Moderne_LoFi()
{
    auto k = makeKit_Moderne_Club();
    k.name = "Moderne Lo-Fi";
    // Lo-fi character: reduced clarity, vinyl-like
    for (int i = 0; i < kNumPads; ++i)
    {
        k.pads[static_cast<std::size_t>(i)].cutoffHz *= 0.68f;
        k.pads[static_cast<std::size_t>(i)].drive    *= 1.08f;
    }
    k.fx.satDrive=1.6f; k.fx.satMix=0.22f;  // was 2.2f/0.25f: too dark/destroyed transients
    k.fx.eqEnable=true; k.fx.eqHighFreq=5000.0f; k.fx.eqHighGain=-2.0f;  // was -4.0f: killed all top-end
    k.fx.eqLowFreq=200.0f; k.fx.eqLowGain=-0.5f;  // was -1.0f: removed too much body
    k.fx.reverbSize=0.44f; k.fx.reverbMix=0.18f;
    k.fx.limiterThreshold=-1.5f;
    applyTargetMatrix(k, KitFamily::Moderne);
    return k;
}

static KitPreset makeKit_Moderne_Trap()
{
    auto k = makeKit_Moderne_Club();
    k.name = "Moderne Trap";
    // Trap: 808 sub kick, very punchy snare, hi-hat rolls
    // Audit Phase 1.4: previous decay 0.38s + pitchDrop 12st caused
    // low-end accumulation in 8th-note trap patterns at 70-80 BPM
    // (47% residual amplitude at next kick). Reduced for groove clarity
    // while preserving the 808 sub character.
    k.pads[0].decaySeconds=0.26f;  // was 0.38: ~28% residual at 80 BPM 8th-notes
    k.pads[0].pitchDropSemitones=9.0f;  // was 12: tighter low-end definition
    k.pads[0].cutoffHz=1500.0f;  // slightly cleaner low-end
    k.pads[0].level=0.92f;
    // Audit Phase 2.2: Trap-specific Kick B repurposing.
    // In Club/LoFi/Electro, Kick B (+6 st) acts as a high-tuned top-click
    // layer over Kick A. For Trap, the 808-style design benefits more from
    // a sub-octave layer (Kick B as deep sub doubling Kick A). This change
    // is preset-local; other Moderne kits keep the top-click design.
    k.pads[1].tuneSemitones = -2.0f;        // was +6: sub layer for 808 doubling
    k.pads[1].decaySeconds  = 0.34f;        // longer sub tail
    k.pads[1].pitchDropSemitones = 6.0f;    // gentler drop, deeper feel
    k.pads[1].cutoffHz      = 1200.0f;      // dark sub
    k.pads[1].level         = 0.74f;
    k.pads[2].decaySeconds=0.10f;  // was 0.06f: too short, no body. 100ms = punchy but audible
    k.pads[2].noiseAmount=0.66f;  // was 0.72f: still gritty but defined
    k.pads[2].drive=1.06f;  // was 1.08f: slightly less flabby
    k.pads[3].noiseAmount=0.64f; k.pads[3].level=0.70f;  // was 0.70/0.72
    k.pads[4].decaySeconds=0.016f; k.pads[4].level=0.52f;
    k.pads[5].decaySeconds=0.055f;
    k.fx.compThreshold=-9.0f; k.fx.compRatio=2.8f; k.fx.compAttack=10.0f; k.fx.compMix=0.48f;  // balanced
    k.fx.transientAttack=0.10f; k.fx.transientMix=0.18f;
    applyTargetMatrix(k, KitFamily::Moderne);
    return k;
}

static KitPreset makeKit_Moderne_Electro()
{
    auto k = makeKit_Moderne_Club();
    k.name = "Moderne Electro";
    // Electro: metallic, synthetic, chorus on hats
    k.pads[0].cutoffHz=2000.0f; k.pads[0].drive=1.12f;
    k.pads[7].level=1.00f; k.pads[7].decaySeconds=0.12f; k.pads[7].pitchDropSemitones=14.0f;
    k.fx.chorusEnable=true; k.fx.chorusRate=2.0f; k.fx.chorusDepth=0.6f; k.fx.chorusMix=0.18f;
    k.fx.delayEnable=true; k.fx.delaySync=true; k.fx.delayNoteDiv=1; k.fx.delayFeedback=0.28f; k.fx.delayMix=0.14f;
    k.fx.reverbSize=0.28f; k.fx.reverbMix=0.10f;
    applyTargetMatrix(k, KitFamily::Moderne);
    return k;
}

// =========================================================================
// Per-pad presets — 20 per slot, with per-preset FX
// =========================================================================

// FX profile helpers — reusable baselines for per-preset effects
static GlobalFxSettings fxDefault()
{
    GlobalFxSettings fx;
    fx.limiterEnable = true;
    fx.limiterThreshold = -0.5f;
    return fx;
}

static GlobalFxSettings fxPunch()
{
    auto fx = fxDefault();
    fx.compThreshold = -14.0f; fx.compRatio = 2.8f; fx.compAttack = 8.0f;
    fx.compRelease = 100.0f; fx.compMix = 0.40f;
    fx.transientAttack = 0.18f; fx.transientSustain = -0.02f; fx.transientMix = 0.22f;
    fx.satDrive = 1.08f; fx.satMix = 0.04f;
    return fx;
}

static GlobalFxSettings fxWarm()
{
    auto fx = fxDefault();
    fx.satDrive = 1.30f; fx.satMix = 0.14f;
    fx.compThreshold = -16.0f; fx.compRatio = 1.8f; fx.compAttack = 20.0f;
    fx.compRelease = 180.0f; fx.compMix = 0.25f;
    fx.eqEnable = true; fx.eqLowFreq = 100.0f; fx.eqLowGain = 2.0f;
    fx.eqHighFreq = 6000.0f; fx.eqHighGain = -1.5f;
    return fx;
}

static GlobalFxSettings fxRoom()
{
    auto fx = fxDefault();
    fx.reverbSize = 0.45f; fx.reverbDamping = 0.60f; fx.reverbWidth = 0.80f;
    fx.reverbMix = 0.22f; fx.reverbPredelay = 14.0f;
    fx.compThreshold = -15.0f; fx.compRatio = 1.8f; fx.compMix = 0.20f;
    return fx;
}

static GlobalFxSettings fxCrunch()
{
    auto fx = fxDefault();
    fx.satDrive = 1.80f; fx.satMix = 0.20f;
    fx.compThreshold = -10.0f; fx.compRatio = 3.5f; fx.compAttack = 6.0f;
    fx.compRelease = 80.0f; fx.compMix = 0.50f;
    fx.transientAttack = 0.12f; fx.transientMix = 0.16f;
    return fx;
}

static GlobalFxSettings fxDist()
{
    auto fx = fxDefault();
    fx.satDrive = 2.40f; fx.satMix = 0.30f;
    fx.compThreshold = -8.0f; fx.compRatio = 4.0f; fx.compAttack = 4.0f;
    fx.compRelease = 60.0f; fx.compMix = 0.55f;
    fx.transientAttack = 0.10f; fx.transientMix = 0.14f;
    return fx;
}

static GlobalFxSettings fxClean()
{
    auto fx = fxDefault();
    fx.compThreshold = -18.0f; fx.compRatio = 1.5f; fx.compAttack = 24.0f;
    fx.compRelease = 200.0f; fx.compMix = 0.12f;
    fx.satDrive = 1.04f; fx.satMix = 0.02f;
    return fx;
}

static GlobalFxSettings fxHall()
{
    auto fx = fxDefault();
    fx.reverbSize = 0.65f; fx.reverbDamping = 0.50f; fx.reverbWidth = 0.90f;
    fx.reverbMix = 0.35f; fx.reverbPredelay = 22.0f;
    fx.compThreshold = -16.0f; fx.compRatio = 1.6f; fx.compMix = 0.15f;
    return fx;
}

static GlobalFxSettings fxElectro()
{
    auto fx = fxDefault();
    fx.chorusEnable = true; fx.chorusRate = 2.2f; fx.chorusDepth = 0.5f; fx.chorusMix = 0.15f;
    fx.compThreshold = -12.0f; fx.compRatio = 2.5f; fx.compAttack = 10.0f;
    fx.compRelease = 120.0f; fx.compMix = 0.35f;
    fx.satDrive = 1.15f; fx.satMix = 0.06f;
    return fx;
}

static GlobalFxSettings fxTransient()
{
    auto fx = fxDefault();
    fx.transientAttack = 0.25f; fx.transientSustain = -0.04f; fx.transientMix = 0.28f;
    fx.compThreshold = -14.0f; fx.compRatio = 2.2f; fx.compMix = 0.25f;
    return fx;
}

static GlobalFxSettings fxSub()
{
    auto fx = fxDefault();
    fx.eqEnable = true; fx.eqLowFreq = 80.0f; fx.eqLowGain = 3.0f;
    fx.eqHighFreq = 4000.0f; fx.eqHighGain = -3.0f;
    fx.compThreshold = -12.0f; fx.compRatio = 2.5f; fx.compMix = 0.30f;
    fx.satDrive = 1.06f; fx.satMix = 0.03f;
    return fx;
}

static std::vector<PadPreset> makeKickPresets()
{
    const float f = kPadCharacteristics[0].baseFrequencyHz;
    return {
        { "Kick Deep",
          makePad(0.90f, 0.0f, 0.45f, 0.0006f, 5.0f, 0.048f, 0.002f, 0.05f, 1.00f, 1800.0f, 0.0f, f, PadVoiceModel::Kick),
          fxSub() },
        { "Kick Punchy",
          makePad(0.88f, 1.0f, 0.24f, 0.0002f, 9.0f, 0.024f, 0.004f, 0.16f, 1.08f, 3200.0f, 0.0f, f, PadVoiceModel::Kick),
          fxPunch() },
        { "Kick Tight",
          makePad(0.86f, 2.0f, 0.12f, 0.0002f, 4.0f, 0.016f, 0.005f, 0.22f, 1.06f, 3800.0f, 0.0f, f, PadVoiceModel::Kick),
          fxTransient() },
        { "Kick 808",
          makePad(0.94f, -2.0f, 0.65f, 0.0008f, 16.0f, 0.065f, 0.001f, 0.03f, 1.00f, 1000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxSub() },
        { "Kick Click",
          makePad(0.82f, 3.0f, 0.18f, 0.0001f, 6.0f, 0.020f, 0.002f, 0.32f, 1.04f, 4400.0f, 0.0f, f, PadVoiceModel::Kick),
          fxTransient() },
        { "Kick Room",
          makePad(0.84f, 0.0f, 0.40f, 0.0005f, 4.0f, 0.040f, 0.010f, 0.10f, 1.02f, 2200.0f, 0.0f, f, PadVoiceModel::Kick),
          fxRoom() },
        { "Kick Sub",
          makePad(0.96f, -5.0f, 0.70f, 0.001f, 18.0f, 0.072f, 0.001f, 0.02f, 1.00f, 800.0f, 0.0f, f, PadVoiceModel::Kick),
          fxSub() },
        { "Kick Layered",
          makePad(0.88f, 1.5f, 0.28f, 0.0003f, 7.0f, 0.030f, 0.006f, 0.18f, 1.06f, 2800.0f, 0.0f, f, PadVoiceModel::Kick),
          fxPunch() },
        { "Kick Soft",
          makePad(0.72f, -1.0f, 0.38f, 0.0008f, 2.5f, 0.042f, 0.002f, 0.03f, 1.00f, 1600.0f, 0.0f, f, PadVoiceModel::Kick),
          fxClean() },
        { "Kick Distort",
          makePad(0.86f, 0.0f, 0.20f, 0.0002f, 7.0f, 0.022f, 0.010f, 0.12f, 2.80f, 2600.0f, 0.0f, f, PadVoiceModel::Kick),
          fxDist() },
        { "Kick Acoustic",
          makePad(0.80f, 0.5f, 0.35f, 0.0006f, 4.0f, 0.038f, 0.015f, 0.10f, 1.04f, 2000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxRoom() },
        { "Kick Electro",
          makePad(0.88f, 2.0f, 0.16f, 0.0002f, 12.0f, 0.018f, 0.002f, 0.14f, 1.10f, 3600.0f, 0.0f, f, PadVoiceModel::Kick),
          fxElectro() },
        { "Kick Muffled",
          makePad(0.82f, -1.5f, 0.42f, 0.0007f, 4.0f, 0.046f, 0.003f, 0.04f, 1.02f, 1200.0f, 0.0f, f, PadVoiceModel::Kick),
          fxClean() },
        { "Kick Resonant",
          makePad(0.86f, 0.0f, 0.48f, 0.0005f, 8.0f, 0.055f, 0.004f, 0.08f, 1.06f, 2000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxWarm() },
        { "Kick Slap",
          makePad(0.84f, 3.5f, 0.10f, 0.0001f, 6.0f, 0.014f, 0.003f, 0.35f, 1.08f, 4000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxPunch() },
        { "Kick Boomy",
          makePad(0.92f, -3.0f, 0.58f, 0.0008f, 10.0f, 0.062f, 0.002f, 0.04f, 1.00f, 900.0f, 0.0f, f, PadVoiceModel::Kick),
          fxHall() },
        { "Kick Pop",
          makePad(0.86f, 1.0f, 0.22f, 0.0003f, 7.5f, 0.026f, 0.004f, 0.15f, 1.06f, 3000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxPunch() },
        { "Kick Industrial",
          makePad(0.88f, -0.5f, 0.18f, 0.0002f, 9.0f, 0.020f, 0.014f, 0.10f, 3.20f, 2400.0f, 0.0f, f, PadVoiceModel::Kick),
          fxDist() },
        { "Kick Warm",
          makePad(0.84f, -0.5f, 0.38f, 0.0006f, 3.5f, 0.042f, 0.004f, 0.07f, 1.08f, 1500.0f, 0.0f, f, PadVoiceModel::Kick),
          fxWarm() },
        { "Kick Ghost",
          makePad(0.60f, 0.0f, 0.15f, 0.0005f, 2.0f, 0.020f, 0.002f, 0.02f, 1.00f, 2200.0f, 0.0f, f, PadVoiceModel::Kick),
          fxClean() },
    };
}

static std::vector<PadPreset> makeKickBPresets()
{
    const float f = kPadCharacteristics[1].baseFrequencyHz;
    return {
        { "Kick B Short",
          makePad(0.70f, 6.0f, 0.10f, 0.0002f, 3.0f, 0.018f, 0.003f, 0.24f, 1.04f, 2800.0f, 0.0f, f, PadVoiceModel::Kick),
          fxTransient() },
        { "Kick B Mid",
          makePad(0.72f, 5.0f, 0.16f, 0.0003f, 2.5f, 0.024f, 0.003f, 0.20f, 1.02f, 2400.0f, 0.0f, f, PadVoiceModel::Kick),
          fxPunch() },
        { "Kick B Accent",
          makePad(0.82f, 4.0f, 0.14f, 0.0002f, 5.0f, 0.020f, 0.004f, 0.28f, 1.08f, 3200.0f, 0.0f, f, PadVoiceModel::Kick),
          fxPunch() },
        { "Kick B Hard",
          makePad(0.76f, 7.0f, 0.11f, 0.0001f, 6.0f, 0.016f, 0.003f, 0.34f, 1.12f, 3600.0f, 0.0f, f, PadVoiceModel::Kick),
          fxCrunch() },
        { "Kick B Jazzy",
          makePad(0.62f, 3.0f, 0.24f, 0.0006f, 1.2f, 0.034f, 0.010f, 0.16f, 1.00f, 1600.0f, 0.0f, f, PadVoiceModel::Kick),
          fxRoom() },
        { "Kick B Boomy",
          makePad(0.74f, 2.0f, 0.32f, 0.0007f, 2.0f, 0.042f, 0.004f, 0.12f, 1.01f, 1400.0f, 0.0f, f, PadVoiceModel::Kick),
          fxHall() },
        { "Kick B Soft",
          makePad(0.58f, 4.0f, 0.18f, 0.0006f, 1.5f, 0.028f, 0.002f, 0.10f, 1.00f, 1800.0f, 0.0f, f, PadVoiceModel::Kick),
          fxClean() },
        { "Kick B Snap",
          makePad(0.72f, 6.5f, 0.08f, 0.0001f, 4.0f, 0.014f, 0.002f, 0.30f, 1.06f, 3800.0f, 0.0f, f, PadVoiceModel::Kick),
          fxTransient() },
        { "Kick B Dist",
          makePad(0.74f, 5.0f, 0.13f, 0.0002f, 4.5f, 0.020f, 0.008f, 0.18f, 2.60f, 2600.0f, 0.0f, f, PadVoiceModel::Kick),
          fxDist() },
        { "Kick B Sub",
          makePad(0.78f, 1.0f, 0.36f, 0.0008f, 4.0f, 0.044f, 0.002f, 0.08f, 1.00f, 1200.0f, 0.0f, f, PadVoiceModel::Kick),
          fxSub() },
        { "Kick B Clicky",
          makePad(0.70f, 7.5f, 0.10f, 0.0001f, 3.5f, 0.016f, 0.002f, 0.36f, 1.08f, 4000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxTransient() },
        { "Kick B Fat",
          makePad(0.76f, 3.5f, 0.26f, 0.0004f, 3.0f, 0.036f, 0.005f, 0.14f, 1.04f, 2000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxWarm() },
        { "Kick B Ring",
          makePad(0.72f, 4.5f, 0.34f, 0.0006f, 2.0f, 0.044f, 0.003f, 0.10f, 1.02f, 1500.0f, 0.0f, f, PadVoiceModel::Kick),
          fxHall() },
        { "Kick B Attack",
          makePad(0.74f, 6.0f, 0.09f, 0.0001f, 7.0f, 0.012f, 0.002f, 0.30f, 1.10f, 4200.0f, 0.0f, f, PadVoiceModel::Kick),
          fxPunch() },
        { "Kick B Loose",
          makePad(0.66f, 3.0f, 0.28f, 0.0006f, 1.8f, 0.038f, 0.006f, 0.12f, 1.01f, 1700.0f, 0.0f, f, PadVoiceModel::Kick),
          fxRoom() },
        { "Kick B Dry",
          makePad(0.70f, 5.0f, 0.12f, 0.0002f, 3.0f, 0.020f, 0.002f, 0.18f, 1.04f, 2600.0f, 0.0f, f, PadVoiceModel::Kick),
          fxClean() },
        { "Kick B Pop",
          makePad(0.72f, 4.0f, 0.15f, 0.0003f, 4.0f, 0.024f, 0.003f, 0.22f, 1.06f, 2800.0f, 0.0f, f, PadVoiceModel::Kick),
          fxPunch() },
        { "Kick B Analog",
          makePad(0.70f, 3.5f, 0.22f, 0.0005f, 2.5f, 0.032f, 0.005f, 0.14f, 1.06f, 2000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxWarm() },
        { "Kick B Tight 2",
          makePad(0.68f, 6.0f, 0.07f, 0.0001f, 4.5f, 0.012f, 0.002f, 0.26f, 1.06f, 4000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxTransient() },
        { "Kick B Ghost",
          makePad(0.50f, 5.0f, 0.13f, 0.0005f, 1.8f, 0.024f, 0.002f, 0.08f, 1.00f, 2000.0f, 0.0f, f, PadVoiceModel::Kick),
          fxClean() },
    };
}

static std::vector<PadPreset> makeSnarePresets()
{
    const float f = kPadCharacteristics[2].baseFrequencyHz;
    return {
        { "Snare Crack",
          makePad(0.82f, 12.0f, 0.03f, 0.0001f, 0.5f, 0.004f, 0.90f, 0.20f, 1.40f, 11000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxTransient() },
        { "Snare Fat",
          makePad(0.80f, 3.0f, 0.22f, 0.0004f, 0.8f, 0.024f, 0.30f, 0.04f, 1.20f, 3200.0f, -0.02f, f, PadVoiceModel::Snare),
          fxWarm() },
        { "Snare Gated",
          makePad(0.76f, 8.0f, 0.15f, 0.0002f, 1.0f, 0.012f, 0.80f, 0.08f, 1.80f, 6000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxCrunch() },
        { "Snare Brush",
          makePad(0.68f, 2.0f, 0.30f, 0.0005f, 0.3f, 0.028f, 0.92f, 0.01f, 1.00f, 3000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxRoom() },
        { "Snare 808",
          makePad(0.74f, -2.0f, 0.18f, 0.0003f, 0.5f, 0.020f, 0.20f, 0.03f, 1.10f, 2800.0f, -0.02f, f, PadVoiceModel::Snare),
          fxSub() },
        { "Snare Tight",
          makePad(0.76f, 10.0f, 0.03f, 0.0001f, 0.2f, 0.004f, 0.85f, 0.18f, 1.10f, 10000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxClean() },
        { "Snare Ring",
          makePad(0.72f, 6.0f, 0.35f, 0.0004f, 1.5f, 0.030f, 0.50f, 0.04f, 1.10f, 3500.0f, -0.02f, f, PadVoiceModel::Snare),
          fxHall() },
        { "Snare Dist",
          makePad(0.82f, 7.0f, 0.10f, 0.0002f, 0.8f, 0.010f, 0.75f, 0.10f, 3.20f, 6500.0f, -0.02f, f, PadVoiceModel::Snare),
          fxDist() },
        { "Snare Side",
          makePad(0.72f, 14.0f, 0.04f, 0.0001f, 0.0f, 0.004f, 0.15f, 0.25f, 1.20f, 12000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxClean() },
        { "Snare Dark",
          makePad(0.70f, -4.0f, 0.25f, 0.0004f, 0.5f, 0.024f, 0.45f, 0.02f, 1.05f, 2000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxWarm() },
        { "Snare Pop",
          makePad(0.78f, 8.0f, 0.06f, 0.0002f, 0.4f, 0.007f, 0.55f, 0.15f, 1.30f, 7500.0f, -0.02f, f, PadVoiceModel::Snare),
          fxPunch() },
        { "Snare Lo-Fi",
          makePad(0.72f, 4.0f, 0.12f, 0.0003f, 0.6f, 0.012f, 0.70f, 0.06f, 2.00f, 2400.0f, -0.02f, f, PadVoiceModel::Snare),
          fxWarm() },
        { "Snare Ghost",
          makePad(0.40f, 6.0f, 0.05f, 0.0003f, 0.2f, 0.006f, 0.35f, 0.02f, 1.00f, 5000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxClean() },
        { "Snare Layered",
          makePad(0.76f, 5.0f, 0.28f, 0.0004f, 0.8f, 0.026f, 0.78f, 0.05f, 1.10f, 5000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxHall() },
        { "Snare Clap",
          makePad(0.78f, 10.0f, 0.12f, 0.0002f, 0.3f, 0.010f, 0.82f, 0.12f, 1.10f, 8000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxTransient() },
        { "Snare Metal",
          makePad(0.74f, 12.0f, 0.07f, 0.0001f, 1.5f, 0.007f, 0.60f, 0.10f, 2.50f, 8500.0f, -0.02f, f, PadVoiceModel::Snare),
          fxCrunch() },
        { "Snare Wood",
          makePad(0.70f, 0.0f, 0.14f, 0.0003f, 0.3f, 0.014f, 0.25f, 0.04f, 1.05f, 4500.0f, -0.02f, f, PadVoiceModel::Snare),
          fxRoom() },
        { "Snare Boom",
          makePad(0.84f, -3.0f, 0.40f, 0.0005f, 2.0f, 0.036f, 0.30f, 0.03f, 1.10f, 2200.0f, -0.02f, f, PadVoiceModel::Snare),
          fxSub() },
        { "Snare Punch",
          makePad(0.80f, 9.0f, 0.05f, 0.0001f, 0.3f, 0.005f, 0.60f, 0.22f, 1.50f, 9000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxPunch() },
        { "Snare Wash",
          makePad(0.66f, 3.0f, 0.38f, 0.0005f, 1.0f, 0.034f, 0.55f, 0.03f, 1.05f, 3000.0f, -0.02f, f, PadVoiceModel::Snare),
          fxHall() },
    };
}

static std::vector<PadPreset> makeClapPresets()
{
    const float f = kPadCharacteristics[3].baseFrequencyHz;
    return {
        // 1. Ultra-sec, clicking, machine-gun
        { "Clap Click",
          makePad(0.78f, -10.0f, 0.025f, 0.0001f, 0.0f, 0.003f, 0.92f, 0.28f, 1.10f, 9000.0f, 0.00f, f, PadVoiceModel::Clap),
          fxTransient() },
        // 2. 808-style, court, chaud, sub
        { "Clap 808",
          makePad(0.80f, 3.0f, 0.12f, 0.0003f, 4.0f, 0.018f, 0.30f, 0.01f, 1.20f, 1200.0f, 0.00f, f, PadVoiceModel::Clap),
          fxSub() },
        // 3. Hall massif, longue queue
        { "Clap Stadium",
          makePad(0.68f, -2.0f, 0.55f, 0.0008f, 0.5f, 0.042f, 0.45f, 0.01f, 1.00f, 1800.0f, 0.14f, f, PadVoiceModel::Clap),
          fxHall() },
        // 4. Crushé, saturation extrême
        { "Clap Crush",
          makePad(0.82f, -5.0f, 0.08f, 0.0002f, 0.0f, 0.007f, 0.85f, 0.06f, 3.50f, 3500.0f, 0.00f, f, PadVoiceModel::Clap),
          fxDist() },
        // 5. Doux, finger snap, minimal
        { "Clap Snap",
          makePad(0.50f, -14.0f, 0.04f, 0.0001f, 0.0f, 0.004f, 0.20f, 0.15f, 1.00f, 6500.0f, 0.00f, f, PadVoiceModel::Clap),
          fxClean() },
        // 6. Gated reverb, 80s
        { "Clap Gated",
          makePad(0.74f, -6.0f, 0.35f, 0.0004f, 0.0f, 0.025f, 0.70f, 0.04f, 1.10f, 2800.0f, 0.06f, f, PadVoiceModel::Clap),
          fxPunch() },
        // 7. Lo-fi, vinyl, saturé
        { "Clap Lo-Fi",
          makePad(0.66f, 1.0f, 0.18f, 0.0003f, 1.5f, 0.015f, 0.60f, 0.02f, 2.00f, 1600.0f, 0.02f, f, PadVoiceModel::Clap),
          fxWarm() },
        // 8. Trashy industriel
        { "Clap Trash",
          makePad(0.76f, -3.0f, 0.10f, 0.0002f, 0.0f, 0.008f, 0.88f, 0.05f, 2.80f, 4200.0f, 0.00f, f, PadVoiceModel::Clap),
          fxCrunch() },
        // 9. Distant, cave, noyé
        { "Clap Cave",
          makePad(0.58f, 0.0f, 0.60f, 0.001f, 0.3f, 0.048f, 0.35f, 0.01f, 1.00f, 1400.0f, 0.18f, f, PadVoiceModel::Clap),
          fxHall() },
        // 10. Électro, chorus, synthetic
        { "Clap Electro",
          makePad(0.72f, -8.0f, 0.15f, 0.0002f, 0.0f, 0.012f, 0.75f, 0.08f, 1.06f, 5500.0f, 0.04f, f, PadVoiceModel::Clap),
          fxElectro() },
        // 11. Layered, gros plan
        { "Clap Fat",
          makePad(0.84f, -4.0f, 0.25f, 0.0003f, 0.0f, 0.020f, 0.55f, 0.03f, 1.40f, 2200.0f, 0.06f, f, PadVoiceModel::Clap),
          fxPunch() },
        // 12. Ghost, très discret
        { "Clap Ghost",
          makePad(0.38f, -2.0f, 0.08f, 0.0004f, 0.0f, 0.008f, 0.40f, 0.01f, 1.00f, 3000.0f, 0.00f, f, PadVoiceModel::Clap),
          fxClean() },
        // 13. Metallic, résonant
        { "Clap Metal",
          makePad(0.70f, 6.0f, 0.06f, 0.0001f, 2.0f, 0.006f, 0.65f, 0.10f, 1.80f, 7000.0f, 0.02f, f, PadVoiceModel::Clap),
          fxCrunch() },
        // 14. Subby, grave profond
        { "Clap Sub",
          makePad(0.78f, 4.0f, 0.30f, 0.0005f, 8.0f, 0.028f, 0.25f, 0.01f, 1.04f, 900.0f, 0.00f, f, PadVoiceModel::Clap),
          fxSub() },
        // 15. Bright, perçant, commercial
        { "Clap Bright",
          makePad(0.74f, -12.0f, 0.06f, 0.0001f, 0.0f, 0.005f, 0.80f, 0.18f, 1.08f, 8500.0f, 0.00f, f, PadVoiceModel::Clap),
          fxTransient() },
        // 16. Dark, moche, underground
        { "Clap Dark",
          makePad(0.62f, 2.0f, 0.22f, 0.0004f, 1.0f, 0.020f, 0.48f, 0.02f, 1.02f, 1100.0f, 0.04f, f, PadVoiceModel::Clap),
          fxWarm() },
        // 17. Tight, short, drum machine
        { "Clap Drum Machine",
          makePad(0.70f, -7.0f, 0.035f, 0.0001f, 0.0f, 0.004f, 0.72f, 0.12f, 1.04f, 6200.0f, 0.00f, f, PadVoiceModel::Clap),
          fxClean() },
        // 18. Ring, long sustain, résonant
        { "Clap Ring",
          makePad(0.66f, -1.0f, 0.45f, 0.0006f, 1.5f, 0.035f, 0.50f, 0.01f, 1.00f, 2000.0f, 0.10f, f, PadVoiceModel::Clap),
          fxRoom() },
        // 19. Slap, très court, très clicky
        { "Clap Slap",
          makePad(0.82f, -15.0f, 0.02f, 0.0001f, 0.0f, 0.002f, 0.95f, 0.35f, 1.12f, 10000.0f, 0.00f, f, PadVoiceModel::Clap),
          fxTransient() },
        // 20. Resonant, tonal, pitché
        { "Clap Tonal",
          makePad(0.68f, 8.0f, 0.20f, 0.0003f, 5.0f, 0.018f, 0.42f, 0.03f, 1.06f, 3200.0f, 0.04f, f, PadVoiceModel::Clap),
          fxRoom() },
    };
}

static std::vector<PadPreset> makeHatClosedPresets()
{
    const float f = kPadCharacteristics[4].baseFrequencyHz;
    return {
        { "HH Tick",
          makePad(0.50f, 2.0f, 0.005f, 0.0f, 0.0f, 0.003f, 0.70f, 0.25f, 1.10f, 15000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxTransient() },
        { "HH Fat",
          makePad(0.52f, 0.0f, 0.030f, 0.0f, 0.0f, 0.008f, 0.35f, 0.08f, 1.02f, 5500.0f, -0.12f, f, PadVoiceModel::Hat),
          fxWarm() },
        { "HH Gritty",
          makePad(0.50f, -1.0f, 0.012f, 0.0f, 0.0f, 0.005f, 0.88f, 0.14f, 2.20f, 9000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxCrunch() },
        { "HH Soft",
          makePad(0.40f, 0.0f, 0.025f, 0.0f, 0.0f, 0.007f, 0.30f, 0.03f, 1.00f, 7000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Metal",
          makePad(0.54f, 3.0f, 0.010f, 0.0f, 0.0f, 0.004f, 0.92f, 0.16f, 1.80f, 13000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxCrunch() },
        { "HH Dark",
          makePad(0.44f, -3.0f, 0.035f, 0.0f, 0.0f, 0.009f, 0.40f, 0.06f, 1.00f, 4200.0f, -0.12f, f, PadVoiceModel::Hat),
          fxWarm() },
        { "HH Bright",
          makePad(0.54f, 3.0f, 0.008f, 0.0f, 0.0f, 0.004f, 0.72f, 0.22f, 1.08f, 14000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxPunch() },
        { "HH Dirty",
          makePad(0.50f, -1.0f, 0.020f, 0.0f, 0.0f, 0.006f, 0.85f, 0.12f, 2.50f, 8000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxDist() },
        { "HH Ghost",
          makePad(0.30f, 0.0f, 0.006f, 0.0f, 0.0f, 0.003f, 0.25f, 0.03f, 1.00f, 8200.0f, -0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Sizzle",
          makePad(0.48f, 0.0f, 0.040f, 0.0f, 0.0f, 0.009f, 0.80f, 0.10f, 1.10f, 9000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxRoom() },
        { "HH Pedal",
          makePad(0.42f, -4.0f, 0.045f, 0.0f, 0.0f, 0.010f, 0.35f, 0.08f, 1.01f, 5000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Noise",
          makePad(0.48f, -2.0f, 0.028f, 0.0f, 0.0f, 0.008f, 0.90f, 0.07f, 1.06f, 4500.0f, -0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Ring",
          makePad(0.50f, 2.0f, 0.050f, 0.0f, 0.0f, 0.010f, 0.55f, 0.09f, 1.04f, 8000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxRoom() },
        { "HH Tight 2",
          makePad(0.50f, 1.0f, 0.004f, 0.0f, 0.0f, 0.002f, 0.68f, 0.20f, 1.10f, 16000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxTransient() },
        { "HH Chunk",
          makePad(0.50f, -1.0f, 0.015f, 0.0f, 0.0f, 0.005f, 0.50f, 0.10f, 1.04f, 5200.0f, -0.12f, f, PadVoiceModel::Hat),
          fxPunch() },
        { "HH Plastic",
          makePad(0.44f, 0.0f, 0.022f, 0.0f, 0.0f, 0.006f, 0.32f, 0.06f, 1.00f, 7500.0f, -0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Splash",
          makePad(0.52f, 1.0f, 0.055f, 0.0f, 0.0f, 0.011f, 0.75f, 0.10f, 1.06f, 10000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxRoom() },
        { "HH Dry",
          makePad(0.48f, 0.0f, 0.008f, 0.0f, 0.0f, 0.004f, 0.50f, 0.08f, 1.00f, 9500.0f, -0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Acid",
          makePad(0.50f, -1.0f, 0.012f, 0.0f, 0.0f, 0.005f, 0.80f, 0.12f, 1.80f, 8500.0f, -0.12f, f, PadVoiceModel::Hat),
          fxElectro() },
        { "HH Sub",
          makePad(0.44f, -2.0f, 0.040f, 0.0f, 0.0f, 0.009f, 0.28f, 0.05f, 1.00f, 4000.0f, -0.12f, f, PadVoiceModel::Hat),
          fxSub() },
    };
}

static std::vector<PadPreset> makeHatOpenPresets()
{
    const float f = kPadCharacteristics[5].baseFrequencyHz;
    return {
        { "HH Open Short",
          makePad(0.44f, 0.0f, 0.035f, 0.0f, 0.0f, 0.007f, 0.55f, 0.09f, 1.00f, 9000.0f, 0.12f, f, PadVoiceModel::Hat),
          fxTransient() },
        { "HH Open Long",
          makePad(0.42f, -1.0f, 0.220f, 0.0f, 0.0f, 0.012f, 0.40f, 0.06f, 1.00f, 5500.0f, 0.12f, f, PadVoiceModel::Hat),
          fxHall() },
        { "HH Open Sizzle",
          makePad(0.46f, 0.0f, 0.120f, 0.0f, 0.0f, 0.010f, 0.85f, 0.08f, 1.04f, 10000.0f, 0.12f, f, PadVoiceModel::Hat),
          fxRoom() },
        { "HH Open Dark",
          makePad(0.40f, -2.0f, 0.180f, 0.0f, 0.0f, 0.011f, 0.35f, 0.06f, 1.00f, 4500.0f, 0.12f, f, PadVoiceModel::Hat),
          fxWarm() },
        { "HH Open Bright",
          makePad(0.48f, 1.0f, 0.060f, 0.0f, 0.0f, 0.008f, 0.66f, 0.15f, 1.04f, 12000.0f, 0.12f, f, PadVoiceModel::Hat),
          fxPunch() },
        { "HH Open Gritty",
          makePad(0.44f, 0.0f, 0.090f, 0.0f, 0.0f, 0.009f, 0.80f, 0.10f, 2.00f, 8000.0f, 0.12f, f, PadVoiceModel::Hat),
          fxCrunch() },
        { "HH Open Wash",
          makePad(0.46f, 0.0f, 0.250f, 0.0f, 0.0f, 0.013f, 0.60f, 0.07f, 1.01f, 7000.0f, 0.12f, f, PadVoiceModel::Hat),
          fxHall() },
        { "HH Open Dist",
          makePad(0.44f, 0.0f, 0.080f, 0.0f, 0.0f, 0.009f, 0.78f, 0.10f, 2.80f, 7500.0f, 0.12f, f, PadVoiceModel::Hat),
          fxDist() },
        { "HH Open Ghost",
          makePad(0.28f, 0.0f, 0.040f, 0.0f, 0.0f, 0.007f, 0.30f, 0.03f, 1.00f, 7200.0f, 0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Open Metal",
          makePad(0.46f, 2.0f, 0.050f, 0.0f, 0.0f, 0.008f, 0.90f, 0.12f, 1.60f, 13000.0f, 0.12f, f, PadVoiceModel::Hat),
          fxCrunch() },
        { "HH Open Loose",
          makePad(0.42f, -1.0f, 0.160f, 0.0f, 0.0f, 0.011f, 0.50f, 0.06f, 1.00f, 6500.0f, 0.12f, f, PadVoiceModel::Hat),
          fxRoom() },
        { "HH Open Soft",
          makePad(0.38f, -1.0f, 0.100f, 0.0f, 0.0f, 0.010f, 0.35f, 0.05f, 1.00f, 5000.0f, 0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Open Ring",
          makePad(0.42f, 0.0f, 0.200f, 0.0f, 0.0f, 0.012f, 0.55f, 0.07f, 1.00f, 5500.0f, 0.12f, f, PadVoiceModel::Hat),
          fxHall() },
        { "HH Open Fat",
          makePad(0.46f, -1.0f, 0.140f, 0.0f, 0.0f, 0.010f, 0.38f, 0.08f, 1.02f, 5200.0f, 0.12f, f, PadVoiceModel::Hat),
          fxWarm() },
        { "HH Open Thin",
          makePad(0.40f, 1.0f, 0.055f, 0.0f, 0.0f, 0.008f, 0.70f, 0.08f, 1.00f, 14000.0f, 0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Open Noise",
          makePad(0.44f, -2.0f, 0.100f, 0.0f, 0.0f, 0.010f, 0.88f, 0.06f, 1.02f, 4800.0f, 0.12f, f, PadVoiceModel::Hat),
          fxClean() },
        { "HH Open Chunk",
          makePad(0.44f, 0.0f, 0.075f, 0.0f, 0.0f, 0.009f, 0.50f, 0.09f, 1.02f, 7500.0f, 0.12f, f, PadVoiceModel::Hat),
          fxPunch() },
        { "HH Open Splash",
          makePad(0.48f, 1.0f, 0.240f, 0.0f, 0.0f, 0.013f, 0.72f, 0.08f, 1.00f, 9500.0f, 0.12f, f, PadVoiceModel::Hat),
          fxHall() },
        { "HH Open Dist 2",
          makePad(0.44f, 0.0f, 0.090f, 0.0f, 0.0f, 0.009f, 0.75f, 0.10f, 3.00f, 8000.0f, 0.12f, f, PadVoiceModel::Hat),
          fxDist() },
        { "HH Open Sub",
          makePad(0.42f, -2.0f, 0.180f, 0.0f, 0.0f, 0.011f, 0.30f, 0.05f, 1.00f, 4200.0f, 0.12f, f, PadVoiceModel::Hat),
          fxSub() },
    };
}

static std::vector<PadPreset> makePerc1Presets()
{
    const float f = kPadCharacteristics[6].baseFrequencyHz;
    return {
        { "Wood Tick",   makePad(0.92f,  2.0f, 0.020f, 0.0001f,  2.0f, 0.005f, 0.05f, 0.18f, 1.02f, 4500.0f, -0.18f, f, PadVoiceModel::PercWood), fxTransient() },
        { "Wood Low",    makePad(0.94f, -6.0f, 0.220f, 0.0002f,  4.0f, 0.035f, 0.08f, 0.05f, 1.00f,  900.0f, -0.18f, f, PadVoiceModel::PercWood), fxSub() },
        { "Wood High",   makePad(0.86f,  4.0f, 0.040f, 0.0001f, 12.0f, 0.010f, 0.06f, 0.14f, 1.04f, 4800.0f, -0.16f, f, PadVoiceModel::PercWood), fxPunch() },
        { "Tabla",       makePad(0.82f,  0.0f, 0.120f, 0.0002f, 14.0f, 0.022f, 0.07f, 0.06f, 1.01f, 1100.0f, -0.12f, f, PadVoiceModel::PercWood), fxRoom() },
        { "Conga",       makePad(0.86f,  3.0f, 0.160f, 0.0002f,  5.0f, 0.028f, 0.06f, 0.07f, 1.00f, 2200.0f, -0.10f, f, PadVoiceModel::PercWood), fxRoom() },
        { "Cajon",       makePad(0.90f, -4.0f, 0.100f, 0.0001f,  8.0f, 0.020f, 0.12f, 0.10f, 1.02f, 1800.0f, -0.14f, f, PadVoiceModel::PercWood), fxPunch() },
        { "Wood Dist",   makePad(0.88f, -2.0f, 0.060f, 0.0001f,  8.0f, 0.014f, 0.15f, 0.10f, 2.80f, 2000.0f, -0.18f, f, PadVoiceModel::PercWood), fxDist() },
        { "Wood Ghost",  makePad(0.45f, -1.0f, 0.040f, 0.0002f,  3.0f, 0.010f, 0.03f, 0.03f, 1.00f, 2200.0f, -0.14f, f, PadVoiceModel::PercWood), fxClean() },
        { "Wood Block",  makePad(0.94f,  6.0f, 0.025f, 0.0001f,  2.0f, 0.006f, 0.04f, 0.20f, 1.04f, 5000.0f, -0.14f, f, PadVoiceModel::PercWood), fxClean() },
        { "Wood Ring",   makePad(0.84f,  0.0f, 0.250f, 0.0002f, 10.0f, 0.040f, 0.10f, 0.05f, 1.01f, 1200.0f, -0.12f, f, PadVoiceModel::PercWood), fxHall() },
        { "Wood Slap",   makePad(0.88f,  2.0f, 0.035f, 0.0001f,  5.0f, 0.008f, 0.06f, 0.16f, 1.06f, 3800.0f, -0.16f, f, PadVoiceModel::PercWood), fxPunch() },
        { "Wood Fat",    makePad(0.92f, -3.0f, 0.180f, 0.0002f,  6.0f, 0.030f, 0.04f, 0.06f, 1.02f,  950.0f, -0.12f, f, PadVoiceModel::PercWood), fxWarm() },
        { "Wood Bright", makePad(0.86f,  3.0f, 0.030f, 0.0001f,  8.0f, 0.007f, 0.05f, 0.15f, 1.02f, 4800.0f, -0.16f, f, PadVoiceModel::PercWood), fxTransient() },
        { "Wood Knock",  makePad(0.88f, -3.0f, 0.080f, 0.0001f, 12.0f, 0.018f, 0.08f, 0.10f, 1.04f, 2400.0f, -0.14f, f, PadVoiceModel::PercWood), fxCrunch() },
        { "Wood Lo-Fi",  makePad(0.86f, -1.0f, 0.100f, 0.0001f,  7.0f, 0.020f, 0.12f, 0.08f, 2.20f, 1400.0f, -0.16f, f, PadVoiceModel::PercWood), fxWarm() },
        { "Wood Dry",    makePad(0.88f, -3.0f, 0.020f, 0.0001f,  2.0f, 0.005f, 0.04f, 0.14f, 1.00f, 4000.0f, -0.18f, f, PadVoiceModel::PercWood), fxClean() },
        { "Wood Deep",   makePad(0.94f, -7.0f, 0.200f, 0.0002f,  3.0f, 0.032f, 0.06f, 0.04f, 1.00f,  800.0f, -0.10f, f, PadVoiceModel::PercWood), fxSub() },
        { "Wood Soft",   makePad(0.78f, -1.0f, 0.080f, 0.0002f,  3.0f, 0.018f, 0.03f, 0.03f, 1.00f, 1800.0f, -0.14f, f, PadVoiceModel::PercWood), fxClean() },
        { "Wood Metal",  makePad(0.88f,  1.0f, 0.050f, 0.0001f,  6.0f, 0.012f, 0.08f, 0.10f, 1.80f, 4200.0f, -0.16f, f, PadVoiceModel::PercWood), fxCrunch() },
        { "Wood Pop",    makePad(0.86f,  0.0f, 0.030f, 0.0001f,  4.0f, 0.007f, 0.05f, 0.18f, 1.04f, 4600.0f, -0.16f, f, PadVoiceModel::PercWood), fxPunch() },
    };
}

static std::vector<PadPreset> makePerc2Presets()
{
    const float f = kPadCharacteristics[7].baseFrequencyHz;
    return {
        { "Metal Tick",    makePad(0.90f,  2.0f, 0.020f, 0.0001f,  2.0f, 0.005f, 0.05f, 0.18f, 1.02f, 6500.0f, 0.16f, f, PadVoiceModel::PercMetal), fxTransient() },
        { "Metal Low",     makePad(0.88f, -7.0f, 0.200f, 0.0002f,  6.0f, 0.032f, 0.08f, 0.04f, 1.00f, 1400.0f, 0.18f, f, PadVoiceModel::PercMetal), fxSub() },
        { "Cowbell",       makePad(0.84f,  5.0f, 0.150f, 0.0001f,  9.0f, 0.024f, 0.06f, 0.08f, 1.02f, 3000.0f, 0.16f, f, PadVoiceModel::PercMetal), fxRoom() },
        { "Rim Shot",      makePad(0.82f,  1.0f, 0.030f, 0.0001f,  4.0f, 0.008f, 0.10f, 0.22f, 1.03f, 7000.0f, 0.12f, f, PadVoiceModel::PercMetal), fxPunch() },
        { "Tambourine",    makePad(0.78f,  4.0f, 0.120f, 0.0001f,  3.0f, 0.015f, 0.22f, 0.04f, 1.00f, 5500.0f, 0.18f, f, PadVoiceModel::PercMetal), fxRoom() },
        { "Metal Dist",    makePad(0.84f, -2.0f, 0.060f, 0.0001f,  6.0f, 0.014f, 0.15f, 0.10f, 3.00f, 3500.0f, 0.14f, f, PadVoiceModel::PercMetal), fxDist() },
        { "Metal Ghost",   makePad(0.40f, -3.0f, 0.040f, 0.0002f,  5.0f, 0.010f, 0.04f, 0.03f, 1.00f, 3000.0f, 0.14f, f, PadVoiceModel::PercMetal), fxClean() },
        { "Bell Tone",     makePad(0.80f,  6.0f, 0.180f, 0.0002f,  7.0f, 0.028f, 0.05f, 0.06f, 1.00f, 2000.0f, 0.16f, f, PadVoiceModel::PercMetal), fxHall() },
        { "Steel Drum",    makePad(0.82f,  2.0f, 0.140f, 0.0002f, 10.0f, 0.022f, 0.06f, 0.07f, 1.01f, 3200.0f, 0.14f, f, PadVoiceModel::PercMetal), fxRoom() },
        { "Anvil",         makePad(0.86f,  7.0f, 0.060f, 0.0001f,  9.0f, 0.016f, 0.12f, 0.10f, 2.50f, 5000.0f, 0.14f, f, PadVoiceModel::PercMetal), fxCrunch() },
        { "Metal Bright",  makePad(0.82f,  3.0f, 0.040f, 0.0001f,  5.0f, 0.010f, 0.08f, 0.14f, 1.02f, 6800.0f, 0.12f, f, PadVoiceModel::PercMetal), fxTransient() },
        { "Metal Dark",    makePad(0.80f, -5.0f, 0.160f, 0.0002f,  8.0f, 0.028f, 0.09f, 0.04f, 1.00f, 1200.0f, 0.18f, f, PadVoiceModel::PercMetal), fxWarm() },
        { "Metal Ring",    makePad(0.80f,  0.0f, 0.220f, 0.0002f,  8.0f, 0.036f, 0.06f, 0.05f, 1.01f, 2200.0f, 0.16f, f, PadVoiceModel::PercMetal), fxHall() },
        { "Metal Dry",     makePad(0.82f,  1.0f, 0.020f, 0.0001f,  3.0f, 0.005f, 0.04f, 0.15f, 1.02f, 5500.0f, 0.12f, f, PadVoiceModel::PercMetal), fxClean() },
        { "Metal Fat",     makePad(0.84f, -4.0f, 0.180f, 0.0002f,  6.0f, 0.030f, 0.08f, 0.04f, 1.02f, 1500.0f, 0.16f, f, PadVoiceModel::PercMetal), fxWarm() },
        { "Metal Click",   makePad(0.82f,  3.0f, 0.025f, 0.0001f,  5.0f, 0.006f, 0.05f, 0.20f, 1.04f, 5200.0f, 0.14f, f, PadVoiceModel::PercMetal), fxPunch() },
        { "Metal Noise",   makePad(0.80f, -2.0f, 0.100f, 0.0001f,  6.0f, 0.020f, 0.25f, 0.06f, 1.02f, 4000.0f, 0.18f, f, PadVoiceModel::PercMetal), fxClean() },
        { "Metal Lo-Fi",   makePad(0.80f, -1.0f, 0.080f, 0.0001f,  6.0f, 0.018f, 0.12f, 0.08f, 2.20f, 2000.0f, 0.14f, f, PadVoiceModel::PercMetal), fxWarm() },
        { "Metal Soft",    makePad(0.76f, -5.0f, 0.100f, 0.0002f,  4.0f, 0.022f, 0.05f, 0.04f, 1.00f, 2600.0f, 0.16f, f, PadVoiceModel::PercMetal), fxClean() },
        { "Metal Pop",     makePad(0.84f,  3.0f, 0.040f, 0.0001f,  5.0f, 0.010f, 0.08f, 0.16f, 1.04f, 6000.0f, 0.12f, f, PadVoiceModel::PercMetal), fxPunch() },
    };
}

static std::vector<PadPreset> makeTomLowPresets()
{
    const float f = kPadCharacteristics[8].baseFrequencyHz;
    return {
        { "Tom Low Deep",    makePad(0.66f, -5.0f, 0.300f, 0.0006f,  3.0f, 0.050f, 0.003f, 0.05f, 1.00f, 1200.0f, -0.10f, f, PadVoiceModel::Tom), fxSub() },
        { "Tom Low Punch",   makePad(0.62f,  1.0f, 0.060f, 0.0002f, 10.0f, 0.014f, 0.005f, 0.14f, 1.08f, 4200.0f, -0.06f, f, PadVoiceModel::Tom), fxPunch() },
        { "Tom Low Tight",   makePad(0.58f,  2.0f, 0.040f, 0.0002f,  4.0f, 0.010f, 0.004f, 0.12f, 1.04f, 4800.0f, -0.06f, f, PadVoiceModel::Tom), fxTransient() },
        { "Tom Low Loose",   makePad(0.56f, -1.0f, 0.220f, 0.0005f,  5.0f, 0.040f, 0.006f, 0.06f, 1.00f, 2000.0f, -0.08f, f, PadVoiceModel::Tom), fxRoom() },
        { "Tom Low Dist",    makePad(0.62f,  0.0f, 0.100f, 0.0003f,  8.0f, 0.022f, 0.012f, 0.10f, 2.80f, 3000.0f, -0.08f, f, PadVoiceModel::Tom), fxDist() },
        { "Tom Low Soft",    makePad(0.50f, -2.0f, 0.140f, 0.0005f,  2.0f, 0.028f, 0.002f, 0.04f, 1.00f, 2200.0f, -0.10f, f, PadVoiceModel::Tom), fxClean() },
        { "Tom Low Boom",    makePad(0.66f, -4.0f, 0.350f, 0.0006f,  6.0f, 0.055f, 0.004f, 0.06f, 1.00f, 1400.0f, -0.12f, f, PadVoiceModel::Tom), fxHall() },
        { "Tom Low Ring",    makePad(0.60f,  0.0f, 0.250f, 0.0005f,  7.0f, 0.045f, 0.006f, 0.07f, 1.02f, 1800.0f, -0.08f, f, PadVoiceModel::Tom), fxRoom() },
        { "Tom Low Dark",    makePad(0.62f, -3.0f, 0.180f, 0.0005f, 12.0f, 0.035f, 0.005f, 0.06f, 1.00f, 1600.0f, -0.10f, f, PadVoiceModel::Tom), fxWarm() },
        { "Tom Low Bright",  makePad(0.58f,  2.0f, 0.070f, 0.0003f,  3.0f, 0.016f, 0.007f, 0.12f, 1.04f, 5000.0f, -0.06f, f, PadVoiceModel::Tom), fxTransient() },
        { "Tom Low Click",   makePad(0.56f,  3.0f, 0.045f, 0.0002f,  2.0f, 0.010f, 0.003f, 0.15f, 1.06f, 4600.0f, -0.06f, f, PadVoiceModel::Tom), fxTransient() },
        { "Tom Low Fat",     makePad(0.64f, -2.0f, 0.200f, 0.0005f,  5.0f, 0.038f, 0.008f, 0.07f, 1.02f, 1800.0f, -0.08f, f, PadVoiceModel::Tom), fxPunch() },
        { "Tom Low Ghost",   makePad(0.42f,  0.0f, 0.080f, 0.0004f,  2.0f, 0.018f, 0.002f, 0.03f, 1.00f, 2600.0f, -0.06f, f, PadVoiceModel::Tom), fxClean() },
        { "Tom Low Analog",  makePad(0.60f, -1.0f, 0.160f, 0.0004f,  6.0f, 0.032f, 0.005f, 0.08f, 1.06f, 2400.0f, -0.08f, f, PadVoiceModel::Tom), fxWarm() },
        { "Tom Low Dry",     makePad(0.58f,  0.0f, 0.050f, 0.0002f,  4.0f, 0.012f, 0.003f, 0.10f, 1.02f, 3800.0f, -0.06f, f, PadVoiceModel::Tom), fxClean() },
        { "Tom Low Resonant",makePad(0.62f, -1.0f, 0.280f, 0.0006f,  8.0f, 0.048f, 0.006f, 0.07f, 1.02f, 1600.0f, -0.10f, f, PadVoiceModel::Tom), fxRoom() },
        { "Tom Low Acoustic",makePad(0.60f, -1.0f, 0.120f, 0.0004f,  5.0f, 0.026f, 0.007f, 0.08f, 1.02f, 2800.0f, -0.08f, f, PadVoiceModel::Tom), fxRoom() },
        { "Tom Low Sub",     makePad(0.68f, -6.0f, 0.340f, 0.0007f,  4.0f, 0.052f, 0.003f, 0.04f, 1.00f, 1200.0f, -0.12f, f, PadVoiceModel::Tom), fxSub() },
        { "Tom Low Drop",    makePad(0.64f,  0.0f, 0.150f, 0.0003f, 12.0f, 0.030f, 0.008f, 0.09f, 1.06f, 2600.0f, -0.08f, f, PadVoiceModel::Tom), fxCrunch() },
        { "Tom Low Massive", makePad(0.66f, -3.0f, 0.320f, 0.0007f,  8.0f, 0.055f, 0.015f, 0.06f, 1.04f, 1600.0f, -0.12f, f, PadVoiceModel::Tom), fxHall() },
    };
}

static std::vector<PadPreset> makeTomHighPresets()
{
    const float f = kPadCharacteristics[9].baseFrequencyHz;
    return {
        { "Tom High Crisp",   makePad(0.58f,  2.0f, 0.050f, 0.0003f,  3.0f, 0.012f, 0.005f, 0.12f, 1.04f, 5200.0f, 0.08f, f, PadVoiceModel::Tom), fxTransient() },
        { "Tom High Punch",   makePad(0.58f,  1.0f, 0.060f, 0.0003f,  8.0f, 0.015f, 0.004f, 0.14f, 1.08f, 5600.0f, 0.06f, f, PadVoiceModel::Tom), fxPunch() },
        { "Tom High Tight",   makePad(0.54f,  3.0f, 0.040f, 0.0002f,  2.0f, 0.009f, 0.004f, 0.14f, 1.04f, 5800.0f, 0.06f, f, PadVoiceModel::Tom), fxTransient() },
        { "Tom High Loose",   makePad(0.54f, -1.0f, 0.180f, 0.0005f,  4.0f, 0.035f, 0.006f, 0.06f, 1.00f, 3200.0f, 0.08f, f, PadVoiceModel::Tom), fxRoom() },
        { "Tom High Dist",    makePad(0.56f,  0.0f, 0.080f, 0.0003f,  7.0f, 0.018f, 0.010f, 0.10f, 2.50f, 4000.0f, 0.08f, f, PadVoiceModel::Tom), fxDist() },
        { "Tom High Soft",    makePad(0.48f, -2.0f, 0.120f, 0.0005f,  2.0f, 0.025f, 0.003f, 0.04f, 1.00f, 3000.0f, 0.08f, f, PadVoiceModel::Tom), fxClean() },
        { "Tom High Ring",    makePad(0.56f,  0.0f, 0.200f, 0.0005f,  6.0f, 0.038f, 0.006f, 0.07f, 1.01f, 3400.0f, 0.08f, f, PadVoiceModel::Tom), fxHall() },
        { "Tom High Dark",    makePad(0.56f, -3.0f, 0.140f, 0.0004f, 10.0f, 0.030f, 0.005f, 0.05f, 1.00f, 2600.0f, 0.10f, f, PadVoiceModel::Tom), fxWarm() },
        { "Tom High Bright",  makePad(0.56f,  3.0f, 0.050f, 0.0002f,  3.0f, 0.012f, 0.006f, 0.12f, 1.04f, 6000.0f, 0.06f, f, PadVoiceModel::Tom), fxTransient() },
        { "Tom High Click",   makePad(0.54f,  4.0f, 0.040f, 0.0002f,  2.0f, 0.009f, 0.004f, 0.15f, 1.06f, 5800.0f, 0.06f, f, PadVoiceModel::Tom), fxTransient() },
        { "Tom High Fat",     makePad(0.58f,  1.0f, 0.150f, 0.0004f,  4.0f, 0.030f, 0.007f, 0.08f, 1.02f, 3600.0f, 0.08f, f, PadVoiceModel::Tom), fxPunch() },
        { "Tom High Ghost",   makePad(0.40f,  0.0f, 0.060f, 0.0004f,  2.0f, 0.014f, 0.003f, 0.03f, 1.00f, 4200.0f, 0.06f, f, PadVoiceModel::Tom), fxClean() },
        { "Tom High Analog",  makePad(0.56f,  0.0f, 0.100f, 0.0004f,  5.0f, 0.022f, 0.005f, 0.08f, 1.06f, 4000.0f, 0.08f, f, PadVoiceModel::Tom), fxWarm() },
        { "Tom High Dry",     makePad(0.54f,  1.0f, 0.050f, 0.0002f,  3.0f, 0.012f, 0.004f, 0.10f, 1.02f, 5000.0f, 0.06f, f, PadVoiceModel::Tom), fxClean() },
        { "Tom High Pop",     makePad(0.56f,  2.0f, 0.070f, 0.0003f,  4.0f, 0.016f, 0.005f, 0.12f, 1.04f, 4800.0f, 0.08f, f, PadVoiceModel::Tom), fxPunch() },
        { "Tom High Resonant",makePad(0.56f,  1.0f, 0.180f, 0.0005f,  7.0f, 0.035f, 0.006f, 0.07f, 1.02f, 3200.0f, 0.08f, f, PadVoiceModel::Tom), fxRoom() },
        { "Tom High Big",     makePad(0.60f,  0.0f, 0.250f, 0.0006f,  5.0f, 0.045f, 0.008f, 0.06f, 1.00f, 2800.0f, 0.10f, f, PadVoiceModel::Tom), fxHall() },
        { "Tom High Acoustic",makePad(0.56f,  0.0f, 0.090f, 0.0004f,  4.0f, 0.020f, 0.006f, 0.08f, 1.02f, 4200.0f, 0.08f, f, PadVoiceModel::Tom), fxRoom() },
        { "Tom High Thin",    makePad(0.50f,  3.0f, 0.040f, 0.0002f,  1.5f, 0.009f, 0.005f, 0.12f, 1.02f, 5600.0f, 0.06f, f, PadVoiceModel::Tom), fxTransient() },
        { "Tom High Snap",    makePad(0.58f,  2.0f, 0.065f, 0.0003f,  6.0f, 0.015f, 0.004f, 0.13f, 1.06f, 5400.0f, 0.06f, f, PadVoiceModel::Tom), fxPunch() },
    };
}

static std::vector<PadPreset> makeCrashPresets()
{
    const float f = kPadCharacteristics[10].baseFrequencyHz;
    return {
        // 1. Ultra-court, splash machine-gun
        { "Crash Tick",
          makePad(0.50f, 4.0f, 0.025f, 0.0001f, 0.0f, 0.003f, 0.88f, 0.22f, 1.08f, 13000.0f, 0.10f, f, PadVoiceModel::Crash),
          fxTransient() },
        // 2. Wash massif, longue queue
        { "Crash Wash",
          makePad(0.42f, -1.0f, 0.70f, 0.001f, 0.3f, 0.022f, 0.40f, 0.02f, 1.00f, 5000.0f, 0.24f, f, PadVoiceModel::Crash),
          fxHall() },
        // 3. Trashy, distortion lourde
        { "Crash Trash",
          makePad(0.52f, -3.0f, 0.15f, 0.0002f, 0.0f, 0.010f, 0.82f, 0.10f, 3.20f, 7500.0f, 0.14f, f, PadVoiceModel::Crash),
          fxDist() },
        // 4. Ghost, très discret
        { "Crash Ghost",
          makePad(0.25f, 0.0f, 0.20f, 0.0005f, 0.0f, 0.012f, 0.28f, 0.01f, 1.00f, 6000.0f, 0.08f, f, PadVoiceModel::Crash),
          fxClean() },
        // 5. Bright commercial, perçant
        { "Crash Bright",
          makePad(0.54f, 3.0f, 0.18f, 0.0001f, 0.0f, 0.008f, 0.75f, 0.18f, 1.06f, 14000.0f, 0.16f, f, PadVoiceModel::Crash),
          fxTransient() },
        // 6. Dark underground, sourd
        { "Crash Dark",
          makePad(0.38f, -4.0f, 0.45f, 0.0008f, 1.0f, 0.018f, 0.32f, 0.02f, 1.00f, 3500.0f, 0.22f, f, PadVoiceModel::Crash),
          fxWarm() },
        // 7. Metallic ring, résonant
        { "Crash Ring",
          makePad(0.46f, 2.0f, 0.55f, 0.0006f, 0.5f, 0.020f, 0.35f, 0.03f, 1.02f, 4200.0f, 0.18f, f, PadVoiceModel::Crash),
          fxRoom() },
        // 8. Splash tight, percussif
        { "Crash Splash",
          makePad(0.48f, 5.0f, 0.04f, 0.0001f, 0.0f, 0.004f, 0.85f, 0.15f, 1.04f, 12000.0f, 0.12f, f, PadVoiceModel::Crash),
          fxPunch() },
        // 9. Ride bell, tonal
        { "Ride Bell",
          makePad(0.52f, -6.0f, 0.30f, 0.0003f, 3.0f, 0.015f, 0.20f, 0.08f, 1.00f, 3800.0f, 0.10f, f, PadVoiceModel::Crash),
          fxRoom() },
        // 10. China, agressif
        { "Crash China",
          makePad(0.50f, 1.0f, 0.22f, 0.0002f, 0.0f, 0.011f, 0.78f, 0.12f, 1.60f, 9500.0f, 0.14f, f, PadVoiceModel::Crash),
          fxCrunch() },
        // 11. Gong, très long, grave
        { "Crash Gong",
          makePad(0.44f, -8.0f, 0.80f, 0.002f, 2.0f, 0.028f, 0.30f, 0.01f, 1.00f, 2800.0f, 0.20f, f, PadVoiceModel::Crash),
          fxHall() },
        // 12. Gated, sec
        { "Crash Gated",
          makePad(0.46f, 0.0f, 0.25f, 0.0003f, 0.0f, 0.012f, 0.60f, 0.06f, 1.08f, 7000.0f, 0.16f, f, PadVoiceModel::Crash),
          fxPunch() },
        // 13. Sub crash, grave profond
        { "Crash Sub",
          makePad(0.48f, -5.0f, 0.60f, 0.001f, 4.0f, 0.024f, 0.25f, 0.01f, 1.00f, 2200.0f, 0.18f, f, PadVoiceModel::Crash),
          fxSub() },
        // 14. Click crash, très clicky
        { "Crash Click",
          makePad(0.52f, 6.0f, 0.06f, 0.0001f, 0.0f, 0.005f, 0.70f, 0.25f, 1.06f, 11000.0f, 0.10f, f, PadVoiceModel::Crash),
          fxTransient() },
        // 15. Soft, delicate
        { "Crash Soft",
          makePad(0.30f, 1.0f, 0.35f, 0.0006f, 0.5f, 0.016f, 0.38f, 0.02f, 1.00f, 5500.0f, 0.20f, f, PadVoiceModel::Crash),
          fxClean() },
        // 16. Noise crash, très bruyant
        { "Crash Noise",
          makePad(0.44f, -2.0f, 0.28f, 0.0003f, 0.0f, 0.013f, 0.92f, 0.05f, 1.02f, 8000.0f, 0.16f, f, PadVoiceModel::Crash),
          fxClean() },
        // 17. Distorted industrial
        { "Crash Industrial",
          makePad(0.50f, -1.0f, 0.12f, 0.0002f, 0.0f, 0.008f, 0.70f, 0.08f, 2.80f, 6500.0f, 0.12f, f, PadVoiceModel::Crash),
          fxDist() },
        // 18. Ride ping, court, tonal
        { "Ride Ping",
          makePad(0.50f, -4.0f, 0.18f, 0.0002f, 2.0f, 0.010f, 0.22f, 0.10f, 1.00f, 4500.0f, 0.08f, f, PadVoiceModel::Crash),
          fxRoom() },
        // 19. Stack, multiple couches
        { "Crash Stack",
          makePad(0.40f, 0.0f, 0.10f, 0.0002f, 0.0f, 0.007f, 0.80f, 0.12f, 1.04f, 10500.0f, 0.14f, f, PadVoiceModel::Crash),
          fxTransient() },
        // 20. Electro, chorus
        { "Crash Electro",
          makePad(0.46f, 2.0f, 0.20f, 0.0003f, 0.0f, 0.011f, 0.58f, 0.06f, 1.04f, 8500.0f, 0.16f, f, PadVoiceModel::Crash),
          fxElectro() },
    };
}

static std::vector<PadPreset> makeFxPresets()
{
    const float f = kPadCharacteristics[11].baseFrequencyHz;
    return {
        { "FX Zap",        makePad(0.52f, 16.0f, 0.025f, 0.0001f, 24.0f, 0.010f, 0.02f, 0.16f, 1.06f, 6800.0f, -0.14f, f, PadVoiceModel::Fx), fxTransient() },
        { "FX Sweep",      makePad(0.56f,  6.0f, 0.200f, 0.0003f, 18.0f, 0.038f, 0.04f, 0.08f, 1.02f, 3200.0f, -0.14f, f, PadVoiceModel::Fx), fxHall() },
        { "FX Impact",     makePad(0.62f,  0.0f, 0.080f, 0.0002f, 10.0f, 0.018f, 0.06f, 0.14f, 1.08f, 2800.0f, -0.14f, f, PadVoiceModel::Fx), fxPunch() },
        { "FX Drone",      makePad(0.44f,  2.0f, 0.380f, 0.0004f,  4.0f, 0.045f, 0.08f, 0.06f, 1.00f, 1800.0f, -0.14f, f, PadVoiceModel::Fx), fxHall() },
        { "FX Glitch",     makePad(0.54f,  8.0f, 0.030f, 0.0001f, 20.0f, 0.010f, 0.10f, 0.18f, 2.40f, 7000.0f, -0.10f, f, PadVoiceModel::Fx), fxDist() },
        { "FX Laser",      makePad(0.52f, 14.0f, 0.040f, 0.0001f, 22.0f, 0.012f, 0.02f, 0.15f, 1.06f, 7500.0f, -0.14f, f, PadVoiceModel::Fx), fxTransient() },
        { "FX Boom",       makePad(0.58f, -8.0f, 0.150f, 0.0003f,  8.0f, 0.030f, 0.03f, 0.08f, 1.02f, 1200.0f, -0.14f, f, PadVoiceModel::Fx), fxSub() },
        { "FX Noise Burst",makePad(0.50f,  4.0f, 0.060f, 0.0002f,  6.0f, 0.014f, 0.12f, 0.10f, 1.04f, 5200.0f, -0.14f, f, PadVoiceModel::Fx), fxCrunch() },
        { "FX Sub Blip",   makePad(0.58f, -6.0f, 0.040f, 0.0001f, 16.0f, 0.012f, 0.01f, 0.08f, 1.00f, 1000.0f, -0.14f, f, PadVoiceModel::Fx), fxSub() },
        { "FX Click",      makePad(0.52f, 10.0f, 0.020f, 0.0001f, 12.0f, 0.008f, 0.02f, 0.20f, 1.08f, 8000.0f, -0.14f, f, PadVoiceModel::Fx), fxTransient() },
        { "FX Wobble",     makePad(0.50f,  5.0f, 0.250f, 0.0003f, 10.0f, 0.042f, 0.06f, 0.08f, 1.02f, 2400.0f, -0.14f, f, PadVoiceModel::Fx), fxElectro() },
        { "FX Dist Hit",   makePad(0.54f,  1.0f, 0.050f, 0.0002f, 14.0f, 0.014f, 0.08f, 0.12f, 3.00f, 4000.0f, -0.14f, f, PadVoiceModel::Fx), fxDist() },
        { "FX Chirp",      makePad(0.48f, 12.0f, 0.050f, 0.0002f, 20.0f, 0.014f, 0.03f, 0.14f, 1.04f, 6000.0f, -0.14f, f, PadVoiceModel::Fx), fxTransient() },
        { "FX Shimmer",    makePad(0.46f,  8.0f, 0.300f, 0.0004f,  6.0f, 0.045f, 0.08f, 0.06f, 1.02f, 3500.0f,  0.10f, f, PadVoiceModel::Fx), fxHall() },
        { "FX Soft Hit",   makePad(0.42f,  3.0f, 0.100f, 0.0003f,  5.0f, 0.022f, 0.03f, 0.05f, 1.00f, 2600.0f, -0.14f, f, PadVoiceModel::Fx), fxClean() },
        { "FX Resonant",   makePad(0.50f,  5.0f, 0.180f, 0.0003f,  8.0f, 0.035f, 0.05f, 0.08f, 1.02f, 3000.0f, -0.14f, f, PadVoiceModel::Fx), fxRoom() },
        { "FX Metallic",   makePad(0.48f, 10.0f, 0.080f, 0.0002f,  6.0f, 0.020f, 0.07f, 0.12f, 1.06f, 5800.0f,  0.10f, f, PadVoiceModel::Fx), fxCrunch() },
        { "FX Ghost",      makePad(0.36f,  5.0f, 0.120f, 0.0003f,  6.0f, 0.025f, 0.02f, 0.04f, 1.00f, 2200.0f, -0.14f, f, PadVoiceModel::Fx), fxClean() },
        { "FX Short Zap",  makePad(0.50f, 12.0f, 0.020f, 0.0001f, 18.0f, 0.008f, 0.02f, 0.18f, 1.06f, 7200.0f, -0.14f, f, PadVoiceModel::Fx), fxTransient() },
        { "FX Rumble",     makePad(0.56f, -4.0f, 0.400f, 0.0004f,  2.0f, 0.048f, 0.06f, 0.06f, 1.04f, 1400.0f, -0.14f, f, PadVoiceModel::Fx), fxSub() },
    };
}

#include "Curated8FactoryPadPresets.inc"

// =========================================================================
// Registry
// =========================================================================
static void applyKitMetadata(KitPreset& preset)
{
    auto set = [&](const char* family,
                   const char* mixRole,
                   const char* description,
                   const char* outputProfile,
                   const float nominalPeakDb,
                   std::initializer_list<const char*> tags)
    {
        preset.familyLabel = family;
        preset.mixRole = mixRole;
        preset.description = description;
        preset.outputProfile = outputProfile;
        preset.nominalPeakDb = nominalPeakDb;
        preset.tags.clear();
        for (const auto* tag : tags)
            preset.tags.emplace_back(tag);
    };

    preset.fx.limiterEnable = true;
    preset.fx.limiterThreshold = std::min(preset.fx.limiterThreshold, -1.0f);

    if (preset.name == "Classique Standard")
    {
        set("Classique", "foundation", "Kit serre et polyvalent pour grooves generaux.", "master-ready", -2.3f, { "dry", "balanced", "punchy" });
        preset.fx.outputGainDb -= 0.80f;
    }
    else if (preset.name == "Classique Tight")
    {
        set("Classique", "tight", "Version plus courte et plus stricte pour arrangements denses.", "master-ready", -2.6f, { "tight", "controlled", "dry" });
        preset.fx.outputGainDb -= 0.40f;
    }
    else if (preset.name == "Classique Open")
    {
        set("Classique", "open", "Version plus ample avec davantage d'air et de queue.", "master-ready", -3.1f, { "open", "roomy", "wide" });
        preset.fx.outputGainDb -= 0.90f;
    }
    else if (preset.name == "Acoustique Room")
        set("Acoustique", "natural", "Kit acoustique avec ambience de piece moderee.", "master-ready", -2.7f, { "natural", "room", "organic" });
    else if (preset.name == "Acoustique Studio")
        set("Acoustique", "mix-ready", "Kit acoustique plus compresse et recentre pour le mix.", "master-ready", -1.9f, { "studio", "focused", "mix-ready" });
    else if (preset.name == "Acoustique Brush")
        set("Acoustique", "soft", "Kit doux a balais et densite reduite.", "master-ready", -2.1f, { "soft", "brush", "organic" });
    else if (preset.name == "Acoustique Jazz")
    {
        set("Acoustique", "airy", "Kit leger et ouvert pour jeu jazz ou fusion.", "master-ready", -3.1f, { "jazz", "airy", "light" });
        preset.fx.outputGainDb -= 0.90f;
    }
    else if (preset.name == "Ambient Pad")
        set("Ambient", "wash", "Kit ambient large avec longues queues et espace stereo.", "master-ready", -3.7f, { "ambient", "wide", "wash" });
    else if (preset.name == "Ambient Dark")
    {
        set("Ambient", "dark", "Kit sombre et plus retenu pour textures lentes.", "master-ready", -4.4f, { "ambient", "dark", "textured" });
        preset.fx.outputGainDb -= 0.90f;
    }
    else if (preset.name == "Ambient Sparse")
        set("Ambient", "sparse", "Kit eclairci et moins dense pour arrangements minimalistes.", "master-ready", -4.0f, { "ambient", "sparse", "minimal" });
    else if (preset.name == "Cinematique Epic")
    {
        set("Cinematique", "cinematic", "Kit large et impactant pour trailers et percussion hybride.", "master-ready", -3.0f, { "cinematic", "epic", "wide" });
        preset.fx.outputGainDb -= 0.85f;
    }
    else if (preset.name == "Cinematique Tension")
    {
        set("Cinematique", "aggressive", "Version plus tendue et plus compressee pour pics de tension.", "master-ready", -2.7f, { "cinematic", "aggressive", "tense" });
        preset.fx.outputGainDb -= 0.30f;
    }
    else if (preset.name == "Cinematique Hybrid")
        set("Cinematique", "hybrid", "Fusion acoustique-electronique avec delay et attaque renforcee.", "master-ready", -1.8f, { "cinematic", "hybrid", "designed" });
    else if (preset.name == "Cinematique Percussion")
    {
        set("Cinematique", "percussion-forward", "Version centree percussions et toms pour beds rythmiques.", "master-ready", -2.2f, { "cinematic", "percussion", "ensemble" });
        preset.fx.outputGainDb -= 0.25f;
    }
    else if (preset.name == "Moderne Club")
        set("Moderne", "club", "Kit moderne dense et direct pour grooves dance et pop.", "master-ready", -1.8f, { "modern", "club", "punchy" });
    else if (preset.name == "Moderne Lo-Fi")
        set("Moderne", "lofi", "Kit degrade et assombri avec saturation et coupe haute.", "master-ready", -3.0f, { "modern", "lofi", "textured" });
    else if (preset.name == "Moderne Trap")
        set("Moderne", "trap", "Kit sub-heavy et sec pour patterns trap et hip-hop modernes.", "master-ready", -2.1f, { "modern", "trap", "sub" });
    else if (preset.name == "Moderne Electro")
        set("Moderne", "electro", "Kit synthetique brillant avec chorus et delay synchronise.", "master-ready", -1.8f, { "modern", "electro", "synthetic" });
    else
        set("User", "custom", "Kit sans categorisation editee.", "master-ready", -6.0f, { "custom" });
}

static const std::vector<KitPreset>& buildFactoryPresets()
{
    static const std::vector<KitPreset> presets = []()
    {
        std::vector<KitPreset> items = {
            makeKit_Classique_Standard(),
            makeKit_Classique_Tight(),
            makeKit_Classique_Open(),
            makeKit_Acoustique_Room(),
            makeKit_Acoustique_Studio(),
            makeKit_Acoustique_Brush(),
            makeKit_Acoustique_Jazz(),
            makeKit_Ambient_Pad(),
            makeKit_Ambient_Dark(),
            makeKit_Ambient_Sparse(),
            makeKit_Cinematique_Epic(),
            makeKit_Cinematique_Tension(),
            makeKit_Cinematique_Hybrid(),
            makeKit_Cinematique_Percussion(),
            makeKit_Moderne_Club(),
            makeKit_Moderne_LoFi(),
            makeKit_Moderne_Trap(),
            makeKit_Moderne_Electro(),
        };

        for (auto& preset : items)
        {
            applyKitMetadata(preset);
            stampKitIdentities(preset);
        }

        return items;
    }();
    return presets;
}

} // namespace (anonymous)

// =========================================================================
// Public API
// =========================================================================

PadSettings getDefaultPadSettings(const int padIndex)
{
    const auto& freq = kPadCharacteristics;
    switch (std::clamp(padIndex, 0, kNumPads - 1))
    {
        case 0:  return withPadIdentity(makePad(0.90f, 1.5f,  0.30f,  0.0004f,  5.0f, 0.032f, 0.003f, 0.08f, 1.02f, 3000.0f,  0.0f,  freq[0].baseFrequencyHz, PadVoiceModel::Kick), 0);
        case 1:  return withPadIdentity(makePad(0.68f, 7.0f,  0.13f,  0.0004f,  2.0f, 0.022f, 0.002f, 0.20f, 1.02f, 2200.0f,  0.0f,  freq[1].baseFrequencyHz, PadVoiceModel::Kick), 1);
        case 2:  return withPadIdentity(makePad(0.74f, 9.0f,  0.09f,  0.0002f,  0.3f, 0.010f, 0.68f,  0.04f, 1.01f, 6200.0f, -0.02f, freq[2].baseFrequencyHz, PadVoiceModel::Snare), 2);
        case 3:  return withPadIdentity(makePad(0.64f,-6.0f,  0.195f, 0.0002f,  0.0f, 0.014f, 0.66f,  0.01f, 1.00f, 3200.0f,  0.04f, freq[3].baseFrequencyHz, PadVoiceModel::Clap), 3);
        case 4:  return withPadIdentity(makePad(0.50f, 0.0f,  0.020f, 0.0f,     0.0f, 0.007f, 0.64f,  0.12f, 1.00f, 8800.0f, -0.12f, freq[4].baseFrequencyHz, PadVoiceModel::Hat), 4);
        case 5:  return withPadIdentity(makePad(0.44f, 0.0f,  0.080f, 0.0f,     0.0f, 0.009f, 0.60f,  0.08f, 1.00f, 7200.0f,  0.12f, freq[5].baseFrequencyHz, PadVoiceModel::Hat), 5);
        case 6:  return withPadIdentity(makePad(0.92f,-5.0f,  0.082f, 0.0001f,  6.0f, 0.018f, 0.10f,  0.07f, 1.00f, 1600.0f, -0.18f, freq[6].baseFrequencyHz, PadVoiceModel::PercWood), 6);
        case 7:  return withPadIdentity(makePad(0.86f,-7.0f,  0.088f, 0.0001f,  8.0f, 0.018f, 0.10f,  0.06f, 1.00f, 2400.0f,  0.16f, freq[7].baseFrequencyHz, PadVoiceModel::PercMetal), 7);
        case 8:  return withPadIdentity(makePad(0.58f, 0.0f,  0.12f,  0.0004f,  3.0f, 0.028f, 0.004f, 0.08f, 1.01f, 3200.0f, -0.08f, freq[8].baseFrequencyHz, PadVoiceModel::Tom), 8);
        case 9:  return withPadIdentity(makePad(0.54f, 0.0f,  0.10f,  0.0004f,  2.5f, 0.024f, 0.004f, 0.07f, 1.01f, 4000.0f,  0.08f, freq[9].baseFrequencyHz, PadVoiceModel::Tom), 9);
        case 10: return withPadIdentity(makePad(0.24f, 0.0f,  0.24f,  0.0f,     0.0f, 0.012f, 0.56f,  0.06f, 1.00f, 7800.0f,  0.20f, freq[10].baseFrequencyHz, PadVoiceModel::Crash), 10);
        case 11: return withPadIdentity(makePad(0.28f, 7.0f,  0.08f,  0.0002f,  8.0f, 0.020f, 0.02f,  0.08f, 1.01f, 4200.0f, -0.14f, freq[11].baseFrequencyHz, PadVoiceModel::Fx), 11);
        default: break;
    }
    return {};
}

TargetRow getTargetRow(const KitFamily family)
{
    const int idx = static_cast<int>(family);
    if (idx < 0 || idx >= 5)
        return kTargetMatrix[0];
    return kTargetMatrix[idx];
}

void applyTargetMatrix(KitPreset& kit, const KitFamily family)
{
    const auto& t = getTargetRow(family);

    // Helper — nudge pad level toward target, keep within [0,1]
    auto nudgeLevel = [](float current, float target) {
        return std::clamp(current * 0.7f + target * 0.3f, 0.10f, 1.00f);
    };

    // Kick pads (0-1)
    for (int i = 0; i < 2; ++i)
    {
        auto& p = kit.pads[static_cast<std::size_t>(i)];
        p.level = nudgeLevel(p.level, t.kick.level);
        applyDensityToPad(p, t.kick.density);
    }
    // Snare pads (2-3)
    for (int i = 2; i < 4; ++i)
    {
        auto& p = kit.pads[static_cast<std::size_t>(i)];
        p.level = nudgeLevel(p.level, t.snare.level);
        applyDensityToPad(p, t.snare.density);
    }
    // Hat pads (4-5)
    for (int i = 4; i < 6; ++i)
    {
        auto& p = kit.pads[static_cast<std::size_t>(i)];
        p.level = nudgeLevel(p.level, t.hat.level);
        applyDensityToPad(p, t.hat.density);
    }
    // Percussion pads (6-7): keep them coherent with the kit family without
    // turning them into noisy hat variants.
    {
        const float percLevel = std::clamp(t.snare.level * 0.35f + t.hat.level * 0.45f + t.fx.level * 0.20f,
                                           0.10f, 1.00f);
        const float percDensity = std::clamp(t.snare.density * 0.30f + t.hat.density * 0.35f + t.fx.density * 0.15f,
                                             0.0f, 0.75f);
        for (int i = 6; i < 8; ++i)
        {
            auto& p = kit.pads[static_cast<std::size_t>(i)];
            p.level = nudgeLevel(p.level, percLevel);
            applyDensityToPad(p, percDensity);
        }
    }
    // Toms (8-9): derive their weight from the kick/snare balance.
    {
        const float tomLevel = std::clamp(t.kick.level * 0.52f + t.snare.level * 0.48f, 0.10f, 1.00f);
        const float tomDensity = std::clamp(t.kick.density * 0.40f + t.snare.density * 0.35f, 0.0f, 0.80f);
        for (int i = 8; i < 10; ++i)
        {
            auto& p = kit.pads[static_cast<std::size_t>(i)];
            p.level = nudgeLevel(p.level, tomLevel);
            applyDensityToPad(p, tomDensity);
        }
    }
    // Crash (10)
    {
        auto& p = kit.pads[10];
        p.level = nudgeLevel(p.level, t.crash.level);
        applyDensityToPad(p, t.crash.density);
    }
    // FX (11)
    {
        auto& p = kit.pads[11];
        p.level = nudgeLevel(p.level, t.fx.level);
        applyDensityToPad(p, t.fx.density);
    }

    // -----------------------------------------------------------------------
    // Audit Phase 1 corrections — applied as final post-processing:
    //   1) Enforce snare > perc hierarchy (audit ETAPE 4/8 critical issue).
    //      Original presets had perc levels 0.90-1.00 and snare 0.68-0.76,
    //      which inverts the natural acoustic hierarchy and masks the snare
    //      in groove playback. We clamp perc pads to ≤ 88% of snare avg.
    //   2) Detune Tom High away from Snare quasi-unison.
    //      Tom High at 250 Hz vs Snare at 248 Hz creates a 1.008 ratio
    //      (audibly fused). Force Tom High to a musical interval (~ minor
    //      third below the snare body). Tune offset is preserved so the
    //      user's "TUNE" knob keeps the same musical effect.
    // These corrections are internal: no exposed parameter, ID, range or
    // default is changed — only the factory preset values are recalibrated.
    // -----------------------------------------------------------------------
    {
        const float snareAvg = (kit.pads[2].level + kit.pads[3].level) * 0.5f;
        const float maxPercLevel = std::clamp(snareAvg * 0.88f, 0.10f, 1.00f);
        for (int i = 6; i < 8; ++i)
        {
            auto& p = kit.pads[static_cast<std::size_t>(i)];
            if (p.level > maxPercLevel)
                p.level = maxPercLevel;
        }
    }
    if (kit.pads[9].voiceModel == PadVoiceModel::Tom)
    {
        // 195 Hz vs Snare 248 Hz → ratio 0.786 (≈ minor third). Avoids
        // quasi-unison masking while keeping Tom High clearly above Tom Low (175 Hz).
        kit.pads[9].baseFrequencyHz = 195.0f;
    }
}

const std::vector<KitPreset>& getFactoryPresets()
{
    return buildFactoryPresets();
}

const std::vector<PadPreset>& getFactoryPadPresets(const int padIndex)
{
    switch (padIndex)
    {
        case 0:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeKickPresets(), 0), 0);      return p; }
        case 1:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeKickBPresets(), 1), 1);     return p; }
        case 2:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeSnarePresets(), 2), 2);     return p; }
        case 3:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeClapPresets(), 3), 3);      return p; }
        case 4:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeHatClosedPresets(), 4), 4); return p; }
        case 5:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeHatOpenPresets(), 5), 5);   return p; }
        case 6:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makePerc1Presets(), 6), 6);     return p; }
        case 7:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makePerc2Presets(), 7), 7);     return p; }
        case 8:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeTomLowPresets(), 8), 8);    return p; }
        case 9:  { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeTomHighPresets(), 9), 9);   return p; }
        case 10: { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeCrashPresets(), 10), 10);   return p; }
        case 11: { static const auto p = stampPadPresetIdentities(makeFactoryPadBankWithCurated8(makeFxPresets(), 11), 11);      return p; }
        default:
        {
            static const std::vector<PadPreset> empty;
            return empty;
        }
    }
}

} // namespace mds
