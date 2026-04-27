#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

// =============================================================================
// Colour palette — MIS Drum Synth (teal-cyan accent, dark studio background)
// =============================================================================
namespace col
{
    static const juce::Colour bg      { 0xff0D0F13 };
    static const juce::Colour surface { 0xff171A20 };
    static const juce::Colour surfHi  { 0xff20242B };
    static const juce::Colour border  { 0xff303742 };
    static const juce::Colour text    { 0xffEAEAEA };
    static const juce::Colour textDim { 0xff9BA3AE };
    static const juce::Colour textSec { 0xffD6DAE0 };
    static const juce::Colour accent  { 0xff2ED7C5 };

    // Category accent colours (pad identity)
    static const juce::Colour cKick   { 0xff9DBE47 };
    static const juce::Colour cSnare  { 0xff70AED2 };
    static const juce::Colour cHat    { 0xff37D5C2 };
    static const juce::Colour cPerc   { 0xff9A82C2 };
    static const juce::Colour cTom    { 0xff729EBA };
    static const juce::Colour cFx     { 0xffAF9842 };
}

// =============================================================================
// Layout constants — 1340 x 760
// =============================================================================
namespace lay
{
    constexpr int W = 1340, H = 760;
    constexpr int headerH = 56;
    constexpr int pad = 16;

    // Left column: pad grid + macros
    constexpr int leftX = 16;
    constexpr int leftW = 380;

    // Right column: pad params + FX sections
    constexpr int rightX = 410;

    // Pad grid area
    constexpr int gridY = 72;
    constexpr int gridH = 390;

    // Macros area (below pad grid)
    constexpr int macroY = 470;
    constexpr int macroH = 270;

    // Pad params area (right top)
    constexpr int paramsY = 72;
    constexpr int paramsH = 390;

    // FX area (right bottom)
    constexpr int fxY = 470;
    constexpr int fxH = 270;
}

// =============================================================================
// Static tables
// =============================================================================
const std::array<DrumSynthAudioProcessorEditor::CtrlDef,
                 DrumSynthAudioProcessorEditor::kPadCtrlN>
    DrumSynthAudioProcessorEditor::kPadCtrls = {{
        { "Level",       "level", "Volume du pad", "Regle le volume de ce pad par rapport aux autres" },
        { "Tune",        "tune", "Accord fin (demi-tons)", "Ajuste la hauteur en demi-tons — positif = plus aigu" },
        { "Decay",       "decay", "Temps de declin", "Duree du son apres l'attaque — court pour serrer, long pour laisser sonner" },
        { "Attack",      "attack", "Temps d'attaque", "Vitesse de montee du son — augmenter pour un fade-in doux" },
        { "Pitch Drop",  "pitch_drop", "Chute de hauteur", "Amplitude de la chute de frequence a l'attaque — plus de punch sur les kicks" },
        { "Pitch Decay", "pitch_decay", "Vitesse de chute", "Rapidite de la descente en hauteur — lent = effet tom, rapide = impact sec" },
        { "Noise",       "noise", "Niveau de bruit", "Quantite de bruit dans le son — ajoute du grain ou du souffle" },
        { "Click",       "click", "Click d'attaque", "Intensite du click d'attaque — mallet dur ou frappe molle" },
        { "Drive",       "drive", "Distorsion", "Saturation harmonique — chauffe le son progressivement" },
        { "Cutoff",      "cutoff", "Frequence de coupure", "Frequence du filtre passe-bas — ferme pour assombrir, ouvert pour plus de brillance" },
        { "Pan",         "pan", "Panoramique", "Position stereo — gauche/droite dans le champ" }
    }};

const std::array<DrumSynthAudioProcessorEditor::FxDef,
                 DrumSynthAudioProcessorEditor::kMacroN>
    DrumSynthAudioProcessorEditor::kMacroCtrls = {{
        { "Punch",  "macro_punch" },
        { "Weight", "macro_weight" },
        { "Air",    "macro_air" },
        { "Dirt",   "macro_dirt" }
    }};

const std::array<DrumSynthAudioProcessorEditor::FxDef,
                 DrumSynthAudioProcessorEditor::kFxN>
    DrumSynthAudioProcessorEditor::kFxCtrls = {{
        { "Threshold",  "comp_threshold" },    // 0
        { "Ratio",      "comp_ratio" },        // 1
        { "Attack",     "comp_attack" },       // 2
        { "Release",    "comp_release" },      // 3
        { "Makeup",     "comp_makeup" },       // 4
        { "Mix",        "comp_mix" },          // 5
        { "Drive",      "sat_drive" },         // 6
        { "Mix",        "sat_mix" },           // 7
        { "Attack",     "transient_attack" },  // 8
        { "Sustain",    "transient_sustain" }, // 9
        { "Mix",        "transient_mix" },     // 10
        { "Size",       "reverb_size" },       // 11
        { "Damping",    "reverb_damping" },    // 12
        { "Width",      "reverb_width" },      // 13
        { "Mix",        "reverb_mix" }         // 14
    }};

const std::array<DrumSynthAudioProcessorEditor::AdvancedFxPageDef, 5>
    DrumSynthAudioProcessorEditor::kAdvancedFxPages = {{
        { "EQ A", { "EQ", "fx_eq_en" }, { "", nullptr },
            {{{ "Low Freq", "eq_low_freq" }, { "Low Gain", "eq_low_gain" },
               { "Mid Freq", "eq_mid_freq" }, { "Mid Gain", "eq_mid_gain" }}},
            col::cSnare },
        { "EQ B", { "EQ", "fx_eq_en" }, { "", nullptr },
            {{{ "Mid Q", "eq_mid_q" }, { "High Freq", "eq_high_freq" },
               { "High Gain", "eq_high_gain" }, { "PreDelay", "reverb_predelay" }}},
            col::cSnare },
        { "CHORUS", { "Chorus", "fx_chorus_en" }, { "", nullptr },
            {{{ "Rate", "chorus_rate" }, { "Depth", "chorus_depth" },
               { "Mix", "chorus_mix" }, { "", nullptr }}},
            col::cHat },
        { "DELAY", { "Delay", "fx_delay_en" }, { "Sync", "delay_sync" },
            {{{ "Time", "delay_time" }, { "Feedback", "delay_feedback" },
               { "Mix", "delay_mix" }, { "Note Div", "delay_note_div" }}},
            col::cFx },
        { "LIMIT", { "Limiter", "fx_limiter_en" }, { "", nullptr },
            {{{ "Threshold", "limiter_threshold" }, { "Release", "limiter_release" },
               { "", nullptr }, { "", nullptr }}},
            col::accent.darker(0.10f) }
    }};

