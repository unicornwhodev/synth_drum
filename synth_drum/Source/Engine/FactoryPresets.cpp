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
    k.pads[11]= makePad(0.50f, 5.0f,  0.09f,  0.0002f,  9.0f, 0.022f, 0.03f,  0.10f, 1.02f, 4000.0f, -0.14f, 560.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.40f; k.fx.reverbDamping=0.68f; k.fx.reverbWidth=0.82f; k.fx.reverbMix=0.16f; k.fx.reverbPredelay=10.0f;
    k.fx.compThreshold=-12.0f; k.fx.compRatio=2.0f; k.fx.compAttack=18.0f; k.fx.compRelease=150.0f; k.fx.compMix=0.35f;
    k.fx.satDrive=1.10f; k.fx.satMix=0.06f;
    k.fx.transientAttack=0.06f; k.fx.transientSustain=-0.03f; k.fx.transientMix=0.14f;
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
    k.pads[0].decaySeconds = 0.48f;
    k.pads[2].decaySeconds = 0.145f;
    k.pads[5].decaySeconds = 0.160f;
    k.pads[10].decaySeconds= 0.38f;
    k.fx.reverbMix = 0.22f; k.fx.reverbSize = 0.55f; k.fx.reverbPredelay = 18.0f;
    k.fx.compMix   = 0.20f;
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
    k.pads[11]= makePad(0.52f, 4.0f,  0.10f,  0.0002f, 7.0f, 0.024f, 0.04f,  0.09f, 1.00f, 3600.0f, -0.14f, 560.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.52f; k.fx.reverbDamping=0.62f; k.fx.reverbWidth=0.78f; k.fx.reverbMix=0.20f; k.fx.reverbPredelay=14.0f;
    k.fx.compThreshold=-14.0f; k.fx.compRatio=1.8f; k.fx.compAttack=22.0f; k.fx.compRelease=180.0f; k.fx.compMix=0.28f;
    k.fx.satDrive=1.04f; k.fx.satMix=0.03f;
    k.fx.transientAttack=0.04f; k.fx.transientSustain=-0.02f; k.fx.transientMix=0.10f;
    k.fx.limiterEnable=true; k.fx.limiterThreshold=-0.5f;
    applyTargetMatrix(k, KitFamily::Acoustique);
    return k;
}

static KitPreset makeKit_Acoustique_Studio()
{
    auto k = makeKit_Acoustique_Room();
    k.name = "Acoustique Studio";
    k.fx.reverbSize=0.36f; k.fx.reverbDamping=0.78f; k.fx.reverbMix=0.13f; k.fx.reverbPredelay=8.0f;
    k.fx.compThreshold=-10.0f; k.fx.compRatio=2.2f; k.fx.compMix=0.40f;
    k.fx.transientAttack=0.08f; k.fx.transientMix=0.16f;
    // Bring snare up a touch
    k.pads[2].level *= 1.05f;
    k.pads[2].noiseAmount += 0.05f;
    applyTargetMatrix(k, KitFamily::Acoustique);
    return k;
}

static KitPreset makeKit_Acoustique_Brush()
{
    auto k = makeKit_Acoustique_Room();
    k.name = "Acoustique Brush";
    // Brush snare: more noise, lower click, softer
    k.pads[2].noiseAmount = 0.78f; k.pads[2].clickAmount = 0.01f; k.pads[2].cutoffHz = 4200.0f;
    k.pads[2].drive = 1.00f;
    // Softer kick
    k.pads[0].level = 0.76f; k.pads[0].drive = 1.00f;
    // Quieter hats
    k.pads[4].level = 0.44f; k.pads[5].level = 0.40f;
    k.fx.reverbMix=0.26f; k.fx.satMix=0.01f; k.fx.compMix=0.18f;
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
    k.pads[11]= makePad(0.48f, 6.0f,  0.14f,  0.0003f, 6.0f, 0.028f, 0.05f,  0.09f, 1.00f, 3200.0f, -0.16f, 560.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.68f; k.fx.reverbDamping=0.55f; k.fx.reverbWidth=0.90f; k.fx.reverbMix=0.28f; k.fx.reverbPredelay=22.0f;
    k.fx.compThreshold=-16.0f; k.fx.compRatio=1.6f; k.fx.compAttack=28.0f; k.fx.compRelease=200.0f; k.fx.compMix=0.22f;
    k.fx.satDrive=1.02f; k.fx.satMix=0.02f;
    k.fx.transientAttack=0.02f; k.fx.transientSustain=0.0f; k.fx.transientMix=0.08f;
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
    k.fx.reverbSize=0.75f; k.fx.reverbDamping=0.45f; k.fx.reverbMix=0.32f; k.fx.reverbPredelay=28.0f;
    k.fx.eqEnable=true; k.fx.eqLowFreq=80.0f; k.fx.eqLowGain=2.0f;
    k.fx.eqHighFreq=8000.0f; k.fx.eqHighGain=-2.5f;
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
    k.fx.reverbSize=0.80f; k.fx.reverbMix=0.35f; k.fx.reverbPredelay=30.0f;
    k.fx.compMix=0.12f;
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
    k.pads[11]= makePad(0.56f, 8.0f,  0.10f,  0.0002f,10.0f, 0.025f, 0.04f,  0.12f, 1.04f, 3800.0f, -0.14f, 560.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.62f; k.fx.reverbDamping=0.50f; k.fx.reverbWidth=0.92f; k.fx.reverbMix=0.22f; k.fx.reverbPredelay=20.0f;
    k.fx.compThreshold=-10.0f; k.fx.compRatio=2.5f; k.fx.compAttack=12.0f; k.fx.compRelease=140.0f; k.fx.compMix=0.45f;
    k.fx.satDrive=1.20f; k.fx.satMix=0.10f;
    k.fx.transientAttack=0.10f; k.fx.transientSustain=-0.04f; k.fx.transientMix=0.18f;
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
    // Electronic kick blended
    k.pads[1].decaySeconds=0.14f; k.pads[1].pitchDropSemitones=8.0f; k.pads[1].noiseAmount=0.002f;
    k.pads[1].cutoffHz=1600.0f;
    // Layered snare: extra click
    k.pads[2].clickAmount=0.12f;
    k.fx.delayEnable=true; k.fx.delayTime=250.0f; k.fx.delayFeedback=0.22f; k.fx.delayMix=0.12f;
    k.fx.reverbMix=0.18f;
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
    k.pads[11]= makePad(0.50f, 7.0f,  0.07f,  0.0002f, 9.0f, 0.020f, 0.02f,  0.10f, 1.02f, 4400.0f, -0.14f, 560.0f, PadVoiceModel::Fx);

    k.fx.reverbSize=0.34f; k.fx.reverbDamping=0.74f; k.fx.reverbWidth=0.82f; k.fx.reverbMix=0.13f; k.fx.reverbPredelay=8.0f;
    k.fx.compThreshold=-11.0f; k.fx.compRatio=2.2f; k.fx.compAttack=14.0f; k.fx.compRelease=130.0f; k.fx.compMix=0.42f;
    k.fx.satDrive=1.18f; k.fx.satMix=0.08f;
    k.fx.transientAttack=0.09f; k.fx.transientSustain=-0.04f; k.fx.transientMix=0.16f;
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
    k.fx.satDrive=2.2f; k.fx.satMix=0.25f;
    k.fx.eqEnable=true; k.fx.eqHighFreq=5000.0f; k.fx.eqHighGain=-4.0f;
    k.fx.eqLowFreq=200.0f; k.fx.eqLowGain=-1.0f;
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
    k.pads[0].decaySeconds=0.55f; k.pads[0].pitchDropSemitones=14.0f; k.pads[0].cutoffHz=1200.0f; k.pads[0].level=0.92f;
    k.pads[2].decaySeconds=0.06f; k.pads[2].noiseAmount=0.72f; k.pads[2].drive=1.08f;
    k.pads[3].noiseAmount=0.70f; k.pads[3].level=0.72f;
    k.pads[4].decaySeconds=0.014f; k.pads[4].level=0.50f;
    k.pads[5].decaySeconds=0.050f;
    k.fx.compThreshold=-8.0f; k.fx.compRatio=3.0f; k.fx.compAttack=8.0f; k.fx.compMix=0.50f;
    k.fx.transientAttack=0.14f; k.fx.transientMix=0.20f;
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
// Per-pad presets (5-10 per pad slot)
// =========================================================================

static std::vector<PadPreset> makeKickPresets()
{
    const float f = kPadCharacteristics[0].baseFrequencyHz;
    return {
        { "Kick Deep",     makePad(0.90f,  0.0f, 0.40f, 0.0004f,  6.0f, 0.040f, 0.003f, 0.08f, 1.02f, 2200.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick Punchy",   makePad(0.88f,  1.0f, 0.26f, 0.0003f,  8.0f, 0.028f, 0.004f, 0.14f, 1.08f, 2800.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick Tight",    makePad(0.86f,  2.0f, 0.16f, 0.0003f,  6.0f, 0.022f, 0.004f, 0.18f, 1.05f, 3200.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick808",       makePad(0.92f, -2.0f, 0.60f, 0.0005f, 14.0f, 0.055f, 0.002f, 0.05f, 1.00f, 1200.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick Click",    makePad(0.82f,  3.0f, 0.20f, 0.0002f,  5.0f, 0.025f, 0.002f, 0.28f, 1.04f, 4000.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick Room",     makePad(0.84f,  0.0f, 0.45f, 0.0005f,  4.0f, 0.042f, 0.010f, 0.10f, 1.01f, 2000.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick Sub",      makePad(0.94f, -4.0f, 0.55f, 0.0006f, 10.0f, 0.060f, 0.002f, 0.06f, 1.00f, 1000.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick Layered",  makePad(0.88f,  1.5f, 0.30f, 0.0003f,  7.0f, 0.032f, 0.006f, 0.20f, 1.06f, 2600.0f, 0.0f, f, PadVoiceModel::Kick) },
    };
}

static std::vector<PadPreset> makeKickBPresets()
{
    const float f = kPadCharacteristics[1].baseFrequencyHz;
    return {
        { "Kick B Short",    makePad(0.70f,  6.0f, 0.12f, 0.0003f, 3.0f, 0.022f, 0.003f, 0.22f, 1.02f, 2400.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick B Mid",      makePad(0.72f,  5.0f, 0.18f, 0.0004f, 2.5f, 0.026f, 0.003f, 0.20f, 1.02f, 2200.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick B Accent",   makePad(0.80f,  4.0f, 0.16f, 0.0003f, 4.0f, 0.024f, 0.004f, 0.24f, 1.06f, 2800.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick B Hard",     makePad(0.76f,  7.0f, 0.14f, 0.0002f, 5.0f, 0.020f, 0.003f, 0.30f, 1.10f, 3000.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick B Jazzy",    makePad(0.64f,  3.0f, 0.22f, 0.0005f, 1.5f, 0.032f, 0.008f, 0.18f, 1.00f, 1800.0f, 0.0f, f, PadVoiceModel::Kick) },
        { "Kick B Boomy",    makePad(0.74f,  2.0f, 0.28f, 0.0005f, 2.0f, 0.038f, 0.005f, 0.14f, 1.01f, 1600.0f, 0.0f, f, PadVoiceModel::Kick) },
    };
}

static std::vector<PadPreset> makeSnarePresets()
{
    const float f = kPadCharacteristics[2].baseFrequencyHz;
    return {
        { "Snare Crisp",    makePad(0.76f,  8.0f, 0.09f, 0.0002f, 0.3f, 0.011f, 0.65f, 0.07f, 1.02f, 6400.0f, -0.02f, f, PadVoiceModel::Snare) },
        { "Snare Fat",      makePad(0.78f,  6.0f, 0.14f, 0.0003f, 0.2f, 0.015f, 0.60f, 0.05f, 1.01f, 5200.0f, -0.02f, f, PadVoiceModel::Snare) },
        { "Snare Tight",    makePad(0.74f, 10.0f, 0.07f, 0.0002f, 0.1f, 0.008f, 0.68f, 0.08f, 1.04f, 7000.0f, -0.02f, f, PadVoiceModel::Snare) },
        { "Snare Punchy",   makePad(0.80f,  7.0f, 0.10f, 0.0002f, 0.4f, 0.010f, 0.62f, 0.10f, 1.06f, 6000.0f, -0.02f, f, PadVoiceModel::Snare) },
        { "Snare Sidestick",makePad(0.70f, 12.0f, 0.06f, 0.0001f, 0.0f, 0.007f, 0.55f, 0.15f, 1.02f, 8000.0f,  0.02f, f, PadVoiceModel::Snare) },
        { "Snare Brush",    makePad(0.68f,  5.0f, 0.13f, 0.0003f, 0.1f, 0.016f, 0.78f, 0.01f, 1.00f, 4200.0f, -0.02f, f, PadVoiceModel::Snare) },
        { "Snare Half-Time",makePad(0.74f,  6.0f, 0.18f, 0.0003f, 0.2f, 0.018f, 0.58f, 0.05f, 1.01f, 5600.0f, -0.02f, f, PadVoiceModel::Snare) },
        { "Snare Dull",     makePad(0.72f,  4.0f, 0.12f, 0.0003f, 0.1f, 0.014f, 0.52f, 0.04f, 1.01f, 4800.0f, -0.02f, f, PadVoiceModel::Snare) },
    };
}

static std::vector<PadPreset> makeClapPresets()
{
    const float f = kPadCharacteristics[3].baseFrequencyHz;
    return {
        { "Clap Natural",    makePad(0.68f, -5.0f, 0.20f, 0.0002f, 0.0f, 0.016f, 0.62f, 0.02f, 1.00f, 3200.0f, 0.04f, f, PadVoiceModel::Clap) },
        { "Clap Room",       makePad(0.70f, -4.0f, 0.28f, 0.0003f, 0.0f, 0.020f, 0.60f, 0.02f, 1.00f, 2800.0f, 0.04f, f, PadVoiceModel::Clap) },
        { "Clap Tight",      makePad(0.66f, -6.0f, 0.12f, 0.0002f, 0.0f, 0.012f, 0.66f, 0.02f, 1.00f, 3800.0f, 0.04f, f, PadVoiceModel::Clap) },
        { "Clap Dirty",      makePad(0.72f, -4.0f, 0.22f, 0.0002f, 0.0f, 0.018f, 0.68f, 0.03f, 1.02f, 2800.0f, 0.04f, f, PadVoiceModel::Clap) },
        { "Clap Electronic", makePad(0.74f, -6.0f, 0.15f, 0.0001f, 0.0f, 0.010f, 0.72f, 0.04f, 1.04f, 4200.0f, 0.04f, f, PadVoiceModel::Clap) },
        { "Clap Soft",       makePad(0.60f, -3.0f, 0.24f, 0.0003f, 0.0f, 0.022f, 0.55f, 0.01f, 1.00f, 2400.0f, 0.04f, f, PadVoiceModel::Clap) },
    };
}

static std::vector<PadPreset> makeHatClosedPresets()
{
    const float f = kPadCharacteristics[4].baseFrequencyHz;
    return {
        { "HH Closed Tight",  makePad(0.50f,  1.0f, 0.014f, 0.0f, 0.0f, 0.006f, 0.64f, 0.13f, 1.02f,  9600.0f, -0.12f, f, PadVoiceModel::Hat) },
        { "HH Closed Mid",    makePad(0.50f,  0.0f, 0.020f, 0.0f, 0.0f, 0.007f, 0.62f, 0.11f, 1.00f,  8800.0f, -0.12f, f, PadVoiceModel::Hat) },
        { "HH Closed Crisp",  makePad(0.53f,  2.0f, 0.018f, 0.0f, 0.0f, 0.006f, 0.66f, 0.15f, 1.03f,  9800.0f, -0.12f, f, PadVoiceModel::Hat) },
        { "HH Closed Dusty",  makePad(0.46f, -2.0f, 0.026f, 0.0f, 0.0f, 0.008f, 0.56f, 0.08f, 1.00f,  7800.0f, -0.12f, f, PadVoiceModel::Hat) },
        { "HH Closed Loose",  makePad(0.48f, -1.0f, 0.032f, 0.0f, 0.0f, 0.009f, 0.60f, 0.09f, 1.00f,  8200.0f, -0.12f, f, PadVoiceModel::Hat) },
        { "HH Pedal",         makePad(0.44f, -4.0f, 0.030f, 0.0f, 0.0f, 0.010f, 0.52f, 0.11f, 1.01f,  7400.0f, -0.12f, f, PadVoiceModel::Hat) },
    };
}

static std::vector<PadPreset> makeHatOpenPresets()
{
    const float f = kPadCharacteristics[5].baseFrequencyHz;
    return {
        { "HH Open Short",   makePad(0.44f,  0.0f, 0.050f, 0.0f, 0.0f, 0.008f, 0.58f, 0.09f, 1.00f,  7600.0f, 0.12f, f, PadVoiceModel::Hat) },
        { "HH Open Mid",     makePad(0.42f,  0.0f, 0.085f, 0.0f, 0.0f, 0.009f, 0.60f, 0.07f, 1.00f,  7200.0f, 0.12f, f, PadVoiceModel::Hat) },
        { "HH Open Long",    makePad(0.42f, -1.0f, 0.145f, 0.0f, 0.0f, 0.010f, 0.58f, 0.06f, 1.00f,  6800.0f, 0.12f, f, PadVoiceModel::Hat) },
        { "HH Open Bright",  makePad(0.46f,  1.0f, 0.095f, 0.0f, 0.0f, 0.009f, 0.66f, 0.11f, 1.02f,  8200.0f, 0.12f, f, PadVoiceModel::Hat) },
        { "HH Open Dark",    makePad(0.40f, -2.0f, 0.115f, 0.0f, 0.0f, 0.010f, 0.52f, 0.06f, 1.00f,  6400.0f, 0.12f, f, PadVoiceModel::Hat) },
        { "HH Wash",         makePad(0.46f,  0.0f, 0.180f, 0.0f, 0.0f, 0.011f, 0.64f, 0.07f, 1.01f,  7600.0f, 0.12f, f, PadVoiceModel::Hat) },
    };
}

static std::vector<PadPreset> makePerc1Presets()
{
    const float f = kPadCharacteristics[6].baseFrequencyHz;
    return {
        { "Wood Low",      makePad(0.90f, -5.0f, 0.082f, 0.0001f, 6.0f, 0.018f, 0.08f, 0.06f, 1.00f, 1200.0f, -0.18f, f, PadVoiceModel::PercWood) },
        { "Wood Mid",      makePad(0.92f, -2.0f, 0.072f, 0.0001f, 5.0f, 0.016f, 0.07f, 0.08f, 1.00f, 1600.0f, -0.18f, f, PadVoiceModel::PercWood) },
        { "Wood High",     makePad(0.88f,  2.0f, 0.062f, 0.0001f, 4.0f, 0.014f, 0.08f, 0.10f, 1.00f, 2100.0f, -0.18f, f, PadVoiceModel::PercWood) },
        { "Wood Dry",      makePad(0.88f, -3.0f, 0.055f, 0.0001f, 6.0f, 0.014f, 0.05f, 0.12f, 1.00f, 1400.0f, -0.18f, f, PadVoiceModel::PercWood) },
        { "Tabla",         makePad(0.84f,  0.0f, 0.105f, 0.0002f, 8.0f, 0.020f, 0.06f, 0.05f, 1.01f, 1100.0f, -0.12f, f, PadVoiceModel::PercWood) },
        { "Conga",         makePad(0.86f,  3.0f, 0.130f, 0.0002f, 4.0f, 0.024f, 0.06f, 0.07f, 1.00f, 1800.0f, -0.10f, f, PadVoiceModel::PercWood) },
        { "Cajon",         makePad(0.90f, -4.0f, 0.090f, 0.0001f, 6.0f, 0.018f, 0.10f, 0.09f, 1.02f, 1300.0f, -0.14f, f, PadVoiceModel::PercWood) },
    };
}

static std::vector<PadPreset> makePerc2Presets()
{
    const float f = kPadCharacteristics[7].baseFrequencyHz;
    return {
        { "Metal Low",     makePad(0.86f, -7.0f, 0.080f, 0.0001f, 7.0f, 0.018f, 0.09f, 0.05f, 1.00f, 1800.0f, 0.16f, f, PadVoiceModel::PercMetal) },
        { "Metal Mid",     makePad(0.84f, -3.0f, 0.075f, 0.0001f, 8.0f, 0.018f, 0.08f, 0.06f, 1.00f, 2400.0f, 0.16f, f, PadVoiceModel::PercMetal) },
        { "Metal Bright",  makePad(0.82f,  2.0f, 0.070f, 0.0001f, 6.0f, 0.016f, 0.10f, 0.08f, 1.00f, 3400.0f, 0.16f, f, PadVoiceModel::PercMetal) },
        { "Cowbell",       makePad(0.84f,  5.0f, 0.135f, 0.0001f, 9.0f, 0.024f, 0.06f, 0.08f, 1.02f, 2800.0f, 0.16f, f, PadVoiceModel::PercMetal) },
        { "Rim Shot",      makePad(0.82f,  1.0f, 0.050f, 0.0001f, 4.0f, 0.012f, 0.10f, 0.16f, 1.03f, 4200.0f, 0.12f, f, PadVoiceModel::PercMetal) },
        { "Tambourine",    makePad(0.78f,  4.0f, 0.110f, 0.0001f, 3.0f, 0.015f, 0.18f, 0.04f, 1.00f, 5200.0f, 0.18f, f, PadVoiceModel::PercMetal) },
        { "Cowbell High",  makePad(0.80f,  8.0f, 0.150f, 0.0001f,10.0f, 0.022f, 0.06f, 0.07f, 1.02f, 3600.0f, 0.16f, f, PadVoiceModel::PercMetal) },
    };
}

static std::vector<PadPreset> makeTomLowPresets()
{
    const float f = kPadCharacteristics[8].baseFrequencyHz;
    return {
        { "Tom Low Deep",   makePad(0.62f, -2.0f, 0.14f, 0.0004f, 4.0f, 0.032f, 0.006f, 0.09f, 1.01f, 2800.0f, -0.08f, f, PadVoiceModel::Tom) },
        { "Tom Low Mid",    makePad(0.60f,  0.0f, 0.12f, 0.0004f, 3.5f, 0.028f, 0.006f, 0.09f, 1.01f, 3000.0f, -0.08f, f, PadVoiceModel::Tom) },
        { "Tom Low Tight",  makePad(0.58f,  1.0f, 0.09f, 0.0003f, 3.0f, 0.022f, 0.006f, 0.10f, 1.01f, 3400.0f, -0.08f, f, PadVoiceModel::Tom) },
        { "Tom Low Big",    makePad(0.66f, -3.0f, 0.18f, 0.0005f, 4.5f, 0.038f, 0.008f, 0.08f, 1.00f, 2400.0f, -0.10f, f, PadVoiceModel::Tom) },
        { "Tom Low Dry",    makePad(0.60f,  0.0f, 0.10f, 0.0003f, 3.0f, 0.025f, 0.005f, 0.10f, 1.02f, 3200.0f, -0.06f, f, PadVoiceModel::Tom) },
    };
}

static std::vector<PadPreset> makeTomHighPresets()
{
    const float f = kPadCharacteristics[9].baseFrequencyHz;
    return {
        { "Tom High Crisp",  makePad(0.58f, 2.0f, 0.10f, 0.0004f, 3.0f, 0.025f, 0.006f, 0.08f, 1.01f, 3800.0f, 0.08f, f, PadVoiceModel::Tom) },
        { "Tom High Mid",    makePad(0.56f, 0.0f, 0.10f, 0.0004f, 2.5f, 0.024f, 0.006f, 0.08f, 1.01f, 4000.0f, 0.08f, f, PadVoiceModel::Tom) },
        { "Tom High Bright", makePad(0.56f, 3.0f, 0.09f, 0.0003f, 2.0f, 0.020f, 0.007f, 0.10f, 1.02f, 4400.0f, 0.08f, f, PadVoiceModel::Tom) },
        { "Tom High Tight",  makePad(0.54f, 2.0f, 0.07f, 0.0003f, 2.0f, 0.018f, 0.007f, 0.11f, 1.02f, 4800.0f, 0.06f, f, PadVoiceModel::Tom) },
        { "Tom High Big",    makePad(0.60f, 0.0f, 0.13f, 0.0005f, 3.5f, 0.030f, 0.008f, 0.07f, 1.00f, 3400.0f, 0.10f, f, PadVoiceModel::Tom) },
    };
}

static std::vector<PadPreset> makeCrashPresets()
{
    const float f = kPadCharacteristics[10].baseFrequencyHz;
    return {
        { "Crash Short",    makePad(0.44f, 0.0f, 0.18f, 0.0f, 0.0f, 0.011f, 0.55f, 0.06f, 1.00f, 8000.0f, 0.20f, f, PadVoiceModel::Crash) },
        { "Crash Mid",      makePad(0.44f, 0.0f, 0.26f, 0.0f, 0.0f, 0.012f, 0.54f, 0.06f, 1.00f, 7600.0f, 0.20f, f, PadVoiceModel::Crash) },
        { "Crash Long",     makePad(0.44f, 0.0f, 0.45f, 0.0f, 0.0f, 0.014f, 0.52f, 0.05f, 1.00f, 7200.0f, 0.20f, f, PadVoiceModel::Crash) },
        { "Crash Bright",   makePad(0.46f, 0.0f, 0.28f, 0.0f, 0.0f, 0.012f, 0.60f, 0.08f, 1.00f, 9000.0f, 0.18f, f, PadVoiceModel::Crash) },
        { "Crash Dark",     makePad(0.42f, 0.0f, 0.32f, 0.0f, 0.0f, 0.013f, 0.48f, 0.05f, 1.00f, 6400.0f, 0.22f, f, PadVoiceModel::Crash) },
        { "Crash Stack",    makePad(0.38f, 0.0f, 0.12f, 0.0f, 0.0f, 0.010f, 0.62f, 0.07f, 1.00f,10000.0f, 0.16f, f, PadVoiceModel::Crash) },
        { "Ride Bell",      makePad(0.50f, 0.0f, 0.22f, 0.0f, 0.0f, 0.012f, 0.45f, 0.06f, 1.00f, 7000.0f, 0.14f, f, PadVoiceModel::Crash) },
    };
}

static std::vector<PadPreset> makeFxPresets()
{
    const float f = kPadCharacteristics[11].baseFrequencyHz;
    return {
        { "FX Zip",        makePad(0.52f,  8.0f, 0.07f, 0.0002f, 14.0f, 0.018f, 0.02f, 0.12f, 1.04f, 4000.0f, -0.14f, f, PadVoiceModel::Fx) },
        { "FX Sweep",      makePad(0.56f,  6.0f, 0.14f, 0.0003f, 12.0f, 0.030f, 0.03f, 0.10f, 1.02f, 3800.0f, -0.14f, f, PadVoiceModel::Fx) },
        { "FX Impact",     makePad(0.60f,  0.0f, 0.10f, 0.0002f, 10.0f, 0.022f, 0.04f, 0.14f, 1.06f, 3200.0f, -0.14f, f, PadVoiceModel::Fx) },
        { "FX Noise Burst",makePad(0.50f,  4.0f, 0.08f, 0.0002f,  8.0f, 0.020f, 0.08f, 0.10f, 1.02f, 5000.0f, -0.14f, f, PadVoiceModel::Fx) },
        { "FX Sub Blip",   makePad(0.58f, -4.0f, 0.05f, 0.0001f, 16.0f, 0.016f, 0.01f, 0.08f, 1.00f, 1600.0f, -0.14f, f, PadVoiceModel::Fx) },
        { "FX Metallic",   makePad(0.48f, 10.0f, 0.12f, 0.0002f,  6.0f, 0.025f, 0.06f, 0.12f, 1.04f, 5600.0f,  0.10f, f, PadVoiceModel::Fx) },
        { "FX Glitch",     makePad(0.54f,  7.0f, 0.04f, 0.0001f, 12.0f, 0.014f, 0.05f, 0.16f, 1.08f, 6000.0f, -0.10f, f, PadVoiceModel::Fx) },
        { "FX Drone",      makePad(0.44f,  2.0f, 0.30f, 0.0004f,  4.0f, 0.040f, 0.06f, 0.08f, 1.00f, 2800.0f, -0.14f, f, PadVoiceModel::Fx) },
    };
}

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
        set("Classique", "foundation", "Kit serre et polyvalent pour grooves generaux.", "master-ready", -1.0f, { "dry", "balanced", "punchy" });
        preset.fx.outputGainDb -= 0.80f;
    }
    else if (preset.name == "Classique Tight")
    {
        set("Classique", "tight", "Version plus courte et plus stricte pour arrangements denses.", "master-ready", -0.7f, { "tight", "controlled", "dry" });
        preset.fx.outputGainDb -= 0.40f;
    }
    else if (preset.name == "Classique Open")
    {
        set("Classique", "open", "Version plus ample avec davantage d'air et de queue.", "master-ready", -1.0f, { "open", "roomy", "wide" });
        preset.fx.outputGainDb -= 0.90f;
    }
    else if (preset.name == "Acoustique Room")
        set("Acoustique", "natural", "Kit acoustique avec ambience de piece moderee.", "master-ready", -0.6f, { "natural", "room", "organic" });
    else if (preset.name == "Acoustique Studio")
        set("Acoustique", "mix-ready", "Kit acoustique plus compresse et recentre pour le mix.", "master-ready", -0.6f, { "studio", "focused", "mix-ready" });
    else if (preset.name == "Acoustique Brush")
        set("Acoustique", "soft", "Kit doux a balais et densite reduite.", "master-ready", -0.7f, { "soft", "brush", "organic" });
    else if (preset.name == "Acoustique Jazz")
    {
        set("Acoustique", "airy", "Kit leger et ouvert pour jeu jazz ou fusion.", "master-ready", -0.6f, { "jazz", "airy", "light" });
        preset.fx.outputGainDb -= 0.60f;
    }
    else if (preset.name == "Ambient Pad")
        set("Ambient", "wash", "Kit ambient large avec longues queues et espace stereo.", "master-ready", -0.5f, { "ambient", "wide", "wash" });
    else if (preset.name == "Ambient Dark")
    {
        set("Ambient", "dark", "Kit sombre et plus retenu pour textures lentes.", "master-ready", -0.8f, { "ambient", "dark", "textured" });
        preset.fx.outputGainDb -= 0.90f;
    }
    else if (preset.name == "Ambient Sparse")
        set("Ambient", "sparse", "Kit eclairci et moins dense pour arrangements minimalistes.", "master-ready", -0.7f, { "ambient", "sparse", "minimal" });
    else if (preset.name == "Cinematique Epic")
    {
        set("Cinematique", "cinematic", "Kit large et impactant pour trailers et percussion hybride.", "master-ready", -0.7f, { "cinematic", "epic", "wide" });
        preset.fx.outputGainDb -= 0.70f;
    }
    else if (preset.name == "Cinematique Tension")
        set("Cinematique", "aggressive", "Version plus tendue et plus compressee pour pics de tension.", "master-ready", -0.5f, { "cinematic", "aggressive", "tense" });
    else if (preset.name == "Cinematique Hybrid")
        set("Cinematique", "hybrid", "Fusion acoustique-electronique avec delay et attaque renforcee.", "master-ready", -0.3f, { "cinematic", "hybrid", "designed" });
    else if (preset.name == "Cinematique Percussion")
        set("Cinematique", "percussion-forward", "Version centree percussions et toms pour beds rythmiques.", "master-ready", -0.4f, { "cinematic", "percussion", "ensemble" });
    else if (preset.name == "Moderne Club")
        set("Moderne", "club", "Kit moderne dense et direct pour grooves dance et pop.", "master-ready", -0.6f, { "modern", "club", "punchy" });
    else if (preset.name == "Moderne Lo-Fi")
        set("Moderne", "lofi", "Kit degrade et assombri avec saturation et coupe haute.", "master-ready", -1.6f, { "modern", "lofi", "textured" });
    else if (preset.name == "Moderne Trap")
        set("Moderne", "trap", "Kit sub-heavy et sec pour patterns trap et hip-hop modernes.", "master-ready", -0.6f, { "modern", "trap", "sub" });
    else if (preset.name == "Moderne Electro")
        set("Moderne", "electro", "Kit synthetique brillant avec chorus et delay synchronise.", "master-ready", -0.6f, { "modern", "electro", "synthetic" });
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
            applyKitMetadata(preset);

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
        case 0:  return makePad(0.90f, 1.5f,  0.30f,  0.0004f,  5.0f, 0.032f, 0.003f, 0.08f, 1.02f, 3000.0f,  0.0f,  freq[0].baseFrequencyHz, PadVoiceModel::Kick);
        case 1:  return makePad(0.68f, 7.0f,  0.13f,  0.0004f,  2.0f, 0.022f, 0.002f, 0.20f, 1.02f, 2200.0f,  0.0f,  freq[1].baseFrequencyHz, PadVoiceModel::Kick);
        case 2:  return makePad(0.74f, 9.0f,  0.09f,  0.0002f,  0.3f, 0.010f, 0.68f,  0.04f, 1.01f, 6200.0f, -0.02f, freq[2].baseFrequencyHz, PadVoiceModel::Snare);
        case 3:  return makePad(0.64f,-6.0f,  0.195f, 0.0002f,  0.0f, 0.014f, 0.66f,  0.01f, 1.00f, 3200.0f,  0.04f, freq[3].baseFrequencyHz, PadVoiceModel::Clap);
        case 4:  return makePad(0.50f, 0.0f,  0.020f, 0.0f,     0.0f, 0.007f, 0.64f,  0.12f, 1.00f, 8800.0f, -0.12f, freq[4].baseFrequencyHz, PadVoiceModel::Hat);
        case 5:  return makePad(0.44f, 0.0f,  0.080f, 0.0f,     0.0f, 0.009f, 0.60f,  0.08f, 1.00f, 7200.0f,  0.12f, freq[5].baseFrequencyHz, PadVoiceModel::Hat);
        case 6:  return makePad(0.92f,-5.0f,  0.082f, 0.0001f,  6.0f, 0.018f, 0.10f,  0.07f, 1.00f, 1600.0f, -0.18f, freq[6].baseFrequencyHz, PadVoiceModel::PercWood);
        case 7:  return makePad(0.86f,-7.0f,  0.088f, 0.0001f,  8.0f, 0.018f, 0.10f,  0.06f, 1.00f, 2400.0f,  0.16f, freq[7].baseFrequencyHz, PadVoiceModel::PercMetal);
        case 8:  return makePad(0.58f, 0.0f,  0.12f,  0.0004f,  3.0f, 0.028f, 0.004f, 0.08f, 1.01f, 3200.0f, -0.08f, freq[8].baseFrequencyHz, PadVoiceModel::Tom);
        case 9:  return makePad(0.54f, 0.0f,  0.10f,  0.0004f,  2.5f, 0.024f, 0.004f, 0.07f, 1.01f, 4000.0f,  0.08f, freq[9].baseFrequencyHz, PadVoiceModel::Tom);
        case 10: return makePad(0.24f, 0.0f,  0.24f,  0.0f,     0.0f, 0.012f, 0.56f,  0.06f, 1.00f, 7800.0f,  0.20f, freq[10].baseFrequencyHz, PadVoiceModel::Crash);
        case 11: return makePad(0.28f, 7.0f,  0.08f,  0.0002f,  8.0f, 0.020f, 0.02f,  0.08f, 1.01f, 4200.0f, -0.14f, freq[11].baseFrequencyHz, PadVoiceModel::Fx);
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
}

const std::vector<KitPreset>& getFactoryPresets()
{
    return buildFactoryPresets();
}

const std::vector<PadPreset>& getFactoryPadPresets(const int padIndex)
{
    switch (padIndex)
    {
        case 0:  { static const auto p = makeKickPresets();      return p; }
        case 1:  { static const auto p = makeKickBPresets();     return p; }
        case 2:  { static const auto p = makeSnarePresets();     return p; }
        case 3:  { static const auto p = makeClapPresets();      return p; }
        case 4:  { static const auto p = makeHatClosedPresets(); return p; }
        case 5:  { static const auto p = makeHatOpenPresets();   return p; }
        case 6:  { static const auto p = makePerc1Presets();     return p; }
        case 7:  { static const auto p = makePerc2Presets();     return p; }
        case 8:  { static const auto p = makeTomLowPresets();    return p; }
        case 9:  { static const auto p = makeTomHighPresets();   return p; }
        case 10: { static const auto p = makeCrashPresets();     return p; }
        case 11: { static const auto p = makeFxPresets();        return p; }
        default:
        {
            static const std::vector<PadPreset> empty;
            return empty;
        }
    }
}

} // namespace mds