namespace
{
constexpr float kShellRadius   = 10.0f;
constexpr float kPanelRadius   = 8.0f;
constexpr float kItemRadius    = 6.0f;
constexpr float kButtonRadius  = 4.0f;
constexpr float kShadowLight   = 2.0f;
constexpr float kShadowDeep    = 3.5f;
constexpr float kGlowNormal    = 0.22f;
constexpr float kGlowSelected  = 0.38f;
constexpr float kGlowHover     = 0.30f;

juce::Colour makeAccentGlow(const juce::Colour base, float alpha = 0.16f)
{
    return base.withAlpha(alpha).brighter(0.10f);
}

juce::Colour blendDrumTint(const juce::Colour base, const juce::Colour tint, const float amount)
{
    if (tint.getFloatAlpha() <= 0.0f)
        return base;

    return base.interpolatedWith(tint, juce::jlimit(0.0f, 1.0f, amount));
}

void addDrumBloom(juce::Graphics& g, const juce::Point<float> centre,
                  const float radiusX, const float radiusY, const juce::Colour colour)
{
    if (colour.getFloatAlpha() <= 0.0f || radiusX <= 0.0f || radiusY <= 0.0f)
        return;

    juce::ColourGradient bloom(colour, centre.x, centre.y,
                               juce::Colours::transparentBlack,
                               centre.x + radiusX, centre.y + radiusY, true);
    bloom.addColour(0.52, colour.withAlpha(juce::jlimit(0.0f, 1.0f, colour.getFloatAlpha() * 0.32f)));
    bloom.addColour(0.86, juce::Colours::transparentBlack);
    g.setGradientFill(bloom);
    g.fillEllipse(centre.x - radiusX, centre.y - radiusY, radiusX * 2.0f, radiusY * 2.0f);
}

void fillV5Panel(juce::Graphics& g, const juce::Rectangle<float> bounds, const float radius,
                 const juce::Colour top, const juce::Colour middle, const juce::Colour bottom)
{
    juce::ColourGradient panelGrad(top, bounds.getX(), bounds.getY(),
                                   bottom, bounds.getX(), bounds.getBottom(), false);
    panelGrad.addColour(0.45, middle);
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(bounds, radius);
}

void fillDrumPanel(juce::Graphics& g, const juce::Rectangle<float> bounds, const float radius,
                   const juce::Colour base, const bool active)
{
    fillV5Panel(g, bounds, radius,
                base.brighter(0.12f),
                base,
                base.darker(0.16f));

    auto sheen = bounds.reduced(1.0f, 1.0f);
    sheen = sheen.removeFromTop(bounds.getHeight() * 0.40f);
    juce::ColourGradient sheenGrad(juce::Colours::white.withAlpha(active ? 0.08f : 0.05f),
                                   sheen.getX(), sheen.getY(),
                                   juce::Colours::transparentBlack,
                                   sheen.getX(), sheen.getBottom(), false);
    g.setGradientFill(sheenGrad);
    g.fillRoundedRectangle(sheen, juce::jmax(0.0f, radius - 1.0f));

    g.setColour(juce::Colours::black.withAlpha(0.14f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius, active ? 1.0f : 0.8f);
    g.setColour(active ? col::border.withAlpha(0.72f) : col::border.withAlpha(0.42f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius, active ? 1.0f : 0.8f);
}

void fillDrumRecess(juce::Graphics& g, const juce::Rectangle<float> bounds, const float radius)
{
    juce::ColourGradient recessGrad(col::bg.interpolatedWith(col::border, 0.42f),
                                    bounds.getX(), bounds.getY(),
                                    col::surface.darker(0.34f),
                                    bounds.getX(), bounds.getBottom(), false);
    recessGrad.addColour(0.45, col::surface.darker(0.20f));
    g.setGradientFill(recessGrad);
    g.fillRoundedRectangle(bounds, radius);

    juce::ColourGradient recessShade(juce::Colours::black.withAlpha(0.22f),
                                     bounds.getX(), bounds.getY(),
                                     juce::Colours::transparentBlack,
                                     bounds.getX(), bounds.getY() + bounds.getHeight() * 0.38f, false);
    g.setGradientFill(recessShade);
    g.fillRoundedRectangle(bounds, radius);

    g.setColour(col::border.withAlpha(0.46f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);
}

void drawGlowStrip(juce::Graphics& g, const juce::Rectangle<float> bounds, const juce::Colour accent,
                   const float thickness = 2.0f, const float coreAlpha = 0.90f)
{
    auto glow = bounds.expanded(thickness * 1.8f, thickness * 1.3f);
    g.setColour(makeAccentGlow(accent, 0.14f * coreAlpha));
    g.fillRoundedRectangle(glow, thickness * 1.7f);
    g.setColour(accent.withAlpha(coreAlpha));
    g.fillRoundedRectangle(bounds, thickness);
}

void drawMetalEllipse(juce::Graphics& g, const juce::Rectangle<float> bounds, const float ringThickness = 1.0f)
{
    juce::ColourGradient rimGrad(juce::Colour(0xff8E95A0), bounds.getX(), bounds.getY(),
                                 juce::Colour(0xff2A2F38), bounds.getRight(), bounds.getBottom(), true);
    rimGrad.addColour(0.35, juce::Colour(0xffD0D5DE));
    rimGrad.addColour(0.68, juce::Colour(0xff505862));
    g.setGradientFill(rimGrad);
    g.fillEllipse(bounds);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawEllipse(bounds.reduced(0.6f), ringThickness);
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.drawEllipse(bounds.reduced(1.4f), ringThickness);
}

void fillDrumShellPanel(juce::Graphics& g, const juce::Rectangle<float> bounds, const float radius,
                        const juce::Colour accent, const bool elevated,
                        const juce::Colour tint = juce::Colours::transparentBlack)
{
    auto top = blendDrumTint(juce::Colour(0xff394148), tint, elevated ? 0.22f : 0.16f).withAlpha(elevated ? 0.984f : 0.966f);
    auto upper = blendDrumTint(juce::Colour(0xff2D343A), tint, elevated ? 0.18f : 0.12f).withAlpha(0.990f);
    auto mid = blendDrumTint(juce::Colour(0xff1B2025), tint, elevated ? 0.12f : 0.08f).withAlpha(0.995f);
    auto bottom = blendDrumTint(juce::Colour(0xff101417), tint, elevated ? 0.08f : 0.05f).withAlpha(0.998f);

    juce::ColourGradient shell(top, bounds.getCentreX(), bounds.getY(),
                               bottom, bounds.getCentreX(), bounds.getBottom(), false);
    shell.addColour(0.20, upper);
    shell.addColour(0.56, mid);
    shell.addColour(0.84, bottom.darker(0.08f));
    g.setGradientFill(shell);
    g.fillRoundedRectangle(bounds, radius);

    {
        juce::Graphics::ScopedSaveState scopedState(g);
        auto face = bounds.reduced(2.0f, 2.0f);
        g.reduceClipRegion(face.toNearestInt());
        addDrumBloom(g,
                     { face.getX() + face.getWidth() * 0.26f, face.getY() + face.getHeight() * 0.18f },
                     face.getWidth() * 0.34f, face.getHeight() * 0.16f,
                     juce::Colours::white.withAlpha(0.024f));
        addDrumBloom(g,
                     { face.getX() + face.getWidth() * 0.76f, face.getY() + face.getHeight() * 0.28f },
                     face.getWidth() * 0.26f, face.getHeight() * 0.15f,
                     juce::Colours::white.withAlpha(0.015f));
        addDrumBloom(g,
                     { face.getCentreX(), face.getBottom() - face.getHeight() * 0.08f },
                     face.getWidth() * 0.42f, face.getHeight() * 0.16f,
                     juce::Colours::black.withAlpha(0.15f));

        if (tint.getFloatAlpha() > 0.0f)
        {
            addDrumBloom(g,
                         { face.getX() + face.getWidth() * 0.70f, face.getY() + face.getHeight() * 0.24f },
                         face.getWidth() * 0.30f, face.getHeight() * 0.18f,
                         tint.withAlpha(elevated ? 0.050f : 0.035f));
        }

        auto topTexture = face.reduced(face.getWidth() * 0.10f, 0.0f)
                             .removeFromTop(juce::jlimit(10.0f, 18.0f, face.getHeight() * 0.11f));
        juce::ColourGradient topTextureGrad(juce::Colours::white.withAlpha(0.020f), topTexture.getCentreX(), topTexture.getY(),
                                            juce::Colours::transparentWhite, topTexture.getCentreX(), topTexture.getBottom(), false);
        g.setGradientFill(topTextureGrad);
        g.fillRoundedRectangle(topTexture, juce::jmax(0.0f, radius - 5.0f));
    }

    auto topBevel = bounds.reduced(5.0f, 4.0f).removeFromTop(juce::jlimit(7.0f, 14.0f, bounds.getHeight() * 0.12f));
    juce::ColourGradient topBevelGrad(juce::Colours::white.withAlpha(0.060f), topBevel.getCentreX(), topBevel.getY(),
                                      juce::Colours::transparentWhite, topBevel.getCentreX(), topBevel.getBottom(), false);
    g.setGradientFill(topBevelGrad);
    g.fillRoundedRectangle(topBevel, juce::jmax(0.0f, radius - 4.0f));

    auto lowerMass = bounds.reduced(bounds.getWidth() * 0.08f, 0.0f)
                        .removeFromBottom(juce::jlimit(16.0f, 30.0f, bounds.getHeight() * 0.20f));
    juce::ColourGradient lowerMassGrad(juce::Colours::transparentBlack,
                                       lowerMass.getCentreX(), lowerMass.getY(),
                                       juce::Colours::black.withAlpha(elevated ? 0.18f : 0.14f),
                                       lowerMass.getCentreX(), lowerMass.getBottom(), false);
    g.setGradientFill(lowerMassGrad);
    g.fillRoundedRectangle(lowerMass, juce::jmax(0.0f, radius - 4.0f));

    const auto sideWallW = juce::jlimit(7.0f, 14.0f, bounds.getWidth() * 0.035f);
    auto leftWall = juce::Rectangle<float>(bounds.getX() + 3.0f, bounds.getY() + 6.0f, sideWallW, bounds.getHeight() - 12.0f);
    juce::ColourGradient leftWallGrad(juce::Colours::black.withAlpha(0.12f), leftWall.getX(), leftWall.getCentreY(),
                                      juce::Colours::transparentBlack, leftWall.getRight(), leftWall.getCentreY(), false);
    g.setGradientFill(leftWallGrad);
    g.fillRoundedRectangle(leftWall, juce::jmax(0.0f, radius - 5.0f));

    auto rightWall = juce::Rectangle<float>(bounds.getRight() - sideWallW - 3.0f, bounds.getY() + 7.0f, sideWallW, bounds.getHeight() - 14.0f);
    juce::ColourGradient rightWallGrad(juce::Colours::transparentBlack, rightWall.getX(), rightWall.getCentreY(),
                                       juce::Colours::black.withAlpha(0.10f), rightWall.getRight(), rightWall.getCentreY(), false);
    g.setGradientFill(rightWallGrad);
    g.fillRoundedRectangle(rightWall, juce::jmax(0.0f, radius - 5.0f));

    auto accentTrace = bounds.reduced(18.0f, 0.0f).removeFromBottom(1.8f);
    const auto traceColour = blendDrumTint(accent, tint, 0.34f);
    juce::ColourGradient accentTraceGrad(traceColour.withAlpha(elevated ? 0.14f : 0.10f), accentTrace.getX(), accentTrace.getCentreY(),
                                         juce::Colours::transparentBlack, accentTrace.getRight(), accentTrace.getCentreY(), false);
    g.setGradientFill(accentTraceGrad);
    g.fillRoundedRectangle(accentTrace, 1.2f);

    g.setColour(juce::Colours::white.withAlpha(elevated ? 0.055f : 0.036f));
    g.drawRoundedRectangle(bounds.reduced(1.2f), juce::jmax(0.0f, radius - 0.7f), 0.7f);
    g.setColour(juce::Colours::black.withAlpha(elevated ? 0.52f : 0.46f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 1.0f);
}

void fillDrumShellCavity(juce::Graphics& g, const juce::Rectangle<float> bounds, const float radius,
                         const juce::Colour accent,
                         const juce::Colour tint = juce::Colours::transparentBlack)
{
    auto top = blendDrumTint(juce::Colour(0xff272E34), tint, 0.18f).withAlpha(0.976f);
    auto mid = blendDrumTint(juce::Colour(0xff1E2429), tint, 0.12f).withAlpha(0.988f);
    auto bottom = blendDrumTint(juce::Colour(0xff14191D), tint, 0.08f).withAlpha(0.994f);

    juce::ColourGradient cavity(top, bounds.getCentreX(), bounds.getY(),
                                bottom, bounds.getCentreX(), bounds.getBottom(), false);
    cavity.addColour(0.24, top.brighter(0.02f));
    cavity.addColour(0.58, mid);
    cavity.addColour(0.84, bottom.darker(0.06f));
    g.setGradientFill(cavity);
    g.fillRoundedRectangle(bounds, radius);

    auto lip = bounds.reduced(1.2f);
    juce::ColourGradient lipShadow(juce::Colours::black.withAlpha(0.10f), lip.getCentreX(), lip.getY(),
                                   juce::Colours::transparentBlack, lip.getCentreX(), lip.getY() + lip.getHeight() * 0.42f, false);
    g.setGradientFill(lipShadow);
    g.fillRoundedRectangle(lip, juce::jmax(0.0f, radius - 1.0f));

    auto lipHighlight = bounds.reduced(3.5f, 3.0f).removeFromTop(juce::jlimit(6.0f, 12.0f, bounds.getHeight() * 0.11f));
    juce::ColourGradient lipHighlightGrad(juce::Colours::white.withAlpha(0.044f), lipHighlight.getCentreX(), lipHighlight.getY(),
                                          juce::Colours::transparentWhite, lipHighlight.getCentreX(), lipHighlight.getBottom(), false);
    g.setGradientFill(lipHighlightGrad);
    g.fillRoundedRectangle(lipHighlight, juce::jmax(0.0f, radius - 4.0f));

    {
        juce::Graphics::ScopedSaveState scopedState(g);
        auto core = bounds.reduced(4.0f, 4.0f);
        g.reduceClipRegion(core.toNearestInt());
        addDrumBloom(g,
                     { core.getX() + core.getWidth() * 0.30f, core.getY() + core.getHeight() * 0.18f },
                     core.getWidth() * 0.22f, core.getHeight() * 0.12f,
                     juce::Colours::white.withAlpha(0.012f));
        addDrumBloom(g,
                     { core.getCentreX(), core.getBottom() - core.getHeight() * 0.08f },
                     core.getWidth() * 0.34f, core.getHeight() * 0.12f,
                     juce::Colours::black.withAlpha(0.12f));

        const auto cavityGlow = blendDrumTint(accent, tint, 0.38f);
        juce::ColourGradient floorGlow(cavityGlow.withAlpha(0.022f), core.getCentreX(), core.getY(),
                                       juce::Colours::transparentBlack, core.getCentreX(), core.getBottom(), false);
        g.setGradientFill(floorGlow);
        g.fillRoundedRectangle(core, juce::jmax(0.0f, radius - 3.0f));
    }

    g.setColour(juce::Colours::white.withAlpha(0.028f));
    g.drawRoundedRectangle(bounds.reduced(1.1f), juce::jmax(0.0f, radius - 0.8f), 0.65f);
    g.setColour(juce::Colours::black.withAlpha(0.42f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), radius, 0.9f);
}

void drawStatusChip(juce::Graphics& g, const juce::Rectangle<float> bounds,
                    const juce::String& text, const juce::Colour base, const juce::Colour outline)
{
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.fillRoundedRectangle(bounds.translated(0.0f, kShadowLight), kItemRadius);
    fillDrumPanel(g, bounds, kItemRadius, base, true);

    auto glowStrip = bounds.reduced(9.0f, 0.0f);
    glowStrip = glowStrip.removeFromBottom(2.5f);
    drawGlowStrip(g, glowStrip, outline, 1.6f, kGlowSelected);

    g.setColour(col::text);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.5f).withStyle("Bold")));
    g.drawText(text, bounds.toNearestInt(), juce::Justification::centred);
}

void drawMeterBar(juce::Graphics& g, const juce::Rectangle<float> bounds,
                  const float level, const juce::Colour colour)
{
    g.setColour(juce::Colours::black.withAlpha(0.20f));
    g.fillRoundedRectangle(bounds.translated(0.0f, kShadowLight), kButtonRadius);
    fillDrumRecess(g, bounds, kButtonRadius);

    constexpr int segments = 12;
    constexpr float gap = 2.0f;
    const float clamped = juce::jlimit(0.0f, 1.0f, level);
    const int active = static_cast<int>(std::round(clamped * static_cast<float>(segments)));
    const float segmentWidth = (bounds.getWidth() - gap * static_cast<float>(segments - 1))
        / static_cast<float>(segments);

    for (int i = 0; i < segments; ++i)
    {
        auto segment = juce::Rectangle<float>(bounds.getX() + i * (segmentWidth + gap),
                                              bounds.getY() + 1.4f,
                                              segmentWidth,
                                              bounds.getHeight() - 2.8f);
        const bool lit = i < active;
        g.setColour(lit ? colour.withAlpha(0.92f) : col::border.withAlpha(0.34f));
        g.fillRoundedRectangle(segment, 1.4f);
        if (lit)
        {
            g.setColour(colour.brighter(0.22f).withAlpha(0.28f));
            g.fillRoundedRectangle(segment.expanded(0.45f, 0.12f), 1.8f);
        }
    }
}

juce::String padModeLabel(const int padIndex)
{
    switch (mds::getPadInfo(padIndex).synthesisMode)
    {
        case mds::SynthesisMode::Tonal:      return "TONAL";
        case mds::SynthesisMode::NoiseBurst: return "BURST";
        case mds::SynthesisMode::Metallic:   return "METAL";
        case mds::SynthesisMode::Modal:      return "MODAL";
        case mds::SynthesisMode::FM:         return "FM";
    }

    return "PAD";
}

juce::String padSummaryText(const int padIndex)
{
    switch (mds::getPadInfo(padIndex).voiceModel)
    {
        case mds::PadVoiceModel::Kick:      return padIndex == 0 ? "SUB / PUNCH" : "CLICK / SUB";
        case mds::PadVoiceModel::Snare:     return "BODY / SNAP";
        case mds::PadVoiceModel::Clap:      return "NOISE BURST";
        case mds::PadVoiceModel::Hat:       return padIndex == 4 ? "TIGHT METAL" : "OPEN METAL";
        case mds::PadVoiceModel::PercWood:  return "WOOD / MODAL";
        case mds::PadVoiceModel::PercMetal: return "METAL / MODAL";
        case mds::PadVoiceModel::Tom:       return padIndex == 8 ? "LOW BODY" : "HIGH BODY";
        case mds::PadVoiceModel::Crash:     return "WASH / METAL";
        case mds::PadVoiceModel::Fx:        return "FM SWEEP";
    }

    return "DRUM PAD";
}

juce::String qualityModeLabel(const DrumSynthAudioProcessor::QualityMode mode)
{
    return mode == DrumSynthAudioProcessor::QualityMode::Studio ? "STUDIO" : "LIVE";
}

struct PadUiCtrlDef
{
    const char* label;
    const char* suffix;
    const char* shortTooltip;
    const char* noviceTooltip;
};

using PadUiProfile = std::array<PadUiCtrlDef, 11>;

const PadUiProfile& getPadUiProfile(const int padIndex)
{
    static const PadUiProfile tonalProfile = {{
        { "Level", "level", "Volume du pad", "Regle le volume de ce pad par rapport aux autres" },
        { "Tune", "tune", "Accord fin (demi-tons)", "Ajuste la hauteur en demi-tons — positif = plus aigu" },
        { "Decay", "decay", "Temps de declin", "Duree du son apres l'attaque — court pour serrer, long pour laisser sonner" },
        { "Attack", "attack", "Temps d'attaque", "Vitesse de montee du son — augmenter pour un fade-in doux" },
        { "Pitch Drop", "pitch_drop", "Chute de hauteur", "Amplitude de la chute de frequence a l'attaque — plus de punch sur les kicks" },
        { "Pitch Decay", "pitch_decay", "Vitesse de chute", "Rapidite de la descente en hauteur — lent = effet tom, rapide = impact sec" },
        { "Noise", "noise", "Niveau de bruit", "Quantite de bruit dans le son — ajoute du grain ou du souffle" },
        { "Click", "click", "Click d'attaque", "Intensite du click d'attaque — mallet dur ou frappe molle" },
        { "Drive", "drive", "Distorsion", "Saturation harmonique — chauffe le son progressivement" },
        { "Cutoff", "cutoff", "Frequence de coupure", "Frequence du filtre passe-bas — ferme pour assombrir, ouvert pour plus de brillance" },
        { "Pan", "pan", "Panoramique", "Position stereo — gauche/droite dans le champ" }
    }};

    static const PadUiProfile clapProfile = {{
        { "Level", "level", "Volume du pad", "Regle le volume du clap dans le kit" },
        { "Tune", "tune", "Accord fin (demi-tons)", "Ajuste la couleur du clap vers le grave ou l'aigu" },
        { "Decay", "decay", "Temps de declin", "Allonge ou raccourcit la queue du clap" },
        { "Attack", "attack", "Temps d'attaque", "Ramollit ou durcit le depart du clap" },
        { "Spread", "clap_spread", "Ecart des rafales", "Eloigne les micro-impacts internes pour un clap plus large" },
        { "Density", "clap_density", "Densite de clap", "Ajoute ou retire des rafales dans le clap" },
        { "Noise", "noise", "Niveau de bruit", "Dose la part de souffle et de texture dans le clap" },
        { "Click", "click", "Click d'attaque", "Renforce l'impact initial du clap" },
        { "Drive", "drive", "Distorsion", "Ajoute du grain et de la compression harmonique" },
        { "Cutoff", "cutoff", "Frequence de coupure", "Eclaircit ou assombrit le clap" },
        { "Pan", "pan", "Panoramique", "Place le clap dans le champ stereo" }
    }};

    static const PadUiProfile hatProfile = {{
        { "Level", "level", "Volume du pad", "Regle le volume du hat" },
        { "Tune", "tune", "Accord fin (demi-tons)", "Decale la hauteur apparente du hat" },
        { "Decay", "decay", "Temps de declin", "Controle la longueur generale du hat" },
        { "Attack", "attack", "Temps d'attaque", "Adoucit l'arrivee du hat" },
        { "Open", "open_amount", "Ouverture", "Ferme ou ouvre le hat pour passer de tight a airy" },
        { "Metal", "metallic_density", "Densite metallique", "Augmente le nombre et l'energie des partiels metalliques" },
        { "Noise", "noise", "Niveau de bruit", "Ajoute du souffle et du sable au hat" },
        { "Click", "click", "Click d'attaque", "Accentue le stick du hat" },
        { "Drive", "drive", "Distorsion", "Ajoute du grit et du mordant" },
        { "Cutoff", "cutoff", "Frequence de coupure", "Assombrit ou ouvre le hat" },
        { "Pan", "pan", "Panoramique", "Position stereo du hat" }
    }};

    static const PadUiProfile crashProfile = {{
        { "Level", "level", "Volume du pad", "Regle le volume du crash" },
        { "Tune", "tune", "Accord fin (demi-tons)", "Decale la hauteur percue du crash" },
        { "Decay", "decay", "Temps de declin", "Controle la longueur generale du crash" },
        { "Attack", "attack", "Temps d'attaque", "Adoucit l'arrivee du crash" },
        { "Wash", "open_amount", "Ouverture / wash", "Ouvre le wash du crash pour une queue plus ample" },
        { "Metal", "metallic_density", "Densite metallique", "Augmente la richesse et la densite des partiels metalliques" },
        { "Noise", "noise", "Niveau de bruit", "Ajoute du souffle a la cymbale" },
        { "Click", "click", "Click d'attaque", "Accentue le stick du crash" },
        { "Drive", "drive", "Distorsion", "Ajoute du grit et du caractere" },
        { "Cutoff", "cutoff", "Frequence de coupure", "Assombrit ou ouvre le crash" },
        { "Pan", "pan", "Panoramique", "Position stereo du crash" }
    }};

    static const PadUiProfile modalProfile = {{
        { "Level", "level", "Volume du pad", "Regle le volume du pad percussif" },
        { "Tune", "tune", "Accord fin (demi-tons)", "Accorde la resonance du corps" },
        { "Decay", "decay", "Temps de declin", "Allonge ou raccourcit la queue du corps" },
        { "Attack", "attack", "Temps d'attaque", "Ramollit l'impact initial" },
        { "Body Tone", "body_tone", "Couleur du corps", "Deplace le timbre entre bois mat et metal brillant" },
        { "Ring", "modal_ring", "Niveau de ring", "Ajoute ou retire de la resonance du corps" },
        { "Noise", "noise", "Niveau de bruit", "Ajoute la part de souffle ou de frottement" },
        { "Click", "click", "Click d'attaque", "Renforce le stick ou le knock initial" },
        { "Drive", "drive", "Distorsion", "Ajoute du grain au corps percussif" },
        { "Cutoff", "cutoff", "Frequence de coupure", "Eclaircit ou assombrit la resonance" },
        { "Pan", "pan", "Panoramique", "Position stereo du pad" }
    }};

    static const PadUiProfile fxProfile = {{
        { "Level", "level", "Volume du pad", "Regle le volume du pad FX" },
        { "Tune", "tune", "Accord fin (demi-tons)", "Decale la hauteur du balayage FM" },
        { "Decay", "decay", "Temps de declin", "Allonge ou raccourcit l'effet" },
        { "Attack", "attack", "Temps d'attaque", "Ramollit ou durcit le depart du FX" },
        { "FM Index", "fm_index", "Index FM", "Augmente la complexite spectrale de la modulation FM" },
        { "FM Sweep", "fm_sweep", "Balayage FM", "Controle la vitesse et l'etendue du balayage FM" },
        { "Noise", "noise", "Niveau de bruit", "Ajoute du souffle autour du FX" },
        { "Click", "click", "Click d'attaque", "Accentue l'impact initial du FX" },
        { "Drive", "drive", "Distorsion", "Ajoute du grain au moteur FM" },
        { "Cutoff", "cutoff", "Frequence de coupure", "Eclaircit ou assombrit la sortie du FX" },
        { "Pan", "pan", "Panoramique", "Position stereo du FX" }
    }};

    switch (mds::getPadInfo(padIndex).voiceModel)
    {
        case mds::PadVoiceModel::Clap: return clapProfile;
        case mds::PadVoiceModel::Hat: return hatProfile;
        case mds::PadVoiceModel::Crash: return crashProfile;
        case mds::PadVoiceModel::PercWood:
        case mds::PadVoiceModel::PercMetal: return modalProfile;
        case mds::PadVoiceModel::Fx: return fxProfile;
        default: return tonalProfile;
    }
}
}

// --- Tooltip texts (Short / Novice) ---
// Order: 11 Pad + 4 Macro + 15 FX + 1 Gain = 31
static const char* kTooltipsShort[31] = {
    // Pad 0-10
    "Volume du pad",        "Accord fin (demi-tons)", "Temps de declin",
    "Temps d'attaque",      "Chute de hauteur",       "Vitesse de chute",
    "Niveau de bruit",      "Click d'attaque",        "Distorsion",
    "Frequence de coupure", "Panoramique",
    // Macro 11-14
    "Impact + transitoire", "Corps + grave",
    "Ouverture + air",      "Saturation + grain",
    // FX 15-29
    "Seuil compresseur",    "Ratio compression",     "Attaque compresseur",
    "Relache compresseur",  "Gain de compensation",  "Mix compresseur",
    "Gain saturation",      "Mix saturation",
    "Attaque transitoire",  "Sustain transitoire",   "Mix transitoire",
    "Taille reverb",        "Amortissement reverb",  "Largeur reverb",
    "Mix reverb",
    // Gain 30
    "Volume de sortie"
};

static const char* kTooltipsNovice[31] = {
    // Pad 0-10
    "Regle le volume de ce pad par rapport aux autres",
    "Ajuste la hauteur en demi-tons — positif = plus aigu",
    "Duree du son apres l'attaque — court pour serrer, long pour laisser sonner",
    "Vitesse de montee du son — augmenter pour un fade-in doux",
    "Amplitude de la chute de frequence a l'attaque — plus de punch sur les kicks",
    "Rapidite de la descente en hauteur — lent = effet tom, rapide = impact sec",
    "Quantite de bruit dans le son — ajoutez du grain ou du souffle",
    "Intensite du click d'attaque — mallet dur ou frappe molle",
    "Saturation harmonique — chauffe le son progressivement",
    "Frequence du filtre passe-bas — ferme pour assombrir, ouvert pour plus de brillance",
    "Position stereo — gauche/droite dans le champ",
    // Macro 11-14
    "Augmente le click, reduit l'attaque, accentue la chute de hauteur — plus de frappe",
    "Allonge le decay des elements graves, abaisse la frequence du kick — plus de poids",
    "Ouvre le filtre de coupure, ajoute du bruit aux hats — plus d'air et d'espace",
    "Augmente le drive et le bruit, reduit le volume — plus de texture et de salissure",
    // FX 15-29
    "Niveau a partir duquel le compresseur agit — plus bas = plus de compression",
    "Force de la compression — 2:1 doux, 8:1 agressif",
    "Vitesse de reaction du compresseur — rapide retient les transitoires",
    "Temps de relachement — court = pompage, long = transparent",
    "Compensation de volume apres compression",
    "Dosage sec/compresse du compresseur",
    "Intensite de la saturation — pousse les harmoniques",
    "Dosage sec/sature — subtil a 10%, agressif a 80%",
    "Emphase de l'attaque — fait ressortir les transitoires du mix",
    "Niveau de sustain du transitoire — gonfle ou reduit le corps",
    "Dosage sec/traite du transient shaper",
    "Taille de la piece virtuelle — petit = tight, grand = cathedral",
    "Amortissement des aigus dans la reverb — plus = plus sombre",
    "Largeur stereo de la reverb — mono a 0, large a 100%",
    "Dosage sec/reverb — discret a 15%, ambient a 50%+",
    // Gain 30
    "Volume master final — derniere etape avant la sortie"
};

struct FxModuleToggleDef
{
    const char* label;
    const char* paramId;
};

struct FxModuleDialDef
{
    const char* label;
    const char* paramId;
};

struct FxModuleViewDef
{
    const char* label;
    const char* summary;
    const char* enableParamId;
    FxModuleToggleDef secondaryToggle;
    std::array<FxModuleDialDef, 7> dials;
    juce::Colour accent;
};

const std::array<FxModuleViewDef, 8> kFxModules = {{
    { "Saturator", "Harmonic drive and colour", "fx_saturator_en", { "", nullptr },
        {{{ "Drive", "sat_drive" }, { "Mix", "sat_mix" }, { "", nullptr }, { "", nullptr }, { "", nullptr }, { "", nullptr }, { "", nullptr } }},
        col::accent },
    { "Transient", "Attack and sustain shaping", "fx_transient_en", { "", nullptr },
        {{{ "Attack", "transient_attack" }, { "Sustain", "transient_sustain" }, { "Mix", "transient_mix" }, { "", nullptr }, { "", nullptr }, { "", nullptr }, { "", nullptr } }},
        col::accent.brighter(0.10f) },
    { "Compressor", "Dynamics control and glue", "fx_comp_en", { "", nullptr },
        {{{ "Threshold", "comp_threshold" }, { "Ratio", "comp_ratio" }, { "Attack", "comp_attack" }, { "Release", "comp_release" }, { "Makeup", "comp_makeup" }, { "Mix", "comp_mix" }, { "", nullptr } }},
        col::accent.darker(0.08f) },
    { "EQ", "Tone balancing across the kit", "fx_eq_en", { "", nullptr },
        {{{ "Low Freq", "eq_low_freq" }, { "Low Gain", "eq_low_gain" }, { "Mid Freq", "eq_mid_freq" }, { "Mid Gain", "eq_mid_gain" }, { "Mid Q", "eq_mid_q" }, { "High Freq", "eq_high_freq" }, { "High Gain", "eq_high_gain" } }},
        col::cSnare },
    { "Chorus", "Stereo width and motion", "fx_chorus_en", { "", nullptr },
        {{{ "Rate", "chorus_rate" }, { "Depth", "chorus_depth" }, { "Mix", "chorus_mix" }, { "", nullptr }, { "", nullptr }, { "", nullptr }, { "", nullptr } }},
        col::cHat },
    { "Delay", "Echo repeats locked to the groove", "fx_delay_en", { "Sync", "delay_sync" },
        {{{ "Time", "delay_time" }, { "Feedback", "delay_feedback" }, { "Mix", "delay_mix" }, { "", nullptr }, { "", nullptr }, { "", nullptr }, { "", nullptr } }},
        col::cFx },
    { "Reverb", "Space, depth, and tail", "fx_reverb_en", { "", nullptr },
        {{{ "Size", "reverb_size" }, { "Damping", "reverb_damping" }, { "Width", "reverb_width" }, { "Mix", "reverb_mix" }, { "Pre-delay", "reverb_predelay" }, { "", nullptr }, { "", nullptr } }},
        col::cTom },
    { "Limiter", "Final output ceiling protection", "fx_limiter_en", { "", nullptr },
        {{{ "Threshold", "limiter_threshold" }, { "Release", "limiter_release" }, { "", nullptr }, { "", nullptr }, { "", nullptr }, { "", nullptr }, { "", nullptr } }},
        col::textSec }
}};

bool isParamIdEqual(const juce::String& paramId, const char* candidate)
{
    return paramId.equalsIgnoreCase(candidate);
}

bool isPercentParam(const juce::String& paramId)
{
    return paramId.endsWithIgnoreCase("_level")
        || paramId.endsWithIgnoreCase("_noise")
        || paramId.endsWithIgnoreCase("_click")
        || paramId.endsWithIgnoreCase("_open_amount")
        || paramId.endsWithIgnoreCase("_metallic_density")
        || paramId.endsWithIgnoreCase("_clap_spread")
        || paramId.endsWithIgnoreCase("_clap_density")
        || paramId.endsWithIgnoreCase("_body_tone")
        || paramId.endsWithIgnoreCase("_modal_ring")
        || paramId.endsWithIgnoreCase("_fm_index")
        || paramId.endsWithIgnoreCase("_fm_sweep")
        || paramId.containsIgnoreCase("macro_")
        || paramId.containsIgnoreCase("_mix")
        || paramId.endsWithIgnoreCase("_depth")
        || isParamIdEqual(paramId, "humanize_level")
        || isParamIdEqual(paramId, "reverb_size")
        || isParamIdEqual(paramId, "reverb_damping")
        || isParamIdEqual(paramId, "reverb_width")
        || isParamIdEqual(paramId, "delay_feedback");
}

juce::String formatPercent(const double value, const int precision = 0)
{
    return juce::String(value * 100.0, precision) + "%";
}

juce::String formatSignedPercent(const double value)
{
    if (std::abs(value) < 0.005)
        return "0%";

    return juce::String(value * 100.0, std::abs(value) < 0.1 ? 1 : 0) + "%";
}

juce::String formatTimeSeconds(const double seconds)
{
    if (seconds < 1.0)
        return juce::String(std::round(seconds * 1000.0)) + " ms";

    return juce::String(seconds, seconds < 10.0 ? 2 : 1) + " s";
}

juce::String formatMilliseconds(const double ms)
{
    return juce::String(ms, ms < 100.0 ? 1 : 0) + " ms";
}

juce::String formatFrequencyHz(const double hz)
{
    if (hz >= 1000.0)
        return juce::String(hz / 1000.0, hz >= 10000.0 ? 1 : 2) + " kHz";

    return juce::String(std::round(hz)) + " Hz";
}

juce::String formatPan(const double pan)
{
    if (std::abs(pan) < 0.01)
        return "C";

    const auto amount = juce::String(std::abs(pan) * 100.0, 0);
    return (pan < 0.0 ? "L" : "R") + amount;
}

juce::String formatSemitones(const double semitones)
{
    return juce::String(semitones, std::abs(semitones) < 10.0 ? 1 : 0) + " st";
}

juce::String formatDb(const double db)
{
    return juce::String(db, std::abs(db) < 10.0 ? 1 : 0) + " dB";
}

juce::String formatRatio(const double ratio)
{
    return juce::String(ratio, ratio < 10.0 ? 1 : 0) + ":1";
}

juce::String formatMultiplier(const double value)
{
    return juce::String(value, value < 10.0 ? 2 : 1) + "x";
}

juce::String shortTooltipForParam(const juce::String& paramId)
{
    if (paramId.endsWithIgnoreCase("_level"))          return "Level";
    if (paramId.endsWithIgnoreCase("_tune"))           return "Pitch in semitones";
    if (paramId.endsWithIgnoreCase("_decay"))          return "Decay time";
    if (paramId.endsWithIgnoreCase("_attack"))         return "Attack time";
    if (paramId.endsWithIgnoreCase("_pitch_drop"))     return "Pitch envelope amount";
    if (paramId.endsWithIgnoreCase("_pitch_decay"))    return "Pitch envelope decay";
    if (paramId.endsWithIgnoreCase("_noise"))          return "Noise amount";
    if (paramId.endsWithIgnoreCase("_click"))          return "Attack click";
    if (paramId.endsWithIgnoreCase("_drive") || isParamIdEqual(paramId, "sat_drive")) return "Drive amount";
    if (paramId.endsWithIgnoreCase("_cutoff"))         return "Filter cutoff";
    if (paramId.endsWithIgnoreCase("_pan"))            return "Stereo position";
    if (paramId.endsWithIgnoreCase("_clap_spread"))    return "Clap spread";
    if (paramId.endsWithIgnoreCase("_clap_density"))   return "Clap density";
    if (paramId.endsWithIgnoreCase("_metallic_density")) return "Metallic density";
    if (paramId.endsWithIgnoreCase("_open_amount"))    return "Open amount";
    if (paramId.endsWithIgnoreCase("_body_tone"))      return "Body tone";
    if (paramId.endsWithIgnoreCase("_modal_ring"))     return "Modal ring";
    if (paramId.endsWithIgnoreCase("_fm_index"))       return "FM index";
    if (paramId.endsWithIgnoreCase("_fm_sweep"))       return "FM sweep";
    if (paramId.containsIgnoreCase("macro_punch"))     return "More impact";
    if (paramId.containsIgnoreCase("macro_weight"))    return "More body";
    if (paramId.containsIgnoreCase("macro_air"))       return "More brightness";
    if (paramId.containsIgnoreCase("macro_dirt"))      return "More grit";
    if (paramId.containsIgnoreCase("threshold"))       return "Threshold";
    if (paramId.containsIgnoreCase("ratio"))           return "Ratio";
    if (paramId.containsIgnoreCase("release"))         return "Release time";
    if (paramId.containsIgnoreCase("makeup"))          return "Makeup gain";
    if (paramId.containsIgnoreCase("transient_attack")) return "Transient attack";
    if (paramId.containsIgnoreCase("transient_sustain")) return "Transient sustain";
    if (paramId.containsIgnoreCase("reverb_predelay")) return "Pre-delay";
    if (paramId.containsIgnoreCase("reverb_"))         return "Reverb setting";
    if (paramId.containsIgnoreCase("eq_"))             return "EQ setting";
    if (paramId.containsIgnoreCase("chorus_"))         return "Chorus setting";
    if (paramId.containsIgnoreCase("delay_time"))      return "Delay time";
    if (paramId.containsIgnoreCase("delay_feedback"))  return "Delay feedback";
    if (paramId.containsIgnoreCase("delay_mix"))       return "Delay mix";
    if (paramId.containsIgnoreCase("lfo_rate"))        return "LFO rate";
    if (paramId.containsIgnoreCase("lfo_depth"))       return "LFO depth";
    if (paramId.containsIgnoreCase("humanize_timing")) return "Timing variance";
    if (paramId.containsIgnoreCase("humanize_level"))  return "Level variance";
    if (paramId.containsIgnoreCase("output_gain"))     return "Master output";
    return "Parameter";
}

juce::String noviceTooltipForParam(const juce::String& paramId)
{
    if (paramId.endsWithIgnoreCase("_level"))          return "Balances this source inside the kit.";
    if (paramId.endsWithIgnoreCase("_tune"))           return "Moves the pitch in semitones.";
    if (paramId.endsWithIgnoreCase("_decay"))          return "Shortens or extends the tail.";
    if (paramId.endsWithIgnoreCase("_attack"))         return "Softens or sharpens the initial hit.";
    if (paramId.endsWithIgnoreCase("_pitch_drop"))     return "Adds more initial pitch sweep for extra punch.";
    if (paramId.endsWithIgnoreCase("_pitch_decay"))    return "Controls how fast the pitch sweep relaxes.";
    if (paramId.endsWithIgnoreCase("_noise"))          return "Adds breath, grain, or noisy texture.";
    if (paramId.endsWithIgnoreCase("_click"))          return "Accentuates the stick or attack click.";
    if (paramId.endsWithIgnoreCase("_drive") || isParamIdEqual(paramId, "sat_drive")) return "Adds saturation and harmonic grit.";
    if (paramId.endsWithIgnoreCase("_cutoff"))         return "Darkens or opens the tone with the low-pass filter.";
    if (paramId.endsWithIgnoreCase("_pan"))            return "Places the sound left, centre, or right.";
    if (paramId.endsWithIgnoreCase("_clap_spread"))    return "Widen the clap bursts in time.";
    if (paramId.endsWithIgnoreCase("_clap_density"))   return "Increase or reduce the number of clap layers.";
    if (paramId.endsWithIgnoreCase("_metallic_density")) return "Adds more metallic partials and shimmer.";
    if (paramId.endsWithIgnoreCase("_open_amount"))    return "Opens the cymbal or hat tail.";
    if (paramId.endsWithIgnoreCase("_body_tone"))      return "Moves the resonant body between darker and brighter colours.";
    if (paramId.endsWithIgnoreCase("_modal_ring"))     return "Controls how much the body keeps ringing.";
    if (paramId.endsWithIgnoreCase("_fm_index"))       return "Adds FM complexity and brighter sidebands.";
    if (paramId.endsWithIgnoreCase("_fm_sweep"))       return "Changes the motion of the FM sweep over time.";
    if (paramId.containsIgnoreCase("macro_punch"))     return "Pushes impact and snap across the whole kit.";
    if (paramId.containsIgnoreCase("macro_weight"))    return "Adds low-end mass and body to the kit.";
    if (paramId.containsIgnoreCase("macro_air"))       return "Lifts brightness, openness, and upper detail.";
    if (paramId.containsIgnoreCase("macro_dirt"))      return "Adds grit, saturation, and rougher texture.";
    if (paramId.containsIgnoreCase("threshold"))       return "Sets the level where the processor starts acting.";
    if (paramId.containsIgnoreCase("ratio"))           return "Controls how strongly the signal is compressed.";
    if (paramId.containsIgnoreCase("makeup"))          return "Adds volume back after compression.";
    if (paramId.containsIgnoreCase("transient_attack")) return "Emphasises or softens the hit transient.";
    if (paramId.containsIgnoreCase("transient_sustain")) return "Increases or reduces the body after the hit.";
    if (paramId.containsIgnoreCase("reverb_predelay")) return "Adds a short gap before the reverb tail starts.";
    if (paramId.containsIgnoreCase("reverb_"))         return "Shapes the size and tone of the reverb space.";
    if (paramId.containsIgnoreCase("eq_"))             return "Balances frequency ranges across the whole kit.";
    if (paramId.containsIgnoreCase("chorus_"))         return "Adds stereo movement and width.";
    if (paramId.containsIgnoreCase("delay_time"))      return "Sets the time between delay repeats.";
    if (paramId.containsIgnoreCase("delay_feedback"))  return "Controls how many repeats you hear.";
    if (paramId.containsIgnoreCase("delay_mix"))       return "Blends dry signal and delay repeats.";
    if (paramId.containsIgnoreCase("lfo_rate"))        return "Sets how fast the global modulation moves.";
    if (paramId.containsIgnoreCase("lfo_depth"))       return "Sets how much modulation the LFO applies.";
    if (paramId.containsIgnoreCase("humanize_timing")) return "Adds random timing variation to hits.";
    if (paramId.containsIgnoreCase("humanize_level"))  return "Adds random velocity variation to hits.";
    if (paramId.containsIgnoreCase("output_gain"))     return "Final output gain before the plugin output.";
    return "Adjusts this parameter for the current module or pad.";
}

// =============================================================================
// MdsLookAndFeel
// =============================================================================
MdsLookAndFeel::MdsLookAndFeel()
{
    setColour(juce::Slider::thumbColourId,               col::text);
    setColour(juce::Slider::rotarySliderFillColourId,    col::accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, col::border);
    setColour(juce::Slider::textBoxTextColourId,         col::textSec);
    setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,        col::surface);
    setColour(juce::ComboBox::outlineColourId,           col::border);
    setColour(juce::ComboBox::textColourId,              col::text);
    setColour(juce::ComboBox::arrowColourId,             col::accent);
    setColour(juce::PopupMenu::backgroundColourId,       col::surface);
    setColour(juce::PopupMenu::textColourId,             col::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, col::surfHi);
    setColour(juce::PopupMenu::highlightedTextColourId,  col::text);
    setColour(juce::Label::textColourId,                 col::textSec.withAlpha(0.88f));
    setColour(juce::ToggleButton::textColourId,          col::text);
    setColour(juce::ToggleButton::tickColourId,          col::accent);
}

void MdsLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                         static_cast<float>(width), static_cast<float>(height)).reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
    auto toAngle = startAngle + sliderPos * (endAngle - startAngle);

    const bool large = radius > 40.0f;
    const float trackThickness = large ? radius * 0.12f : radius * 0.14f;
    const float trackRadius = radius - trackThickness * 0.65f;
    const float bodyRadius = radius * (large ? 0.57f : 0.54f);

    g.setColour(juce::Colours::black.withAlpha(0.30f));
    g.drawEllipse(cx - trackRadius - trackThickness * 0.70f,
                  cy - trackRadius - trackThickness * 0.70f,
                  (trackRadius + trackThickness * 0.70f) * 2.0f,
                  (trackRadius + trackThickness * 0.70f) * 2.0f,
                  trackThickness * 1.65f);

    juce::Path bgArc;
    bgArc.addCentredArc(cx, cy, trackRadius, trackRadius, 0.0f, startAngle, endAngle, true);
    g.setColour(col::bg.interpolatedWith(col::border, 0.24f));
    g.strokePath(bgArc, juce::PathStrokeType(trackThickness * 1.05f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(col::border.withAlpha(0.34f));
    g.strokePath(bgArc, juce::PathStrokeType(trackThickness * 0.70f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const int tickCount = 15;
    for (int i = 0; i <= tickCount; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(tickCount);
        const float angle = startAngle + t * (endAngle - startAngle) - juce::MathConstants<float>::halfPi;
        const float innerR = trackRadius + trackThickness * 0.72f;
        const float outerR = innerR + (i == tickCount / 2 ? 5.0f : 3.2f);
        g.setColour(i <= sliderPos * static_cast<float>(tickCount)
                        ? accent.withAlpha(0.55f)
                        : col::border.withAlpha(0.42f));
        g.drawLine(cx + innerR * std::cos(angle), cy + innerR * std::sin(angle),
                   cx + outerR * std::cos(angle), cy + outerR * std::sin(angle),
                   i == tickCount / 2 ? 1.2f : 1.0f);
    }

    if (sliderPos > 0.001f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(cx, cy, trackRadius, trackRadius, 0.0f, startAngle, toAngle, true);
        g.setColour(makeAccentGlow(accent, 0.16f));
        g.strokePath(valueArc, juce::PathStrokeType(trackThickness * 2.2f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(accent.withAlpha(0.96f));
        g.strokePath(valueArc, juce::PathStrokeType(trackThickness * 0.92f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    auto bodyBounds = juce::Rectangle<float>(cx - bodyRadius, cy - bodyRadius,
                                             bodyRadius * 2.0f, bodyRadius * 2.0f);
    g.setColour(juce::Colours::black.withAlpha(0.46f));
    g.fillEllipse(bodyBounds.translated(0.0f, 2.5f));
    g.setColour(juce::Colours::black.withAlpha(0.18f));
    g.fillEllipse(bodyBounds.translated(0.0f, 0.8f));
    drawMetalEllipse(g, bodyBounds, 1.0f);

    auto faceBounds = bodyBounds.reduced(bodyRadius * 0.14f);
    juce::ColourGradient faceGrad(col::surfHi.brighter(0.22f), faceBounds.getCentreX(), faceBounds.getY(),
                                  col::surface.darker(0.28f), faceBounds.getCentreX(), faceBounds.getBottom(), false);
    faceGrad.addColour(0.48, col::surface);
    g.setGradientFill(faceGrad);
    g.fillEllipse(faceBounds);

    g.setColour(col::border.withAlpha(0.44f));
    g.drawEllipse(bodyBounds.reduced(bodyRadius * 0.06f), 1.0f);
    auto grooveOuter = faceBounds.reduced(bodyRadius * 0.16f);
    auto grooveInner = faceBounds.reduced(bodyRadius * 0.28f);
    g.setColour(juce::Colours::white.withAlpha(0.06f));
    g.drawEllipse(grooveOuter, 0.7f);
    g.setColour(juce::Colours::black.withAlpha(0.14f));
    g.drawEllipse(grooveInner, 0.7f);

    auto highlightBounds = faceBounds.withTrimmedBottom(faceBounds.getHeight() * 0.48f);
    juce::ColourGradient highlightGrad(juce::Colours::white.withAlpha(0.14f),
                                       highlightBounds.getCentreX(), highlightBounds.getY(),
                                       juce::Colours::transparentBlack,
                                       highlightBounds.getCentreX(), highlightBounds.getBottom(), false);
    g.setGradientFill(highlightGrad);
    g.fillEllipse(highlightBounds);

    g.setColour(accent.withAlpha(0.28f));
    g.drawEllipse(faceBounds.reduced(2.0f), 1.0f);

    const float lineStart = bodyRadius * 0.12f;
    const float lineEnd = bodyRadius * 0.76f;
    const float lineAngle = toAngle - juce::MathConstants<float>::halfPi;
    const float lineWidth = large ? 2.2f : 1.9f;
    g.setColour(makeAccentGlow(accent, 0.36f));
    g.drawLine(cx + lineStart * std::cos(lineAngle), cy + lineStart * std::sin(lineAngle),
               cx + lineEnd * std::cos(lineAngle), cy + lineEnd * std::sin(lineAngle), lineWidth + 2.8f);
    g.setColour(col::text);
    g.drawLine(cx + lineStart * std::cos(lineAngle), cy + lineStart * std::sin(lineAngle),
               cx + lineEnd * std::cos(lineAngle), cy + lineEnd * std::sin(lineAngle), lineWidth);

    const float dotRadius = large ? 2.4f : 1.9f;
    const float tipX = cx + lineEnd * std::cos(lineAngle);
    const float tipY = cy + lineEnd * std::sin(lineAngle);
    g.setColour(accent.brighter(0.12f));
    g.fillEllipse(tipX - dotRadius, tipY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}

void MdsLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& btn,
    const juce::Colour& bgColour, bool hi, bool dn)
{
    auto b = btn.getLocalBounds().toFloat().reduced(0.5f);
    constexpr float cr = kButtonRadius;

    auto accent = bgColour.getSaturation() > 0.08f ? bgColour : col::accent;
    auto base = col::surface.interpolatedWith(bgColour, 0.45f);
    if (btn.getToggleState())
        base = base.interpolatedWith(accent, 0.18f);
    if (hi)
        base = base.brighter(0.04f);
    if (dn)
        base = base.darker(0.10f);

    auto drawB = dn ? b.translated(0.5f, 1.0f) : b;
    if (!dn)
    {
        g.setColour(juce::Colours::black.withAlpha(0.26f));
        g.fillRoundedRectangle(drawB.translated(0.0f, kShadowLight), cr);
    }

    fillDrumPanel(g, drawB, cr, base, hi || btn.getToggleState());

    if (hi || btn.getToggleState())
    {
        auto glowStrip = drawB.reduced(10.0f, 0.0f);
        glowStrip = glowStrip.removeFromBottom(3.0f);
        drawGlowStrip(g, glowStrip, accent, 2.0f, btn.getToggleState() ? kGlowSelected : kGlowHover);
    }
}

void MdsLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn,
    bool shouldDrawHighlighted, bool /*shouldDrawDown*/)
{
    auto fullBounds = btn.getLocalBounds().toFloat().reduced(2.0f);
    bool on = btn.getToggleState();
    const auto accent = btn.findColour(juce::ToggleButton::tickColourId);

    auto switchBounds = fullBounds;
    auto textBounds = fullBounds;
    if (btn.getButtonText().isNotEmpty() && fullBounds.getWidth() >= 72.0f)
    {
        const float switchWidth = juce::jmin(48.0f, fullBounds.getWidth() * 0.42f);
        switchBounds = fullBounds.removeFromRight(switchWidth);
        textBounds = fullBounds.reduced(4.0f, 0.0f);
    }
    else
    {
        textBounds = {};
    }

    float cr = switchBounds.getHeight() * 0.5f;

    g.setColour(juce::Colours::black.withAlpha(0.28f));
    g.fillRoundedRectangle(switchBounds.translated(0.0f, kShadowLight), cr);
    fillDrumRecess(g, switchBounds, cr);

    if (on || shouldDrawHighlighted)
    {
        auto trackTint = switchBounds.reduced(1.5f);
        juce::ColourGradient trackGlow(makeAccentGlow(accent, on ? 0.18f : 0.10f),
                                       trackTint.getCentreX(), trackTint.getY(),
                                       juce::Colours::transparentBlack,
                                       trackTint.getCentreX(), trackTint.getBottom(), false);
        g.setGradientFill(trackGlow);
        g.fillRoundedRectangle(trackTint, cr - 1.5f);
    }

    if (on)
    {
        auto glowStrip = switchBounds.reduced(switchBounds.getWidth() * 0.45f, 0.0f);
        glowStrip = glowStrip.removeFromRight(switchBounds.getWidth() * 0.45f).reduced(4.0f, 0.0f);
        glowStrip = glowStrip.removeFromBottom(3.0f);
        drawGlowStrip(g, glowStrip, accent, 2.0f, kGlowSelected);
    }

    float knobD = switchBounds.getHeight() - 4.0f;
    float knobX = on ? (switchBounds.getRight() - knobD - 2.0f) : (switchBounds.getX() + 2.0f);
    float knobY = switchBounds.getY() + 2.0f;

    g.setColour(juce::Colours::black.withAlpha(0.18f));
    g.fillEllipse(knobX, knobY + 1.0f, knobD, knobD);

    auto knobBounds = juce::Rectangle<float>(knobX, knobY, knobD, knobD);
    drawMetalEllipse(g, knobBounds, 0.8f);

    auto inner = knobBounds.reduced(2.0f);
    juce::ColourGradient knobFace(juce::Colour(0xffECEFF4), inner.getCentreX(), inner.getY(),
                                  juce::Colour(0xff9EA4AF), inner.getCentreX(), inner.getBottom(), false);
    knobFace.addColour(0.45, juce::Colour(0xffD4D9E1));
    g.setGradientFill(knobFace);
    g.fillEllipse(inner);

    g.setColour(on ? accent.withAlpha(0.32f) : col::border.withAlpha(0.38f));
    g.drawEllipse(inner.reduced(0.5f), 0.8f);

    if (!textBounds.isEmpty())
    {
        g.setColour(on ? col::text : col::textSec);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.5f).withStyle("Bold")));
        g.drawText(btn.getButtonText(),
                   textBounds.toNearestInt().reduced(0, 1),
                   juce::Justification::centredLeft);
    }
}

void MdsLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
    bool /*isDown*/, int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f,
        static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    constexpr float cr = kButtonRadius;

    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.fillRoundedRectangle(bounds.translated(0.0f, kShadowLight), cr);

    const bool active = box.hasKeyboardFocus(true) || box.isMouseOver(true);
    auto base = box.findColour(juce::ComboBox::backgroundColourId);
    if (active)
        base = base.interpolatedWith(col::accent, 0.10f);
    fillDrumPanel(g, bounds, cr, base, active);

    if (active)
    {
        auto glowStrip = bounds.reduced(10.0f, 0.0f);
        glowStrip = glowStrip.removeFromBottom(3.0f);
        drawGlowStrip(g, glowStrip, box.findColour(juce::ComboBox::arrowColourId), 2.0f, kGlowHover);
    }

    float cx = static_cast<float>(width) - 15.0f;
    float cy = static_cast<float>(height) * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath(cx - 4.0f, cy - 2.0f);
    chevron.lineTo(cx, cy + 2.5f);
    chevron.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.strokePath(chevron, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void MdsLookAndFeel::drawProgressBar(juce::Graphics& g, juce::ProgressBar& bar,
    int width, int height, double progress, const juce::String& /*textToShow*/)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f,
        static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    drawMeterBar(g, bounds,
                 static_cast<float>(juce::jlimit(0.0, 1.0, progress)),
                 bar.findColour(juce::ProgressBar::foregroundColourId));
}

// =============================================================================
// PadComponent
// =============================================================================
void PadComponent::configure(int index, const juce::String& padName, const juce::String& padSummary, juce::Colour catCol)
{
    idx = index;
    name = padName;
    summary = padSummary;
    cat = catCol;
}

void PadComponent::setSelected(bool s)
{
    if (sel != s) { sel = s; repaint(); }
}

void PadComponent::flash()
{
    flashA = 1.0f;
    repaint();
}

void PadComponent::tick()
{
    if (flashA > 0.008f)
    {
        flashA *= 0.84f;
        if (flashA < 0.008f) flashA = 0.0f;
        repaint();
    }
}

void PadComponent::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    constexpr float cr = kPanelRadius;

    if (!pressed)
    {
        g.setColour(juce::Colours::black.withAlpha(0.28f));
        g.fillRoundedRectangle(b.translated(0.0f, kShadowDeep), cr);
    }

    auto drawB = pressed ? b.translated(0.5f, 1.5f) : b;
    auto base = sel ? col::surface.interpolatedWith(cat, 0.22f) : col::surface;
    if (hover && !pressed)
        base = base.brighter(0.04f);
    fillDrumPanel(g, drawB, cr, base, sel || hover);

    auto core = drawB.reduced(5.5f);
    fillDrumRecess(g, core, cr - 2.0f);

    if (sel)
    {
        juce::ColourGradient selGlow(cat.withAlpha(0.14f), drawB.getCentreX(), drawB.getCentreY(),
                                     juce::Colours::transparentBlack, drawB.getCentreX(), drawB.getY(), true);
        g.setGradientFill(selGlow);
        g.fillRoundedRectangle(drawB.reduced(4.0f), cr - 2.0f);
    }

    auto barRect = drawB.reduced(10.0f, 0.0f);
    barRect = barRect.removeFromTop(3.5f);
    drawGlowStrip(g, barRect, cat, 2.0f, sel ? kGlowSelected : (hover ? kGlowHover : kGlowNormal));

    auto ledOuter = juce::Rectangle<float>(drawB.getRight() - 17.0f, drawB.getY() + 7.0f, 8.0f, 8.0f);
    drawMetalEllipse(g, ledOuter.expanded(0.6f), 0.6f);
    g.setColour((sel || hover ? cat.brighter(0.18f) : col::border).withAlpha(sel || hover ? 0.96f : 0.42f));
    g.fillEllipse(ledOuter);

    g.setColour(sel ? col::textDim : col::textDim.withAlpha(0.40f));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
    g.drawText(juce::String(idx + 1),
               drawB.reduced(8.0f, 10.0f), juce::Justification::topLeft);

    auto summaryBadge = drawB.reduced(8.0f, 0.0f);
    summaryBadge = summaryBadge.removeFromBottom(15.0f);
    fillDrumPanel(g, summaryBadge, 4.5f,
                  col::bg.interpolatedWith(cat, sel ? 0.20f : 0.12f),
                  sel || hover);

    g.setColour(sel ? col::text : (hover ? col::textSec : col::textDim));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    g.drawText(name, drawB.reduced(10.0f, 18.0f), juce::Justification::centred);

    g.setColour(sel ? col::textSec : col::textDim.withAlpha(0.78f));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.2f).withStyle("Bold")));
    g.drawText(summary, summaryBadge.toNearestInt().reduced(4, 0), juce::Justification::centred);

    if (flashA > 0.008f)
    {
        g.setColour(cat.withAlpha(flashA * 0.30f));
        g.fillRoundedRectangle(drawB, cr);
        g.setColour(cat.withAlpha(flashA * 0.15f));
        g.drawRoundedRectangle(drawB.reduced(-2.0f), cr + 2.0f, 3.0f);
    }
}

void PadComponent::mouseDown(const juce::MouseEvent&)
{
    pressed = true;
    repaint();
    if (onClicked) onClicked(idx);
}

void PadComponent::mouseUp(const juce::MouseEvent&)
{
    pressed = false;
    repaint();
}

void PadComponent::mouseEnter(const juce::MouseEvent&) { hover = true;  repaint(); }
void PadComponent::mouseExit(const juce::MouseEvent&)  { hover = false; pressed = false; repaint(); }

// =============================================================================
// FxRackItem
// =============================================================================
void FxRackItem::configure(int index, const juce::String& moduleName, const juce::String& moduleSummary, juce::Colour moduleAccent)
{
    idx = index;
    name = moduleName;
    summary = moduleSummary;
    accent = moduleAccent;
}

void FxRackItem::setSelected(bool s)
{
    if (sel != s)
    {
        sel = s;
        repaint();
    }
}

void FxRackItem::setEnabledState(bool s)
{
    if (enabledState != s)
    {
        enabledState = s;
        repaint();
    }
}

void FxRackItem::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(0.5f);
    constexpr float radius = kItemRadius;

    g.setColour(juce::Colours::black.withAlpha(sel ? 0.24f : 0.18f));
    g.fillRoundedRectangle(bounds.translated(0.0f, kShadowLight), radius);

    auto base = col::surface;
    if (sel)
        base = base.interpolatedWith(accent, 0.16f);
    else if (hover)
        base = base.brighter(0.04f);

    fillDrumPanel(g, bounds, radius, base, sel || hover);

    auto led = juce::Rectangle<float>(bounds.getX() + 10.0f, bounds.getCentreY() - 4.0f, 8.0f, 8.0f);
    drawMetalEllipse(g, led.expanded(0.6f), 0.7f);
    g.setColour((enabledState ? accent : col::border.withAlpha(0.55f)).withAlpha(enabledState ? 0.95f : 0.55f));
    g.fillEllipse(led);

    auto textArea = bounds.toNearestInt().reduced(24, 7);
    g.setColour(col::text);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    g.drawText(name, textArea.removeFromTop(14), juce::Justification::centredLeft);

    g.setColour(sel ? col::textSec : col::textDim);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
    g.drawText(summary, textArea, juce::Justification::centredLeft);

    if (sel)
    {
        auto glowStrip = bounds.reduced(10.0f, 0.0f);
        glowStrip = glowStrip.removeFromBottom(2.4f);
        drawGlowStrip(g, glowStrip, accent, 1.6f, kGlowSelected);
    }
}

void FxRackItem::mouseDown(const juce::MouseEvent&)
{
    if (onClicked)
        onClicked(idx);
}

void FxRackItem::mouseEnter(const juce::MouseEvent&)
{
    hover = true;
    repaint();
}

void FxRackItem::mouseExit(const juce::MouseEvent&)
{
    hover = false;
    repaint();
}

// =============================================================================
// Helpers
// =============================================================================
juce::String DrumSynthAudioProcessorEditor::formatValueForParam(const juce::String& paramId, double value)
{
    if (paramId.endsWithIgnoreCase("_tune") || paramId.endsWithIgnoreCase("_pitch_drop"))
        return formatSemitones(value);

    if (paramId.endsWithIgnoreCase("_decay") || paramId.endsWithIgnoreCase("_pitch_decay"))
        return formatTimeSeconds(value);

    if (paramId.endsWithIgnoreCase("_attack") && paramId.startsWithIgnoreCase("pad_"))
        return formatTimeSeconds(value);

    if (paramId.endsWithIgnoreCase("_cutoff")
        || paramId.containsIgnoreCase("_freq")
        || paramId.containsIgnoreCase("lfo_rate"))
        return formatFrequencyHz(value);

    if (paramId.endsWithIgnoreCase("_pan"))
        return formatPan(value);

    if (paramId.containsIgnoreCase("threshold")
        || paramId.containsIgnoreCase("makeup")
        || paramId.containsIgnoreCase("_gain")
        || isParamIdEqual(paramId, "output_gain"))
        return formatDb(value);

    if (paramId.containsIgnoreCase("ratio"))
        return formatRatio(value);

    if (paramId.containsIgnoreCase("comp_attack")
        || paramId.containsIgnoreCase("comp_release")
        || paramId.containsIgnoreCase("limiter_release")
        || paramId.containsIgnoreCase("delay_time")
        || paramId.containsIgnoreCase("reverb_predelay")
        || paramId.containsIgnoreCase("humanize_timing"))
        return formatMilliseconds(value);

    if (paramId.containsIgnoreCase("sat_drive") || paramId.endsWithIgnoreCase("_drive"))
        return formatMultiplier(value);

    if (paramId.containsIgnoreCase("transient_attack") || paramId.containsIgnoreCase("transient_sustain"))
        return formatSignedPercent(value);

    if (paramId.containsIgnoreCase("eq_mid_q"))
        return juce::String(value, value < 10.0 ? 2 : 1);

    if (isPercentParam(paramId))
        return formatPercent(value, value < 0.1 ? 1 : 0);

    return juce::String(value, 2);
}

juce::Colour DrumSynthAudioProcessorEditor::padCatColour(int i)
{
    switch (i)
    {
        case 0: case 1:   return col::cKick;
        case 2: case 3:   return col::cSnare;
        case 4: case 5:   return col::cHat;
        case 6: case 7:   return col::cPerc;
        case 8: case 9:   return col::cTom;
        case 10: case 11: return col::cFx;
        default:           return col::accent;
    }
}

const char* DrumSynthAudioProcessorEditor::padCatName(int i)
{
    switch (i)
    {
        case 0: case 1:   return "KICK";
        case 2: case 3:   return "SNARE";
        case 4: case 5:   return "HI-HAT";
        case 6: case 7:   return "PERC";
        case 8: case 9:   return "TOM";
        case 10: case 11: return "FX";
        default:           return "PAD";
    }
}

void DrumSynthAudioProcessorEditor::setupDial(juce::Slider& s, juce::Colour fill) const
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 16);
    s.setColour(juce::Slider::rotarySliderFillColourId, fill);
    s.setTextBoxIsEditable(false);
    s.setScrollWheelEnabled(false);
    s.setSliderSnapsToMousePosition(false);
    s.setMouseDragSensitivity(240);
    s.setDoubleClickReturnValue(false, 0.0);
}

void DrumSynthAudioProcessorEditor::setupSmallDial(juce::Slider& s, juce::Colour fill) const
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 14);
    s.setColour(juce::Slider::rotarySliderFillColourId, fill);
    s.setTextBoxIsEditable(false);
    s.setScrollWheelEnabled(false);
    s.setSliderSnapsToMousePosition(false);
    s.setMouseDragSensitivity(220);
    s.setDoubleClickReturnValue(false, 0.0);
}

void DrumSynthAudioProcessorEditor::updateSliderTextFormat(juce::Slider& s, const juce::String& paramId) const
{
    s.textFromValueFunction = [paramId](double value)
    {
        return DrumSynthAudioProcessorEditor::formatValueForParam(paramId, value);
    };
}

int DrumSynthAudioProcessorEditor::selectedPadFromParam() const
{
    if (auto* raw = proc.getAPVTS().getRawParameterValue("selected_pad"))
        return juce::jlimit(0, mds::kNumPads - 1,
                            static_cast<int>(std::round(raw->load())));
    return 0;
}

// =============================================================================
// Constructor
// =============================================================================
DrumSynthAudioProcessorEditor::DrumSynthAudioProcessorEditor(
    DrumSynthAudioProcessor& processor)
    : AudioProcessorEditor(&processor), proc(processor)
{
    setOpaque(true);
    setLookAndFeel(&lnf);

    // Generate procedural noise texture (anthracite grain)
    bgTexture = juce::Image(juce::Image::ARGB, 256, 256, true);
    {
        juce::Image::BitmapData bits(bgTexture, juce::Image::BitmapData::writeOnly);
        juce::Random rng(42);
        for (int ny = 0; ny < 256; ++ny)
            for (int nx = 0; nx < 256; ++nx)
            {
                auto lum = (juce::uint8)(rng.nextFloat() * 60.0f + 10.0f);
                auto a = (juce::uint8)(rng.nextFloat() * 18.0f);
                bits.setPixelColour(nx, ny, juce::Colour(lum, lum, lum, a));
            }
    }

    // ---- Icons ----
    iconGain   = juce::ImageFileFormat::loadFrom(BinaryData::_006_gain_volume_png,      BinaryData::_006_gain_volume_pngSize);
    iconOutput = juce::ImageFileFormat::loadFrom(BinaryData::_012_output_routing_png,   BinaryData::_012_output_routing_pngSize);
    iconSingle = juce::ImageFileFormat::loadFrom(BinaryData::_020_single_note_png,      BinaryData::_020_single_note_pngSize);

    categoryIcons[0] = juce::ImageFileFormat::loadFrom(BinaryData::_014_categorie_kick_png,     BinaryData::_014_categorie_kick_pngSize);
    categoryIcons[1] = juce::ImageFileFormat::loadFrom(BinaryData::_015_categorie_snare_png,    BinaryData::_015_categorie_snare_pngSize);
    categoryIcons[2] = juce::ImageFileFormat::loadFrom(BinaryData::_016_categorie_hi_hat_png,   BinaryData::_016_categorie_hi_hat_pngSize);
    categoryIcons[3] = categoryIcons[2];
    categoryIcons[4] = juce::ImageFileFormat::loadFrom(BinaryData::_018_categorie_tom_png,      BinaryData::_018_categorie_tom_pngSize);
    categoryIcons[5] = juce::ImageFileFormat::loadFrom(BinaryData::_019_categorie_fx_png,       BinaryData::_019_categorie_fx_pngSize);

    padCtrlIcons[0]  = juce::ImageFileFormat::loadFrom(BinaryData::_001_level_00001__png,        BinaryData::_001_level_00001__pngSize);
    padCtrlIcons[1]  = juce::ImageFileFormat::loadFrom(BinaryData::_002_tune_00001__png,         BinaryData::_002_tune_00001__pngSize);
    padCtrlIcons[2]  = juce::ImageFileFormat::loadFrom(BinaryData::_004_decay_00001__png,        BinaryData::_004_decay_00001__pngSize);
    padCtrlIcons[3]  = juce::ImageFileFormat::loadFrom(BinaryData::_003_attack_00001__png,       BinaryData::_003_attack_00001__pngSize);
    padCtrlIcons[4]  = juce::ImageFileFormat::loadFrom(BinaryData::_012_pitch_drop_00001__png,   BinaryData::_012_pitch_drop_00001__pngSize);
    padCtrlIcons[5]  = juce::ImageFileFormat::loadFrom(BinaryData::_009_brightness_00001__png,   BinaryData::_009_brightness_00001__pngSize);
    padCtrlIcons[6]  = juce::ImageFileFormat::loadFrom(BinaryData::_014_noise_00001__png,        BinaryData::_014_noise_00001__pngSize);
    padCtrlIcons[7]  = juce::ImageFileFormat::loadFrom(BinaryData::_013_click_00001__png,        BinaryData::_013_click_00001__pngSize);
    padCtrlIcons[8]  = juce::ImageFileFormat::loadFrom(BinaryData::_037_drive_saturation_00001__png, BinaryData::_037_drive_saturation_00001__pngSize);
    padCtrlIcons[9]  = juce::ImageFileFormat::loadFrom(BinaryData::_007_cutoff_00001__png,       BinaryData::_007_cutoff_00001__pngSize);
    padCtrlIcons[10] = juce::ImageFileFormat::loadFrom(BinaryData::_008_pan_00001__png,          BinaryData::_008_pan_00001__pngSize);

    macroIcons[0] = juce::ImageFileFormat::loadFrom(BinaryData::_024_macro_punch_png,        BinaryData::_024_macro_punch_pngSize);
    macroIcons[1] = juce::ImageFileFormat::loadFrom(BinaryData::_025_macro_weight_png,       BinaryData::_025_macro_weight_pngSize);
    macroIcons[2] = juce::ImageFileFormat::loadFrom(BinaryData::_026_macro_air_png,          BinaryData::_026_macro_air_pngSize);
    macroIcons[3] = juce::ImageFileFormat::loadFrom(BinaryData::_037_drive_saturation_00001__png, BinaryData::_037_drive_saturation_00001__pngSize);

    fxSectionIconComp = juce::ImageFileFormat::loadFrom(BinaryData::_021_compressor_png,               BinaryData::_021_compressor_pngSize);
    fxSectionIconSat  = juce::ImageFileFormat::loadFrom(BinaryData::_022_saturator_png,                BinaryData::_022_saturator_pngSize);
    fxSectionIconTrans= juce::ImageFileFormat::loadFrom(BinaryData::_044_envelope_amount_00001__png,   BinaryData::_044_envelope_amount_00001__pngSize);
    fxSectionIconReverb = categoryIcons[5];

    fxDialIcons[0]  = juce::ImageFileFormat::loadFrom(BinaryData::_024_threshold_00001__png,          BinaryData::_024_threshold_00001__pngSize);
    fxDialIcons[1]  = juce::ImageFileFormat::loadFrom(BinaryData::_025_ratio_00001__png,              BinaryData::_025_ratio_00001__pngSize);
    fxDialIcons[2]  = juce::ImageFileFormat::loadFrom(BinaryData::_003_attack_00001__png,             BinaryData::_003_attack_00001__pngSize);
    fxDialIcons[3]  = juce::ImageFileFormat::loadFrom(BinaryData::_006_release_00001__png,            BinaryData::_006_release_00001__pngSize);
    fxDialIcons[4]  = juce::ImageFileFormat::loadFrom(BinaryData::_026_makeup_gain_00001__png,        BinaryData::_026_makeup_gain_00001__pngSize);
    fxDialIcons[5]  = juce::ImageFileFormat::loadFrom(BinaryData::_038_blend_mix_00001__png,          BinaryData::_038_blend_mix_00001__pngSize);
    fxDialIcons[6]  = juce::ImageFileFormat::loadFrom(BinaryData::_037_drive_saturation_00001__png,   BinaryData::_037_drive_saturation_00001__pngSize);
    fxDialIcons[7]  = fxDialIcons[5];
    fxDialIcons[8]  = fxDialIcons[2];
    fxDialIcons[9]  = juce::ImageFileFormat::loadFrom(BinaryData::_005_sustain_00001__png,            BinaryData::_005_sustain_00001__pngSize);
    fxDialIcons[10] = fxDialIcons[5];
    fxDialIcons[11] = padCtrlIcons[2];
    fxDialIcons[12] = padCtrlIcons[9];
    fxDialIcons[13] = padCtrlIcons[10];
    fxDialIcons[14] = fxDialIcons[5];

    addAndMakeVisible(presetBox);
    refreshPresetList();
    presetBox.onChange = [this]
    {
        if (presetUiRefreshing)
            return;

        const auto idx = presetBox.getSelectedItemIndex();
        if (idx < 0 || idx >= visiblePresetEntries.size())
            return;

        const auto& entry = visiblePresetEntries.getReference(idx);
        if (entry.isFactory)
            proc.applyFactoryPreset(entry.factoryIndex);
        else if (entry.presetFile.existsAsFile())
            proc.loadUserPreset(entry.presetFile);

        refreshPresetMetadata();
        refreshMidiLearnPanel();
    };

    addAndMakeVisible(presetSourceFilterBox);
    presetSourceFilterBox.addItem("All Sources", 1);
    presetSourceFilterBox.addItem("Factory", 2);
    presetSourceFilterBox.addItem("User", 3);
    presetSourceFilterBox.setSelectedId(1, juce::dontSendNotification);
    presetSourceFilterBox.onChange = [this] { refreshPresetList(); };

    addAndMakeVisible(presetFamilyFilterBox);
    presetFamilyFilterBox.onChange = [this] { refreshPresetList(); };

    addAndMakeVisible(presetRoleFilterBox);
    presetRoleFilterBox.onChange = [this] { refreshPresetList(); };

    addAndMakeVisible(presetTagFilterBox);
    presetTagFilterBox.onChange = [this] { refreshPresetList(); };

    addAndMakeVisible(presetMetaLabel);
    presetMetaLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    presetMetaLabel.setColour(juce::Label::textColourId, col::textDim);
    presetMetaLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(prevPresetBtn);
    prevPresetBtn.setButtonText("<");
    prevPresetBtn.onClick = [this] { navigatePreset(-1); };

    addAndMakeVisible(nextPresetBtn);
    nextPresetBtn.setButtonText(">");
    nextPresetBtn.onClick = [this] { navigatePreset(1); };

    addAndMakeVisible(saveBtn);
    saveBtn.setButtonText("Save");
    saveBtn.onClick = [this] { saveCurrentPreset(); };

    addAndMakeVisible(saveAsBtn);
    saveAsBtn.setButtonText("Save As");
    saveAsBtn.onClick = [this] { showSaveAsDialog(); };

    addAndMakeVisible(deleteBtn);
    deleteBtn.setButtonText("Delete");
    deleteBtn.onClick = [this] { deleteCurrentUserPreset(); };

    addAndMakeVisible(importBtn);
    importBtn.setButtonText("Import");
    importBtn.onClick = [this] { importPresetsFromZip(); };

    addAndMakeVisible(tooltipModeBtn);
    tooltipModeBtn.setButtonText("Tips");
    tooltipModeBtn.setColour(juce::TextButton::buttonColourId, col::surface);
    tooltipModeBtn.setColour(juce::TextButton::textColourOffId, col::textDim);
    tooltipModeBtn.onClick = [this] { cycleTooltipMode(); };

    addAndMakeVisible(midiCCPageLabel);
    midiCCPageLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    midiCCPageLabel.setColour(juce::Label::textColourId, col::accent);
    midiCCPageLabel.setJustificationType(juce::Justification::centredLeft);
    midiCCPageLabel.setText("CC Page: Macros", juce::dontSendNotification);

    addAndMakeVisible(qualityModeBox);
    qualityModeBox.addItem("Live", 1);
    qualityModeBox.addItem("Studio", 2);
    qualityModeAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "quality_mode", qualityModeBox);

    addAndMakeVisible(velocityCurveBox);
    for (auto& name : { "Linear", "Soft", "Softer", "Hard", "Harder", "Fixed", "Touch" })
        velocityCurveBox.addItem(name, velocityCurveBox.getNumItems() + 1);
    velocityCurveAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "velocity_curve", velocityCurveBox);

    addAndMakeVisible(lfoLabel);
    lfoLabel.setText("LFO", juce::dontSendNotification);
    lfoLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    lfoLabel.setColour(juce::Label::textColourId, col::accent);

    addAndMakeVisible(lfoRateDial);
    setupSmallDial(lfoRateDial, col::accent);
    updateSliderTextFormat(lfoRateDial, "lfo_rate");
    lfoRateAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "lfo_rate", lfoRateDial);

    addAndMakeVisible(lfoDepthDial);
    setupSmallDial(lfoDepthDial, col::accent);
    updateSliderTextFormat(lfoDepthDial, "lfo_depth");
    lfoDepthAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "lfo_depth", lfoDepthDial);

    addAndMakeVisible(lfoWaveBox);
    for (auto& name : { "Sine", "Triangle", "Saw", "Square" })
        lfoWaveBox.addItem(name, lfoWaveBox.getNumItems() + 1);
    lfoWaveAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "lfo_wave", lfoWaveBox);

    addAndMakeVisible(humanizeLabel);
    humanizeLabel.setText("Humanize", juce::dontSendNotification);
    humanizeLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    humanizeLabel.setColour(juce::Label::textColourId, col::accent.darker(0.12f));

    addAndMakeVisible(humanizeTimingDial);
    setupSmallDial(humanizeTimingDial, col::accent.darker(0.12f));
    updateSliderTextFormat(humanizeTimingDial, "humanize_timing");
    humanizeTimingAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "humanize_timing", humanizeTimingDial);

    addAndMakeVisible(humanizeLevelDial);
    setupSmallDial(humanizeLevelDial, col::accent.darker(0.12f));
    updateSliderTextFormat(humanizeLevelDial, "humanize_level");
    humanizeLevelAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "humanize_level", humanizeLevelDial);

    addAndMakeVisible(gainDial);
    setupSmallDial(gainDial, col::accent);
    updateSliderTextFormat(gainDial, "output_gain");
    gainAtt = std::make_unique<SliderAttach>(proc.getAPVTS(), "output_gain", gainDial);

    addAndMakeVisible(singleNoteBtn);
    singleNoteBtn.setButtonText("Single Note");
    singleNoteBtn.setClickingTogglesState(true);
    singleAtt = std::make_unique<ButtonAttach>(proc.getAPVTS(), "single_note_mode", singleNoteBtn);

    addAndMakeVisible(utilityDrawerBtn);
    utilityDrawerBtn.setButtonText("Utility");
    utilityDrawerBtn.onClick = [this] { setUtilityDrawerOpen(!utilityDrawerOpen); };

    padSelector.setVisible(false);
    addChildComponent(padSelector);
    for (int i = 0; i < mds::kNumPads; ++i)
        padSelector.addItem("Pad " + juce::String(i + 1) + " - " + juce::String(mds::makePadName(i)), i + 1);
    selPadAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "selected_pad", padSelector);
    padSelector.onChange = [this]
    {
        rebuildPadAttachments();
        refreshPadSelection();
    };

    for (int i = 0; i < mds::kNumPads; ++i)
    {
        auto& pad = pads[static_cast<std::size_t>(i)];
        pad.configure(i, juce::String(mds::makePadName(i)), padSummaryText(i), padCatColour(i));
        pad.onClicked = [this](int idx)
        {
            padSelector.setSelectedId(idx + 1);
            proc.queuePadTrigger(idx, 1.0f);
            pads[static_cast<std::size_t>(idx)].flash();
        };
        addAndMakeVisible(pad);
    }

    addAndMakeVisible(instrTitle);
    instrTitle.setJustificationType(juce::Justification::centredLeft);
    instrTitle.setFont(juce::Font(juce::FontOptions{}.withHeight(16.0f).withStyle("Bold")));
    instrTitle.setColour(juce::Label::textColourId, col::text);

    addAndMakeVisible(padSummaryLabel);
    padSummaryLabel.setJustificationType(juce::Justification::centredLeft);
    padSummaryLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.5f)));
    padSummaryLabel.setColour(juce::Label::textColourId, col::textDim);

    addAndMakeVisible(padPresetBox);
    padPresetBox.setTextWhenNothingSelected("Factory Pad Preset");
    padPresetBox.onChange = [this]
    {
        const int idx = padPresetBox.getSelectedItemIndex();
        if (idx > 0)
            proc.applyFactoryPadPreset(selectedPadFromParam(), idx - 1);
    };

    addAndMakeVisible(outputBox);
    for (int i = 0; i <= DrumSynthAudioProcessor::kNumAuxOutputs; ++i)
        outputBox.addItem(i == 0 ? "Master" : "Out " + juce::String(i), i + 1);

    for (int i = 0; i < kPadCtrlN; ++i)
    {
        auto si = static_cast<std::size_t>(i);
        setupDial(padDials[si], col::accent);
        addAndMakeVisible(padDials[si]);

        padDlLabels[si].setText(kPadCtrls[si].label, juce::dontSendNotification);
        padDlLabels[si].setJustificationType(juce::Justification::centred);
        padDlLabels[si].setFont(juce::Font(juce::FontOptions{}.withHeight(10.4f)));
        padDlLabels[si].setColour(juce::Label::textColourId, col::textSec.withAlpha(0.84f));
        addAndMakeVisible(padDlLabels[si]);
    }

    for (int i = 0; i < kMacroN; ++i)
    {
        auto si = static_cast<std::size_t>(i);
        macroAtt[si] = std::make_unique<SliderAttach>(proc.getAPVTS(), kMacroCtrls[si].paramId, macroDials[si]);
        setupDial(macroDials[si], col::accent);
        updateSliderTextFormat(macroDials[si], kMacroCtrls[si].paramId);
        addAndMakeVisible(macroDials[si]);

        macroLbls[si].setText(kMacroCtrls[si].label, juce::dontSendNotification);
        macroLbls[si].setJustificationType(juce::Justification::centred);
        macroLbls[si].setFont(juce::Font(juce::FontOptions{}.withHeight(10.4f)));
        macroLbls[si].setColour(juce::Label::textColourId, col::textSec.withAlpha(0.84f));
        addAndMakeVisible(macroLbls[si]);
    }

    for (int i = 0; i < kFxRackCount; ++i)
    {
        auto& item = fxRackItems[static_cast<std::size_t>(i)];
        item.configure(i,
                       kFxModules[static_cast<std::size_t>(i)].label,
                       kFxModules[static_cast<std::size_t>(i)].summary,
                       kFxModules[static_cast<std::size_t>(i)].accent);
        item.onClicked = [this](int idx)
        {
            if (selectedFxModule == idx)
                return;

            selectedFxModule = idx;
            rebuildAdvancedFxPage();
            refreshFxRackState();
            repaint();
        };
        addAndMakeVisible(item);
    }

    for (int i = 0; i < kFxDetailPool; ++i)
    {
        auto si = static_cast<std::size_t>(i);
        setupSmallDial(fxDials[si], col::accent);
        addAndMakeVisible(fxDials[si]);
        fxDials[si].setVisible(false);
        fxLbls[si].setJustificationType(juce::Justification::centred);
        fxLbls[si].setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
        fxLbls[si].setColour(juce::Label::textColourId, col::textSec.withAlpha(0.84f));
        addAndMakeVisible(fxLbls[si]);
        fxLbls[si].setVisible(false);
    }

    addAndMakeVisible(advancedFxPageBox);
    advancedFxPageBox.setVisible(false);
    advancedFxPageBox.setTextWhenNothingSelected("Note Division");

    addAndMakeVisible(advancedToggleA);
    addAndMakeVisible(advancedToggleB);
    advancedToggleA.setClickingTogglesState(true);
    advancedToggleB.setClickingTogglesState(true);

    addAndMakeVisible(engineStatusLabel);
    engineStatusLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f)));
    engineStatusLabel.setColour(juce::Label::textColourId, col::textSec);
    engineStatusLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(mainMeterLabel);
    mainMeterLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withStyle("Bold")));
    mainMeterLabel.setColour(juce::Label::textColourId, col::textDim);
    mainMeterLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(mainMeterBar);
    mainMeterBar.setColour(juce::ProgressBar::foregroundColourId, col::accent);
    mainMeterBar.setColour(juce::ProgressBar::backgroundColourId, col::border.withAlpha(0.7f));

    addAndMakeVisible(auxMeterLabel);
    auxMeterLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f).withStyle("Bold")));
    auxMeterLabel.setColour(juce::Label::textColourId, col::textDim);
    auxMeterLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(auxMeterBar);
    auxMeterBar.setColour(juce::ProgressBar::foregroundColourId, col::cFx);
    auxMeterBar.setColour(juce::ProgressBar::backgroundColourId, col::border.withAlpha(0.7f));

    addAndMakeVisible(clipResetBtn);
    clipResetBtn.setButtonText("Safe");
    clipResetBtn.onClick = [this] { proc.clearClipLatch(); refreshStatusPanel(); };

    addAndMakeVisible(midiLearnTargetBox);
    addAndMakeVisible(midiLearnMappingsBox);

    addAndMakeVisible(midiLearnArmBtn);
    midiLearnArmBtn.setButtonText("Learn");
    midiLearnArmBtn.onClick = [this]
    {
        if (proc.isMidiLearning())
        {
            proc.cancelMidiLearn();
        }
        else
        {
            const int index = midiLearnTargetBox.getSelectedItemIndex();
            if (index >= 0 && index < midiLearnTargetIds.size())
                proc.startMidiLearn(midiLearnTargetIds[index]);
        }

        refreshMidiLearnPanel();
    };

    addAndMakeVisible(midiLearnClearBtn);
    midiLearnClearBtn.setButtonText("Clear");
    midiLearnClearBtn.onClick = [this]
    {
        const int index = midiLearnMappingsBox.getSelectedItemIndex();
        if (index >= 0 && index < midiLearnMappedParamIds.size())
            proc.clearMidiLearn(midiLearnMappedParamIds[index]);
        refreshMidiLearnPanel();
    };

    addAndMakeVisible(midiLearnResetBtn);
    midiLearnResetBtn.setButtonText("Reset");
    midiLearnResetBtn.onClick = [this]
    {
        proc.clearAllMidiLearn();
        refreshMidiLearnPanel();
    };

    addAndMakeVisible(midiLearnStatusLabel);
    midiLearnStatusLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(9.0f)));
    midiLearnStatusLabel.setColour(juce::Label::textColourId, col::textDim);
    midiLearnStatusLabel.setJustificationType(juce::Justification::centredLeft);

    rebuildPadAttachments();
    rebuildAdvancedFxPage();
    refreshPadSelection();
    refreshPresetMetadata();
    refreshStatusPanel();
    refreshMidiLearnPanel();
    refreshPresetActionButtons();
    refreshFxRackState();
    setUtilityDrawerOpen(false);
    applyTooltips();
    startTimerHz(30);

    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(kAspectRatio);
    getConstrainer()->setMinimumSize(kMinW, kMinH);
    setSize(lay::W, lay::H);
}

DrumSynthAudioProcessorEditor::~DrumSynthAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

// =============================================================================
// Paint
// =============================================================================
void DrumSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    const bool compact = getWidth() < 1180 || getHeight() < 680;
    const int margin = compact ? 12 : 16;
    const int gutter = compact ? 10 : 14;
    const int headerH = compact ? 92 : 96;
    const int panelTopInset = compact ? 40 : 44;
    const int subSectionInset = compact ? 28 : 30;

    g.fillAll(col::bg);

    if (bgTexture.isValid())
    {
        g.setOpacity(0.10f);
        g.setTiledImageFill(bgTexture, 0, 0, 1.0f);
        g.fillRect(getLocalBounds());
        g.setOpacity(1.0f);
    }

    const auto selectedPad = selectedPadFromParam();
    const auto padAccent = padCatColour(selectedPad);
    const auto familyLabel = juce::String(padCatName(selectedPad));
    const auto modeLabel = padModeLabel(selectedPad);
    const auto& selectedFx = kFxModules[static_cast<std::size_t>(juce::jlimit(0, kFxRackCount - 1, selectedFxModule))];

    juce::ColourGradient ambient(padAccent.withAlpha(0.10f), getWidth() * 0.18f, getHeight() * 0.22f,
                                 juce::Colours::transparentBlack, getWidth() * 0.70f, getHeight() * 0.82f, true);
    g.setGradientFill(ambient);
    g.fillRect(getLocalBounds());

    juce::ColourGradient lowerGlow(selectedFx.accent.withAlpha(0.08f), getWidth() * 0.84f, getHeight() * 0.74f,
                                   juce::Colours::transparentBlack, getWidth() * 0.44f, getHeight() * 0.28f, true);
    g.setGradientFill(lowerGlow);
    g.fillRect(getLocalBounds());

    auto page = getLocalBounds().reduced(margin);
    auto header = page.removeFromTop(headerH);

    const int utilityH = utilityDrawerOpen ? juce::jlimit(compact ? 120 : 138, compact ? 152 : 176, page.getHeight() / 4) : 0;
    juce::Rectangle<int> utilityArea;
    if (utilityH > 0)
    {
        page.removeFromBottom(gutter);
        utilityArea = page.removeFromBottom(utilityH);
    }

    const int leftW = juce::jlimit(compact ? 250 : 280, compact ? 290 : 330, static_cast<int>(page.getWidth() * 0.27f));
    const int centerW = juce::jlimit(compact ? 260 : 300, compact ? 320 : 370, static_cast<int>(page.getWidth() * 0.31f));

    auto left = page.removeFromLeft(leftW);
    page.removeFromLeft(gutter);
    auto center = page.removeFromLeft(centerW);
    page.removeFromLeft(gutter);
    auto right = page;

    const int performanceH = juce::jlimit(compact ? 176 : 208, compact ? 228 : 262, static_cast<int>(right.getHeight() * 0.37f));
    auto performance = right.removeFromTop(performanceH);
    right.removeFromTop(gutter);
    auto fx = right;

    auto drawPanel = [&](const juce::Rectangle<int>& area, const juce::Colour accent, const juce::Colour tint)
    {
        auto panel = area.toFloat();
        g.setColour(juce::Colours::black.withAlpha(0.18f));
        g.fillRoundedRectangle(panel.translated(0.0f, kShadowDeep), kShellRadius);
        fillDrumShellPanel(g, panel, kShellRadius, accent, true, tint);
    };

    auto drawPanelHeading = [&](const juce::Rectangle<int>& area, const juce::String& title,
                                const juce::String& subtitle, const juce::Colour accent)
    {
        auto band = area.reduced(14, 12).removeFromTop(panelTopInset - 8);
        auto titleLine = band.removeFromTop(14);
        auto lane = juce::Rectangle<float>(static_cast<float>(titleLine.getX()),
                                           static_cast<float>(band.getBottom() - 1),
                                           juce::jmin(92.0f, static_cast<float>(band.getWidth()) * 0.32f),
                                           1.8f);
        drawGlowStrip(g, lane, accent, 1.1f, kGlowNormal);

        g.setColour(col::textSec.withAlpha(0.96f));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.2f).withStyle("Bold")));
        g.drawText(title, titleLine, juce::Justification::centredLeft);

        g.setColour(col::textDim.withAlpha(0.92f));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.4f)));
        g.drawText(subtitle, band.removeFromTop(12), juce::Justification::centredLeft);
    };

    auto drawSubSectionTitle = [&](const juce::Rectangle<int>& area, const juce::String& text, const juce::Colour accent)
    {
        auto band = area.reduced(12, 10).removeFromTop(subSectionInset - 8);
        auto titleLine = band.removeFromTop(14);
        auto lane = juce::Rectangle<float>(static_cast<float>(titleLine.getX()),
                                           static_cast<float>(band.getBottom() - 1),
                                           juce::jmin(68.0f, static_cast<float>(band.getWidth()) * 0.28f),
                                           1.6f);
        drawGlowStrip(g, lane, accent, 0.9f, kGlowNormal);

        g.setColour(col::textSec.withAlpha(0.92f));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.5f).withStyle("Bold")));
        g.drawText(text, titleLine, juce::Justification::centredLeft);
    };

    drawPanel(header, col::accent, col::accent.withAlpha(0.08f));
    drawPanel(left, padAccent, padAccent.withAlpha(0.09f));
    drawPanel(center, padAccent, padAccent.withAlpha(0.04f));
    drawPanel(performance, col::accent, col::accent.withAlpha(0.05f));
    drawPanel(fx, selectedFx.accent, selectedFx.accent.withAlpha(0.05f));
    if (utilityDrawerOpen)
        drawPanel(utilityArea, col::accent.darker(0.10f), col::accent.withAlpha(0.04f));

    auto headerInner = header.reduced(compact ? 12 : 14, compact ? 12 : 14);
    auto identityWell = headerInner.removeFromLeft(compact ? 120 : 146);
    auto gainWell = headerInner.removeFromRight(compact ? 82 : 92);
    auto headerBody = headerInner;
    auto headerRow1 = headerBody.removeFromTop(compact ? 32 : 34);
    headerBody.removeFromTop(compact ? 6 : 8);
    auto headerRow2 = headerBody.removeFromTop(compact ? 24 : 26);

    fillDrumShellCavity(g, identityWell.toFloat(), kPanelRadius, col::accent, col::accent.withAlpha(0.05f));
    fillDrumShellCavity(g, headerRow1.toFloat(), kItemRadius, padAccent, padAccent.withAlpha(0.04f));
    fillDrumShellCavity(g, headerRow2.toFloat(), kItemRadius, col::accent, juce::Colour(0xff51626a).withAlpha(0.04f));
    fillDrumShellCavity(g, gainWell.toFloat(), kPanelRadius, col::accent, col::accent.withAlpha(0.04f));

    {
        auto idText = identityWell.reduced(14, 10);
        g.setColour(col::accent.withAlpha(0.94f));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(16.5f).withStyle("Bold")));
        g.drawText("MIS", idText.removeFromTop(20), juce::Justification::bottomLeft);

        g.setColour(col::textSec);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.6f)));
        g.drawText("Drum Synth", idText.removeFromTop(14), juce::Justification::topLeft);

        g.setColour(col::textDim.withAlpha(0.82f));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.1f)));
        g.drawText("kit architect", idText, juce::Justification::topLeft);
    }

    {
        auto outLabel = gainWell.reduced(8, 6).removeFromTop(10);
        g.setColour(col::textDim.withAlpha(0.88f));
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(8.8f).withStyle("Bold")));
        g.drawText("OUTPUT", outLabel, juce::Justification::centred);
    }

    drawPanelHeading(left, "PAD BANK", "select, load and route the active voice", padAccent);
    drawPanelHeading(center, "VOICE DESIGN", "envelope, pitch contour and body shaping", padAccent);
    drawPanelHeading(performance, "PLAY + MOTION", "macro gestures, trigger mode and modulation", col::accent);
    drawPanelHeading(fx, "MIX BUS", "kit-wide processing and final polish", selectedFx.accent);
    if (utilityDrawerOpen)
        drawPanelHeading(utilityArea, "UTILITY BAY", "metering, MIDI learn and runtime status", col::accent);

    const auto chipY = left.getY() + (compact ? 38 : 42);
    drawStatusChip(g, juce::Rectangle<float>(static_cast<float>(left.getX() + 14), static_cast<float>(chipY), 70.0f, 18.0f),
                   familyLabel, col::surface.interpolatedWith(padAccent, 0.18f), padAccent.withAlpha(0.72f));
    drawStatusChip(g, juce::Rectangle<float>(static_cast<float>(left.getX() + 90), static_cast<float>(chipY), 74.0f, 18.0f),
                   modeLabel, col::surface.interpolatedWith(padAccent, 0.10f), padAccent.withAlpha(0.58f));

    auto centerInner = center.reduced(12);
    centerInner.removeFromTop(panelTopInset);
    const int groupGap = compact ? 8 : 10;
    const int ampH = juce::jlimit(120, 170, static_cast<int>(centerInner.getHeight() * 0.34f));
    const int pitchH = juce::jlimit(92, 128, static_cast<int>(centerInner.getHeight() * 0.23f));
    auto ampArea = centerInner.removeFromTop(ampH);
    centerInner.removeFromTop(groupGap);
    auto pitchArea = centerInner.removeFromTop(pitchH);
    centerInner.removeFromTop(groupGap);
    auto toneArea = centerInner;
    fillDrumShellCavity(g, ampArea.toFloat(), 8.0f, padAccent, padAccent.withAlpha(0.04f));
    fillDrumShellCavity(g, pitchArea.toFloat(), 8.0f, padAccent, padAccent.withAlpha(0.03f));
    fillDrumShellCavity(g, toneArea.toFloat(), 8.0f, col::accent, col::accent.withAlpha(0.03f));
    drawSubSectionTitle(ampArea, "AMP ENVELOPE", padAccent);
    drawSubSectionTitle(pitchArea, "PITCH SHAPE", padAccent.withAlpha(0.86f));
    drawSubSectionTitle(toneArea, "BODY + TONE", col::accent);

    auto performanceInner = performance.reduced(12);
    performanceInner.removeFromTop(panelTopInset);
    const int macroH = juce::jlimit(100, 136, static_cast<int>(performanceInner.getHeight() * 0.48f));
    auto macroArea = performanceInner.removeFromTop(macroH);
    performanceInner.removeFromTop(10);
    auto controlsArea = performanceInner;
    fillDrumShellCavity(g, macroArea.toFloat(), 8.0f, col::accent, col::accent.withAlpha(0.03f));
    fillDrumShellCavity(g, controlsArea.toFloat(), 8.0f, col::accent.darker(0.12f), juce::Colour(0xff4a6560).withAlpha(0.03f));
    drawSubSectionTitle(macroArea, "MACRO SHAPERS", col::accent);
    drawSubSectionTitle(controlsArea, "PLAY MODE", col::accent.darker(0.12f));

    auto fxInner = fx.reduced(12);
    fxInner.removeFromTop(panelTopInset);
    const int rackW = juce::jlimit(150, 182, static_cast<int>(fxInner.getWidth() * 0.34f));
    auto rackArea = fxInner.removeFromLeft(rackW);
    fxInner.removeFromLeft(10);
    auto detailArea = fxInner;
    fillDrumShellCavity(g, rackArea.toFloat(), 8.0f, selectedFx.accent, selectedFx.accent.withAlpha(0.03f));
    fillDrumShellCavity(g, detailArea.toFloat(), 8.0f, selectedFx.accent, selectedFx.accent.withAlpha(0.03f));
    drawSubSectionTitle(rackArea, "CHAIN", selectedFx.accent);
    drawSubSectionTitle(detailArea, juce::String(selectedFx.label).toUpperCase(), selectedFx.accent);
    g.setColour(col::textDim);
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.5f)));
    g.drawText(selectedFx.summary,
               detailArea.getX() + 22, detailArea.getY() + 32,
               detailArea.getWidth() - 32, 16,
               juce::Justification::centredLeft);

    if (utilityDrawerOpen)
    {
        auto utilityInner = utilityArea.reduced(12);
        utilityInner.removeFromTop(panelTopInset);
        fillDrumShellCavity(g, utilityInner.toFloat(), 8.0f, col::accent, col::accent.withAlpha(0.02f));
        g.setColour(col::textDim);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.5f)));
        g.drawText("Runtime status, metering, and MIDI Learn stay accessible without crowding the main edit path.",
                   utilityArea.getX() + 22, utilityArea.getY() + 32,
                   utilityArea.getWidth() - 32, 16, juce::Justification::centredLeft);
    }
}

// =============================================================================
// Resized — proportional layout based on current window size
// =============================================================================
void DrumSynthAudioProcessorEditor::resized()
{
    const bool compact = getWidth() < 1180 || getHeight() < 680;
    const int margin = compact ? 12 : 16, gutter = compact ? 10 : 14, headerH = compact ? 92 : 96, gap = compact ? 6 : 8;
    const int panelTopInset = compact ? 40 : 44;
    const int leftTopInset = compact ? 54 : 58;
    const int sectionTopInset = compact ? 28 : 30;
    auto page = getLocalBounds().reduced(margin);
    auto header = page.removeFromTop(headerH);
    const int utilityH = utilityDrawerOpen ? juce::jlimit(compact ? 120 : 138, compact ? 152 : 176, page.getHeight() / 4) : 0;
    juce::Rectangle<int> utilityArea;
    if (utilityH > 0) { page.removeFromBottom(gutter); utilityArea = page.removeFromBottom(utilityH); }

    const int leftW = juce::jlimit(compact ? 250 : 280, compact ? 290 : 330, static_cast<int>(page.getWidth() * 0.27f));
    const int centerW = juce::jlimit(compact ? 260 : 300, compact ? 320 : 370, static_cast<int>(page.getWidth() * 0.31f));
    auto left = page.removeFromLeft(leftW); page.removeFromLeft(gutter);
    auto center = page.removeFromLeft(centerW); page.removeFromLeft(gutter);
    auto right = page;
    const int performanceH = juce::jlimit(compact ? 176 : 208, compact ? 228 : 262, static_cast<int>(right.getHeight() * 0.37f));
    auto performance = right.removeFromTop(performanceH); right.removeFromTop(gutter); auto fx = right;

    auto layoutGrid = [&](juce::Rectangle<int> area, auto applyCell, int count, int columns, int topInset = 26)
    {
        auto content = area.reduced(8); content.removeFromTop(topInset);
        const int rows = juce::jmax(1, (count + columns - 1) / columns);
        const int cellGap = compact ? 6 : 8;
        const int cellW = (content.getWidth() - cellGap * (columns - 1)) / columns;
        const int cellH = (content.getHeight() - cellGap * (rows - 1)) / rows;
        for (int i = 0; i < count; ++i)
        {
            const int c = i % columns, r = i / columns;
            applyCell(i, juce::Rectangle<int>(content.getX() + c * (cellW + cellGap),
                                              content.getY() + r * (cellH + cellGap), cellW, cellH).reduced(2));
        }
    };

    {
        auto hi = header.reduced(compact ? 12 : 14, compact ? 12 : 14);
        auto gainArea = hi.removeFromRight(compact ? 82 : 92); hi.removeFromLeft(compact ? 126 : 154);
        auto row1 = hi.removeFromTop(compact ? 30 : 32); hi.removeFromTop(gap); auto row2 = hi.removeFromTop(compact ? 24 : 26);
        gainDial.setBounds(gainArea.withSizeKeepingCentre(compact ? 56 : 62, compact ? 56 : 62));

        const int navW = compact ? 30 : 32, saveW = compact ? 60 : 68, saveAsW = compact ? 72 : 82, deleteW = compact ? 66 : 72, importW = compact ? 68 : 78;
        auto row1Left = row1; prevPresetBtn.setBounds(row1Left.removeFromLeft(navW)); row1Left.removeFromLeft(gap);
        auto actions = row1Left.removeFromRight(navW + gap + saveW + gap + saveAsW + gap + deleteW + gap + importW);
        presetBox.setBounds(row1Left);
        nextPresetBtn.setBounds(actions.removeFromLeft(navW)); actions.removeFromLeft(gap);
        saveBtn.setBounds(actions.removeFromLeft(saveW)); actions.removeFromLeft(gap);
        saveAsBtn.setBounds(actions.removeFromLeft(saveAsW)); actions.removeFromLeft(gap);
        deleteBtn.setBounds(actions.removeFromLeft(deleteW)); actions.removeFromLeft(gap);
        importBtn.setBounds(actions.removeFromLeft(importW));

        auto row2Left = row2;
        const int sourceW = compact ? 96 : 108, familyW = compact ? 116 : 128, roleW = compact ? 122 : 138, tagW = compact ? 100 : 116;
        presetSourceFilterBox.setBounds(row2Left.removeFromLeft(sourceW)); row2Left.removeFromLeft(gap);
        presetFamilyFilterBox.setBounds(row2Left.removeFromLeft(familyW)); row2Left.removeFromLeft(gap);
        presetRoleFilterBox.setBounds(row2Left.removeFromLeft(roleW)); row2Left.removeFromLeft(gap);
        presetTagFilterBox.setBounds(row2Left.removeFromLeft(tagW)); row2Left.removeFromLeft(gap);
        presetMetaLabel.setBounds(row2Left);
    }

    {
        auto li = left.reduced(12); li.removeFromTop(leftTopInset);
        auto info = li.removeFromTop(compact ? 36 : 42);
        instrTitle.setBounds(info.removeFromTop(22)); info.removeFromTop(4); padSummaryLabel.setBounds(info.removeFromTop(14)); li.removeFromTop(compact ? 10 : 12);
        auto bottom = li.removeFromBottom(compact ? 32 : 34); li.removeFromBottom(8); auto grid = li.reduced(4);
        constexpr int cols = 4, rows = 3; const int padGap = compact ? 8 : 10;
        const int side = juce::jmax(48, juce::jmin((grid.getWidth() - padGap * (cols - 1)) / cols, (grid.getHeight() - padGap * (rows - 1)) / rows));
        const int startX = grid.getX() + juce::jmax(0, (grid.getWidth() - (cols * side + (cols - 1) * padGap)) / 2);
        const int startY = grid.getY() + juce::jmax(0, (grid.getHeight() - (rows * side + (rows - 1) * padGap)) / 2);
        for (int i = 0; i < mds::kNumPads; ++i)
            pads[static_cast<std::size_t>(i)].setBounds(startX + (i % cols) * (side + padGap), startY + (i / cols) * (side + padGap), side, side);
        outputBox.setBounds(bottom.removeFromRight(compact ? 92 : 104)); bottom.removeFromRight(gap); padPresetBox.setBounds(bottom);
    }

    {
        auto ci = center.reduced(12); ci.removeFromTop(panelTopInset);
        const int ampH = juce::jlimit(120, 170, static_cast<int>(ci.getHeight() * 0.34f));
        const int pitchH = juce::jlimit(92, 128, static_cast<int>(ci.getHeight() * 0.23f));
        auto amp = ci.removeFromTop(ampH); ci.removeFromTop(10); auto pitch = ci.removeFromTop(pitchH); ci.removeFromTop(10); auto tone = ci;
        const std::array<int, 4> ampOrder { 0, 3, 2, 10 }; const std::array<int, 3> pitchOrder { 1, 4, 5 }; const std::array<int, 4> toneOrder { 6, 7, 8, 9 };
        layoutGrid(amp, [&](int i, juce::Rectangle<int> cell){ auto si = static_cast<std::size_t>(ampOrder[static_cast<std::size_t>(i)]); padDlLabels[si].setBounds(cell.removeFromTop(16)); padDials[si].setBounds(cell); }, 4, 2, sectionTopInset);
        layoutGrid(pitch, [&](int i, juce::Rectangle<int> cell){ auto si = static_cast<std::size_t>(pitchOrder[static_cast<std::size_t>(i)]); padDlLabels[si].setBounds(cell.removeFromTop(16)); padDials[si].setBounds(cell); }, 3, 3, sectionTopInset);
        layoutGrid(tone, [&](int i, juce::Rectangle<int> cell){ auto si = static_cast<std::size_t>(toneOrder[static_cast<std::size_t>(i)]); padDlLabels[si].setBounds(cell.removeFromTop(16)); padDials[si].setBounds(cell); }, 4, 2, sectionTopInset);
    }

    {
        auto pi = performance.reduced(12); pi.removeFromTop(panelTopInset);
        const int macroH = juce::jlimit(100, 136, static_cast<int>(pi.getHeight() * 0.48f));
        auto macroArea = pi.removeFromTop(macroH); pi.removeFromTop(10); auto controls = pi;
        layoutGrid(macroArea, [&](int i, juce::Rectangle<int> cell){ auto si = static_cast<std::size_t>(i); macroLbls[si].setBounds(cell.removeFromTop(16)); macroDials[si].setBounds(cell); }, kMacroN, 4, sectionTopInset);

        auto ctl = controls.reduced(8); ctl.removeFromTop(sectionTopInset); auto top = ctl.removeFromTop(compact ? 26 : 28); ctl.removeFromTop(8);
        const int topW = (top.getWidth() - gap * 3) / 4;
        qualityModeBox.setBounds(top.removeFromLeft(topW)); top.removeFromLeft(gap);
        velocityCurveBox.setBounds(top.removeFromLeft(topW)); top.removeFromLeft(gap);
        singleNoteBtn.setBounds(top.removeFromLeft(topW)); top.removeFromLeft(gap); utilityDrawerBtn.setBounds(top);

        auto lfo = ctl.removeFromLeft((ctl.getWidth() - gap) / 2); ctl.removeFromLeft(gap); auto human = ctl;
        lfo = lfo.reduced(4); lfoLabel.setBounds(lfo.removeFromTop(14)); auto wave = lfo.removeFromBottom(24); lfo.removeFromBottom(4);
        const int lfoW = (lfo.getWidth() - gap) / 2; lfoRateDial.setBounds(lfo.removeFromLeft(lfoW).reduced(2)); lfo.removeFromLeft(gap); lfoDepthDial.setBounds(lfo.reduced(2)); lfoWaveBox.setBounds(wave);
        human = human.reduced(4); humanizeLabel.setBounds(human.removeFromTop(14)); human.removeFromTop(4);
        const int humanW = (human.getWidth() - gap) / 2; humanizeTimingDial.setBounds(human.removeFromLeft(humanW).reduced(2)); human.removeFromLeft(gap); humanizeLevelDial.setBounds(human.reduced(2));
    }

    {
        auto fi = fx.reduced(12); fi.removeFromTop(panelTopInset);
        auto rack = fi.removeFromLeft(juce::jlimit(150, 182, static_cast<int>(fi.getWidth() * 0.34f))); fi.removeFromLeft(10); auto detail = fi;
        auto rackInner = rack.reduced(8); rackInner.removeFromTop(sectionTopInset);
        for (int i = 0, h = (rackInner.getHeight() - 6 * (kFxRackCount - 1)) / kFxRackCount; i < kFxRackCount; ++i)
        {
            fxRackItems[static_cast<std::size_t>(i)].setBounds(rackInner.removeFromTop(h));
            if (i < kFxRackCount - 1) rackInner.removeFromTop(6);
        }

        auto di = detail.reduced(8); di.removeFromTop(compact ? 54 : 58);
        const bool showA = advancedToggleA.isVisible(), showB = advancedToggleB.isVisible(), showChoice = advancedFxPageBox.isVisible();
        if (showA || showB || showChoice)
        {
            auto top = di.removeFromTop(compact ? 24 : 26); di.removeFromTop(8);
            if (showChoice) { advancedFxPageBox.setBounds(top.removeFromRight(juce::jlimit(122, 190, top.getWidth() / 3))); top.removeFromRight(gap); } else advancedFxPageBox.setBounds({});
            if (showB) { advancedToggleB.setBounds(top.removeFromRight(compact ? 94 : 108)); top.removeFromRight(gap); } else advancedToggleB.setBounds({});
            if (showA) advancedToggleA.setBounds(top.removeFromLeft(juce::jlimit(106, 132, top.getWidth()))); else advancedToggleA.setBounds({});
        }
        else { advancedToggleA.setBounds({}); advancedToggleB.setBounds({}); advancedFxPageBox.setBounds({}); }

        std::array<int, kFxDetailPool> visible {}; int count = 0;
        for (int i = 0; i < kFxDetailPool; ++i)
            if (fxDials[static_cast<std::size_t>(i)].isVisible() && fxLbls[static_cast<std::size_t>(i)].isVisible()) visible[static_cast<std::size_t>(count++)] = i;
            else { fxLbls[static_cast<std::size_t>(i)].setBounds({}); fxDials[static_cast<std::size_t>(i)].setBounds({}); }
        if (count > 0)
        {
            const int columns = count == 1 ? 1 : (count == 2 ? 2 : (count <= 3 ? 3 : 4));
            layoutGrid(di, [&](int i, juce::Rectangle<int> cell){ auto si = static_cast<std::size_t>(visible[static_cast<std::size_t>(i)]); fxLbls[si].setBounds(cell.removeFromTop(16)); fxDials[si].setBounds(cell); }, count, columns, 0);
        }
    }

    if (utilityDrawerOpen)
    {
        auto ui = utilityArea.reduced(12); ui.removeFromTop(compact ? 54 : 58);
        auto row1 = ui.removeFromTop(24); ui.removeFromTop(8); auto row2 = ui.removeFromTop(18); ui.removeFromTop(8); auto row3 = ui.removeFromTop(24);
        tooltipModeBtn.setBounds(row1.removeFromLeft(compact ? 72 : 80)); row1.removeFromLeft(gap);
        midiCCPageLabel.setBounds(row1.removeFromLeft(compact ? 122 : 144)); row1.removeFromLeft(gap);
        clipResetBtn.setBounds(row1.removeFromRight(compact ? 74 : 84)); row1.removeFromRight(gap); engineStatusLabel.setBounds(row1);
        mainMeterLabel.setBounds(row2.removeFromLeft(compact ? 88 : 96)); row2.removeFromLeft(gap);
        mainMeterBar.setBounds(row2.removeFromLeft(static_cast<int>(row2.getWidth() * 0.42f))); row2.removeFromLeft(gap);
        auxMeterLabel.setBounds(row2.removeFromLeft(compact ? 68 : 76)); row2.removeFromLeft(gap); auxMeterBar.setBounds(row2);
        auto status = row3.removeFromRight(juce::jmax(compact ? 180 : 220, row3.getWidth() / 5)); row3.removeFromRight(gap);
        midiLearnResetBtn.setBounds(row3.removeFromRight(compact ? 68 : 76)); row3.removeFromRight(gap);
        midiLearnClearBtn.setBounds(row3.removeFromRight(compact ? 68 : 72)); row3.removeFromRight(gap);
        midiLearnArmBtn.setBounds(row3.removeFromRight(compact ? 72 : 78)); row3.removeFromRight(gap);
        midiLearnTargetBox.setBounds(row3.removeFromLeft((row3.getWidth() - gap) / 2)); row3.removeFromLeft(gap);
        midiLearnMappingsBox.setBounds(row3); midiLearnStatusLabel.setBounds(status);
    }
    else
    {
        tooltipModeBtn.setBounds({}); midiCCPageLabel.setBounds({}); engineStatusLabel.setBounds({}); clipResetBtn.setBounds({});
        mainMeterLabel.setBounds({}); mainMeterBar.setBounds({}); auxMeterLabel.setBounds({}); auxMeterBar.setBounds({});
        midiLearnTargetBox.setBounds({}); midiLearnMappingsBox.setBounds({}); midiLearnArmBtn.setBounds({});
        midiLearnClearBtn.setBounds({}); midiLearnResetBtn.setBounds({}); midiLearnStatusLabel.setBounds({});
    }
}

// =============================================================================
// Timer
// =============================================================================
void DrumSynthAudioProcessorEditor::timerCallback()
{
    for (auto& p : pads) p.tick();

    rebuildPadAttachments();
    refreshPadPresetList();
    refreshPadSelection();
    refreshPresetMetadata();
    refreshStatusPanel();
    refreshMidiLearnPanel();

    // MIDI CC page label sync
    {
        const int pg = proc.getMidiCCPage();
        auto newTxt = juce::String("CC Page: ") + proc.getCCPageName(pg);
        if (midiCCPageLabel.getText() != newTxt)
            midiCCPageLabel.setText(newTxt, juce::dontSendNotification);
    }

    // Sync preset selection with processor state
    const auto currentEntry = proc.getCurrentPresetEntry();
    int targetIndex = -1;
    for (int i = 0; i < visiblePresetEntries.size(); ++i)
    {
        const auto& entry = visiblePresetEntries.getReference(i);
        const bool samePreset = entry.isFactory == currentEntry.isFactory
            && (entry.isFactory
                    ? entry.factoryIndex == currentEntry.factoryIndex
                    : entry.presetFile == currentEntry.presetFile);
        if (samePreset)
        {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex >= 0 && presetBox.getSelectedItemIndex() != targetIndex)
        presetBox.setSelectedItemIndex(targetIndex, juce::dontSendNotification);

    // Delete only available for user presets
    refreshPresetActionButtons();
    refreshFxRackState();
}

// =============================================================================
// Pad attachment management
// =============================================================================
void DrumSynthAudioProcessorEditor::rebuildPadAttachments()
{
    auto padIdx = selectedPadFromParam();
    if (padIdx == cachedPad) return;
    cachedPad = padIdx;
    const auto& profile = getPadUiProfile(cachedPad);

    for (auto& a : padDlAttach) a.reset();
    outAtt.reset();

    for (int i = 0; i < kPadCtrlN; ++i)
    {
        auto si = static_cast<std::size_t>(i);
        auto id = DrumSynthAudioProcessor::makePadParamId(
            cachedPad, profile[si].suffix);
        padDlAttach[si] = std::make_unique<SliderAttach>(
            proc.getAPVTS(), id, padDials[si]);
        padDlLabels[si].setText(profile[si].label, juce::dontSendNotification);
        updateSliderTextFormat(padDials[si], id);
    }

    outAtt = std::make_unique<ComboBoxAttach>(
        proc.getAPVTS(),
        DrumSynthAudioProcessor::makePadParamId(cachedPad, "output"),
        outputBox);

    auto catC = padCatColour(padIdx);
    instrTitle.setText("Pad " + juce::String(padIdx + 1) + " - " + juce::String(mds::makePadName(padIdx)),
                       juce::dontSendNotification);
    instrTitle.setColour(juce::Label::textColourId, catC.brighter(0.10f));
    const auto summary = juce::String(padCatName(padIdx)) + "  |  " + padModeLabel(padIdx) + "  |  " + padSummaryText(padIdx);
    padSummaryLabel.setText(summary, juce::dontSendNotification);
    padSummaryLabel.setTooltip(summary);
    padSummaryLabel.setColour(juce::Label::textColourId, col::textDim);

    for (auto& dial : padDials)
        dial.setColour(juce::Slider::rotarySliderFillColourId, catC);

    applyTooltips();
    repaint();
}

void DrumSynthAudioProcessorEditor::rebuildAdvancedFxPage()
{
    for (auto& att : fxAtt)
        att.reset();
    advancedToggleAAtt.reset();
    advancedToggleBAtt.reset();
    advancedChoiceAtt.reset();

    const auto& module = kFxModules[static_cast<std::size_t>(juce::jlimit(0, kFxRackCount - 1, selectedFxModule))];

    advancedToggleA.setVisible(module.enableParamId != nullptr && juce::String(module.enableParamId).isNotEmpty());
    advancedToggleA.setButtonText("Enabled");
    advancedToggleA.setColour(juce::ToggleButton::tickColourId, module.accent);
    if (advancedToggleA.isVisible())
        advancedToggleAAtt = std::make_unique<ButtonAttach>(proc.getAPVTS(), module.enableParamId, advancedToggleA);

    const bool hasSecondary = module.secondaryToggle.paramId != nullptr && juce::String(module.secondaryToggle.label).isNotEmpty();
    advancedToggleB.setVisible(hasSecondary);
    advancedToggleB.setButtonText(module.secondaryToggle.label);
    advancedToggleB.setColour(juce::ToggleButton::tickColourId, module.accent);
    if (hasSecondary)
        advancedToggleBAtt = std::make_unique<ButtonAttach>(proc.getAPVTS(), module.secondaryToggle.paramId, advancedToggleB);

    advancedFxPageBox.clear(juce::dontSendNotification);
    advancedFxPageBox.setVisible(false);
    advancedFxPageBox.setEnabled(false);
    if (juce::String(module.label).equalsIgnoreCase("Delay"))
    {
        for (const auto& label : { "1/4", "1/8", "1/16", "Dotted 1/8", "Triplet 1/8" })
            advancedFxPageBox.addItem(label, advancedFxPageBox.getNumItems() + 1);
        advancedFxPageBox.setVisible(true);
        advancedFxPageBox.setEnabled(true);
        advancedFxPageBox.setTextWhenNothingSelected("Note Division");
        advancedChoiceAtt = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), "delay_note_div", advancedFxPageBox);
    }

    for (int i = 0; i < kFxDetailPool; ++i)
    {
        auto si = static_cast<std::size_t>(i);
        const auto& def = module.dials[si];
        const bool visible = def.paramId != nullptr && juce::String(def.label).isNotEmpty();
        fxLbls[si].setVisible(visible);
        fxDials[si].setVisible(visible);
        if (!visible)
            continue;

        fxLbls[si].setText(def.label, juce::dontSendNotification);
        fxDials[si].setColour(juce::Slider::rotarySliderFillColourId, module.accent);
        updateSliderTextFormat(fxDials[si], def.paramId);
        fxAtt[si] = std::make_unique<SliderAttach>(proc.getAPVTS(), def.paramId, fxDials[si]);
    }

    applyTooltips();
    resized();
    repaint();
}

void DrumSynthAudioProcessorEditor::refreshPadSelection()
{
    auto sel = selectedPadFromParam();
    for (int i = 0; i < mds::kNumPads; ++i)
        pads[static_cast<std::size_t>(i)].setSelected(i == sel);
}

// =============================================================================
// Preset management
// =============================================================================
void DrumSynthAudioProcessorEditor::refreshPresetList()
{
    const auto selectedFamily = presetFamilyFilterBox.getText();
    const auto selectedRole = presetRoleFilterBox.getText();
    const auto selectedTag = presetTagFilterBox.getText();

    presetUiRefreshing = true;
    presetBox.clear(juce::dontSendNotification);
    visiblePresetEntries.clear();
    userPresetFiles.clear();

    if (presetFamilyFilterBox.getNumItems() == 0)
        presetFamilyFilterBox.addItem("All Families", 1);
    if (presetRoleFilterBox.getNumItems() == 0)
        presetRoleFilterBox.addItem("All Roles", 1);
    if (presetTagFilterBox.getNumItems() == 0)
        presetTagFilterBox.addItem("All Tags", 1);

    auto refillChoiceBox = [](juce::ComboBox& box,
                              const juce::String& allLabel,
                              const juce::StringArray& values,
                              const juce::String& selectedText)
    {
        box.clear(juce::dontSendNotification);
        box.addItem(allLabel, 1);
        int itemId = 2;
        for (const auto& value : values)
            box.addItem(value, itemId++);

        const auto targetText = selectedText.isNotEmpty() ? selectedText : allLabel;
        int matchId = 0;
        for (int i = 0; i < box.getNumItems(); ++i)
        {
            if (box.getItemText(i) == targetText)
            {
                matchId = box.getItemId(i);
                break;
            }
        }
        box.setSelectedId(matchId > 0 ? matchId : 1, juce::dontSendNotification);
    };

    const auto allEntries = proc.scanPresetLibrary();
    juce::StringArray families;
    juce::StringArray roles;
    juce::StringArray tags;
    families.add("All");
    roles.add("All");
    tags.add("All");
    for (const auto& entry : allEntries)
    {
        if (entry.familyLabel.isNotEmpty())
            families.addIfNotAlreadyThere(entry.familyLabel);
        if (entry.mixRole.isNotEmpty())
            roles.addIfNotAlreadyThere(entry.mixRole);
        for (const auto& tag : entry.tags)
            tags.addIfNotAlreadyThere(tag);
    }
    families.sort(true);
    roles.sort(true);
    tags.sort(true);
    if (families[0] != "All")
        families.move(families.indexOf("All"), 0);
    if (roles[0] != "All")
        roles.move(roles.indexOf("All"), 0);
    if (tags[0] != "All")
        tags.move(tags.indexOf("All"), 0);

    refillChoiceBox(presetFamilyFilterBox, "All Families", families, selectedFamily);
    refillChoiceBox(presetRoleFilterBox, "All Roles", roles, selectedRole);
    refillChoiceBox(presetTagFilterBox, "All Tags", tags, selectedTag);

    factoryPresetCount = proc.getFactoryPresetNames().size();
    for (const auto& entry : allEntries)
    {
        if (!entry.isFactory && entry.presetFile.existsAsFile())
            userPresetFiles.addIfNotAlreadyThere(entry.presetFile);

        if (!presetEntryMatchesFilters(entry))
            continue;

        visiblePresetEntries.add(entry);

        juce::StringArray parts;
        parts.add(entry.name);
        if (!entry.familyLabel.isEmpty())
            parts.add(entry.familyLabel);
        if (!entry.mixRole.isEmpty())
            parts.add(entry.mixRole);
        if (entry.nominalPeakDb < 0.0f)
            parts.add(juce::String(entry.nominalPeakDb, 1) + " dB");
        if (!entry.outputProfile.isEmpty())
            parts.add(entry.outputProfile);

        auto label = parts.joinIntoString("  |  ");
        if (!entry.isFactory)
            label = "[USER] " + label;
        presetBox.addItem(label, visiblePresetEntries.size());
    }

    const auto currentEntry = proc.getCurrentPresetEntry();
    for (int i = 0; i < visiblePresetEntries.size(); ++i)
    {
        const auto& entry = visiblePresetEntries.getReference(i);
        const bool samePreset = entry.isFactory == currentEntry.isFactory
            && (entry.isFactory
                    ? entry.factoryIndex == currentEntry.factoryIndex
                    : entry.presetFile == currentEntry.presetFile);
        if (samePreset)
        {
            presetBox.setSelectedItemIndex(i, juce::dontSendNotification);
            break;
        }
    }

    presetUiRefreshing = false;
    presetBox.setEnabled(visiblePresetEntries.size() > 0);
    refreshPresetMetadata();
    refreshPresetActionButtons();
}

void DrumSynthAudioProcessorEditor::refreshPadPresetList()
{
    const int padIndex = selectedPadFromParam();
    const auto currentIdx = proc.getCurrentPadPresetIndex(padIndex);
    const auto factoryNames = proc.getFactoryPadPresetNames(padIndex);
    const int targetSelection = currentIdx >= 0 ? currentIdx + 1 : 0;

    if (padIndex == cachedPadPresetListPad
        && currentIdx == cachedPadPresetFactoryIndex
        && factoryNames.size() == cachedPadPresetFactoryCount)
    {
        if (padPresetBox.getSelectedItemIndex() != targetSelection)
            padPresetBox.setSelectedItemIndex(targetSelection, juce::dontSendNotification);
        return;
    }

    padPresetBox.clear(juce::dontSendNotification);
    padPresetBox.addItem("Pad Preset: Custom", 1);
    for (int i = 0; i < factoryNames.size(); ++i)
        padPresetBox.addItem(factoryNames[i], i + 2);

    if (padPresetBox.getSelectedItemIndex() != targetSelection)
        padPresetBox.setSelectedItemIndex(targetSelection, juce::dontSendNotification);

    cachedPadPresetListPad = padIndex;
    cachedPadPresetFactoryIndex = currentIdx;
    cachedPadPresetFactoryCount = factoryNames.size();
}

void DrumSynthAudioProcessorEditor::refreshPresetActionButtons()
{
    const bool isUserPreset = proc.isCurrentPresetUser();
    saveBtn.setButtonText(isUserPreset ? "Save" : "Override");
    saveBtn.setTooltip(isUserPreset
        ? "Update the current user preset."
        : "Create or update a local override of the current factory preset.");
    deleteBtn.setEnabled(isUserPreset);
    deleteBtn.setTooltip(isUserPreset ? "Delete the current user preset." : "Factory presets cannot be deleted.");
    saveAsBtn.setTooltip("Save the current kit as a new user preset.");
    importBtn.setTooltip("Import user presets from a ZIP archive.");
}

void DrumSynthAudioProcessorEditor::refreshPresetMetadata()
{
    const auto entry = proc.getCurrentPresetEntry();
    juce::StringArray parts;

    if (!entry.familyLabel.isEmpty())
        parts.add(entry.familyLabel);
    if (!entry.mixRole.isEmpty())
        parts.add(entry.mixRole);
    if (!entry.tags.isEmpty())
        parts.add(entry.tags.joinIntoString(", "));
    parts.add(juce::String(entry.nominalPeakDb, 1) + " dBFS");
    if (!entry.outputProfile.isEmpty())
        parts.add(entry.outputProfile);
    parts.add(entry.isFactory ? "Factory" : "User");

    auto meta = parts.joinIntoString("  |  ");
    if (!entry.description.isEmpty())
        meta << "  |  " << entry.description;

    presetMetaLabel.setText(meta, juce::dontSendNotification);
    presetMetaLabel.setTooltip(meta);
}

void DrumSynthAudioProcessorEditor::refreshStatusPanel()
{
    const auto peakToDb = [](const float linear)
    {
        return juce::Decibels::gainToDecibels(juce::jmax(linear, 1.0e-6f), -96.0f);
    };

    mainMeterValue = juce::jlimit(0.0, 1.0, static_cast<double>(proc.getMainPeakMeter()));
    auxMeterValue = juce::jlimit(0.0, 1.0, static_cast<double>(proc.getAuxPeakMeter()));

    mainMeterLabel.setText("Main " + juce::String(peakToDb(proc.getMainPeakMeter()), 1)
                               + " / " + juce::String(peakToDb(proc.getMainRmsMeter()), 1) + " dB",
                           juce::dontSendNotification);
    auxMeterLabel.setText("Aux " + juce::String(peakToDb(proc.getAuxPeakMeter()), 1)
                              + " / " + juce::String(peakToDb(proc.getAuxRmsMeter()), 1) + " dB",
                          juce::dontSendNotification);

    const bool delaySyncEnabled = proc.getAPVTS().getRawParameterValue("delay_sync")->load() >= 0.5f;
    const bool auxPostFxEnabled = proc.getAPVTS().getRawParameterValue("aux_post_fx")->load() >= 0.5f;
    const auto bpm = proc.getLastHostBpm();

    juce::StringArray statusParts;
    if (delaySyncEnabled)
    {
        if (proc.isDelaySyncActive() && bpm > 1.0f)
            statusParts.add("Sync to Host " + juce::String(bpm, 1) + " BPM");
        else
            statusParts.add("Sync to Host");
    }
    else
    {
        statusParts.add("Sync Off");
    }
    statusParts.add(auxPostFxEnabled ? "Aux Post-FX" : "Aux Pre-FX");
    statusParts.add(proc.isClipLatched() ? "Clip" : "Safe");
    engineStatusLabel.setText(statusParts.joinIntoString("  |  "), juce::dontSendNotification);

    clipResetBtn.setButtonText(proc.isClipLatched() ? "Clip" : "Safe");
    clipResetBtn.setColour(juce::TextButton::buttonColourId,
                           proc.isClipLatched() ? juce::Colours::darkred : col::surface);
    clipResetBtn.setColour(juce::TextButton::textColourOffId,
                           proc.isClipLatched() ? juce::Colours::white : col::textDim);
}

void DrumSynthAudioProcessorEditor::refreshMidiLearnPanel()
{
    if (midiLearnTargetBox.getNumItems() == 0)
    {
        midiLearnTargetIds = proc.getMidiLearnTargetIds();
        for (int i = 0; i < midiLearnTargetIds.size(); ++i)
        {
            const auto& paramId = midiLearnTargetIds[i];
            midiLearnTargetBox.addItem(proc.getParameterDisplayName(paramId) + " [" + paramId + "]", i + 1);
        }
        if (midiLearnTargetBox.getNumItems() > 0)
            midiLearnTargetBox.setSelectedItemIndex(0, juce::dontSendNotification);
    }

    const auto selectedMappingText = midiLearnMappingsBox.getText();
    midiLearnMappingsBox.clear(juce::dontSendNotification);
    midiLearnMappedParamIds = proc.getMidiLearnedParams();
    for (int i = 0; i < midiLearnMappedParamIds.size(); ++i)
    {
        const auto& paramId = midiLearnMappedParamIds[i];
        const auto cc = proc.getMidiCcForParam(paramId);
        midiLearnMappingsBox.addItem("CC" + juce::String(cc) + " -> " + proc.getParameterDisplayName(paramId), i + 1);
    }
    if (midiLearnMappingsBox.getNumItems() > 0)
    {
        int matchId = 0;
        for (int i = 0; i < midiLearnMappingsBox.getNumItems(); ++i)
        {
            if (midiLearnMappingsBox.getItemText(i) == selectedMappingText)
            {
                matchId = midiLearnMappingsBox.getItemId(i);
                break;
            }
        }
        midiLearnMappingsBox.setSelectedId(matchId > 0 ? matchId : 1, juce::dontSendNotification);
    }

    midiLearnArmBtn.setButtonText(proc.isMidiLearning() ? "Cancel" : "Learn");
    midiLearnClearBtn.setEnabled(midiLearnMappingsBox.getNumItems() > 0);
    midiLearnResetBtn.setEnabled(midiLearnMappingsBox.getNumItems() > 0);

    auto status = juce::String("No MIDI mappings");
    if (proc.isMidiLearning())
    {
        status = "Armed: " + proc.getParameterDisplayName(proc.getMidiLearnParamId());
    }
    else if (const auto cc = proc.getMidiLearnCcNumber(); cc >= 0)
    {
        status = "Captured CC" + juce::String(cc);
        if (!proc.getMidiLearnParamId().isEmpty())
            status << " -> " << proc.getParameterDisplayName(proc.getMidiLearnParamId());
    }
    else if (midiLearnMappedParamIds.size() > 0)
    {
        status = juce::String(midiLearnMappedParamIds.size()) + " mapping(s) active";
    }

    midiLearnStatusLabel.setText(status, juce::dontSendNotification);
}

void DrumSynthAudioProcessorEditor::refreshFxRackState()
{
    for (int i = 0; i < kFxRackCount; ++i)
    {
        const auto& module = kFxModules[static_cast<std::size_t>(i)];
        bool enabled = true;
        if (auto* raw = proc.getAPVTS().getRawParameterValue(module.enableParamId))
            enabled = raw->load() >= 0.5f;

        auto& item = fxRackItems[static_cast<std::size_t>(i)];
        item.setSelected(i == selectedFxModule);
        item.setEnabledState(enabled);
    }
}

void DrumSynthAudioProcessorEditor::setUtilityDrawerOpen(bool shouldOpen)
{
    utilityDrawerOpen = shouldOpen;
    utilityDrawerBtn.setButtonText(utilityDrawerOpen ? "Hide Utility" : "Utility");

    for (auto* component : { static_cast<juce::Component*>(&tooltipModeBtn), static_cast<juce::Component*>(&midiCCPageLabel),
                             static_cast<juce::Component*>(&engineStatusLabel), static_cast<juce::Component*>(&mainMeterLabel),
                             static_cast<juce::Component*>(&mainMeterBar), static_cast<juce::Component*>(&auxMeterLabel),
                             static_cast<juce::Component*>(&auxMeterBar), static_cast<juce::Component*>(&clipResetBtn),
                             static_cast<juce::Component*>(&midiLearnTargetBox), static_cast<juce::Component*>(&midiLearnMappingsBox),
                             static_cast<juce::Component*>(&midiLearnArmBtn), static_cast<juce::Component*>(&midiLearnClearBtn),
                             static_cast<juce::Component*>(&midiLearnResetBtn), static_cast<juce::Component*>(&midiLearnStatusLabel) })
    {
        component->setVisible(utilityDrawerOpen);
    }

    resized();
    repaint();
}

bool DrumSynthAudioProcessorEditor::presetEntryMatchesFilters(const DrumSynthAudioProcessor::PresetLibraryEntry& entry) const
{
    const auto sourceFilter = presetSourceFilterBox.getText();
    if (sourceFilter == "Factory" && !entry.isFactory)
        return false;
    if (sourceFilter == "User" && entry.isFactory)
        return false;

    const auto familyFilter = presetFamilyFilterBox.getText();
    if (familyFilter.isNotEmpty() && familyFilter != "All Families" && entry.familyLabel != familyFilter)
        return false;

    const auto roleFilter = presetRoleFilterBox.getText();
    if (roleFilter.isNotEmpty() && roleFilter != "All Roles" && entry.mixRole != roleFilter)
        return false;

    const auto tagFilter = presetTagFilterBox.getText();
    if (tagFilter.isNotEmpty() && tagFilter != "All Tags" && !entry.tags.contains(tagFilter))
        return false;

    return true;
}

void DrumSynthAudioProcessorEditor::showSaveAsDialog()
{
    auto defaultName = proc.isCurrentPresetUser()
        ? proc.getCurrentUserPresetFile().getFileNameWithoutExtension()
        : juce::String("My Kit");

    auto* aw = new juce::AlertWindow("Save Preset",
                                     "Enter a name for the new preset:",
                                     juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", defaultName, "Preset name:");
    aw->addButton("Save", 1);
    aw->addButton("Cancel", 0);

    aw->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, aw](int result) {
            if (result == 1)
            {
                auto name = aw->getTextEditorContents("name").trim();
                if (name.isNotEmpty() && proc.saveUserPreset(name))
                    refreshPresetList();
            }
            delete aw;
        }), false);
}

void DrumSynthAudioProcessorEditor::saveCurrentPreset()
{
    if (proc.isCurrentPresetUser())
        proc.updateUserPreset(proc.getCurrentUserPresetFile());
    else
        proc.saveFactoryPreset(proc.getCurrentFactoryPresetIndex());

    refreshPresetList();
}

void DrumSynthAudioProcessorEditor::deleteCurrentUserPreset()
{
    if (!proc.isCurrentPresetUser()) return;

    auto file = proc.getCurrentUserPresetFile();
    auto name = file.getFileNameWithoutExtension();

    auto* aw = new juce::AlertWindow("Delete Preset",
                                     "Delete \"" + name + "\"?",
                                     juce::MessageBoxIconType::WarningIcon);
    aw->addButton("Delete", 1);
    aw->addButton("Cancel", 0);

    aw->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, aw, file](int result) {
            if (result == 1)
            {
                proc.deleteUserPreset(file);
                if (factoryPresetCount > 0)
                    proc.applyFactoryPreset(0);
                refreshPresetList();
            }
            delete aw;
        }), false);
}

void DrumSynthAudioProcessorEditor::navigatePreset(int direction)
{
    auto total = presetBox.getNumItems();
    if (total <= 0) return;

    auto current = presetBox.getSelectedItemIndex();
    auto next = current + direction;

    if (next < 0) next = total - 1;
    if (next >= total) next = 0;

    presetBox.setSelectedItemIndex(next);
}

void DrumSynthAudioProcessorEditor::importPresetsFromZip()
{
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Import Presets (ZIP)",
        juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
        "*.zip");

    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto results = fc.getResults();
            if (results.isEmpty()) return;

            auto zipFile = results.getFirst();
            if (!zipFile.existsAsFile()) return;

            juce::FileInputStream zipStream(zipFile);
            if (!zipStream.openedOk()) return;

            juce::ZipFile zip(zipStream);
            auto destDir = DrumSynthAudioProcessor::getUserPresetsDirectory();
            int imported = 0;

            for (int i = 0; i < zip.getNumEntries(); ++i)
            {
                auto* entry = zip.getEntry(i);
                if (entry == nullptr) continue;

                auto name = juce::File::createLegalFileName(
                    juce::File(entry->filename).getFileName());
                if (!name.endsWithIgnoreCase(".xml")) continue;

                auto destFile = destDir.getChildFile(name);
                auto* entryStream = zip.createStreamForEntry(i);
                if (entryStream == nullptr) continue;

                destFile.deleteFile();
                juce::FileOutputStream out(destFile);
                if (out.openedOk())
                {
                    out.writeFromInputStream(*entryStream, -1);
                    ++imported;
                }
                delete entryStream;
            }

            refreshPresetList();

            auto msg = imported > 0
                ? juce::String(imported) + " preset(s) imported."
                : juce::String("No XML presets were found in the ZIP archive.");
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::InfoIcon,
                "Import Presets", msg);
        });
}

// =============================================================================
// Tooltip mode cycling
// =============================================================================
void DrumSynthAudioProcessorEditor::cycleTooltipMode()
{
    tooltipMode = (tooltipMode + 1) % 3;
    const char* labels[] = { "Tips", "Short", "Guide" };
    tooltipModeBtn.setButtonText(labels[tooltipMode]);
    tooltipWindow.setVisible(tooltipMode != 0);
    applyTooltips();
}

void DrumSynthAudioProcessorEditor::applyTooltips()
{
    const auto activePad = cachedPad >= 0 ? cachedPad : selectedPadFromParam();
    const auto& profile = getPadUiProfile(activePad);
    const auto tooltipFor = [&](const juce::String& paramId)
    {
        if (tooltipMode == 1) return shortTooltipForParam(paramId);
        if (tooltipMode == 2) return noviceTooltipForParam(paramId);
        return juce::String();
    };

    for (int i = 0; i < kPadCtrlN; ++i)
    {
        const auto paramId = DrumSynthAudioProcessor::makePadParamId(activePad, profile[static_cast<std::size_t>(i)].suffix);
        padDials[static_cast<std::size_t>(i)].setTooltip(tooltipFor(paramId));
    }

    for (int i = 0; i < kMacroN; ++i)
        macroDials[static_cast<std::size_t>(i)].setTooltip(tooltipFor(kMacroCtrls[static_cast<std::size_t>(i)].paramId));

    const auto& fxModule = kFxModules[static_cast<std::size_t>(juce::jlimit(0, kFxRackCount - 1, selectedFxModule))];
    for (int i = 0; i < kFxDetailPool; ++i)
    {
        const auto& def = fxModule.dials[static_cast<std::size_t>(i)];
        fxDials[static_cast<std::size_t>(i)].setTooltip(def.paramId != nullptr ? tooltipFor(def.paramId) : juce::String());
    }

    gainDial.setTooltip(tooltipFor("output_gain"));
    lfoRateDial.setTooltip(tooltipFor("lfo_rate"));
    lfoDepthDial.setTooltip(tooltipFor("lfo_depth"));
    humanizeTimingDial.setTooltip(tooltipFor("humanize_timing"));
    humanizeLevelDial.setTooltip(tooltipFor("humanize_level"));

    singleNoteBtn.setTooltip(tooltipMode == 0 ? juce::String() : "Trigger the selected pad from any incoming MIDI note.");
    qualityModeBox.setTooltip(tooltipMode == 0 ? juce::String() : "Switch between lower CPU live mode and higher quality studio mode.");
    velocityCurveBox.setTooltip(tooltipMode == 0 ? juce::String() : "Choose how incoming MIDI velocity maps to pad response.");
    utilityDrawerBtn.setTooltip(tooltipMode == 0 ? juce::String() : "Show or hide meters, runtime status, and MIDI Learn tools.");
    advancedToggleA.setTooltip(tooltipMode == 0 ? juce::String() : "Enable or bypass the selected FX module.");
    advancedToggleB.setTooltip(tooltipMode == 0 ? juce::String() : "Sync the selected FX module to the host tempo when available.");
    advancedFxPageBox.setTooltip(tooltipMode == 0 ? juce::String() : "Choose the synced delay note division.");
    padPresetBox.setTooltip(tooltipMode == 0 ? juce::String() : "Load a factory preset into the selected pad.");
    outputBox.setTooltip(tooltipMode == 0 ? juce::String() : "Route the selected pad to the master or an auxiliary output.");
}
