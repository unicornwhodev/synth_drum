#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

using namespace UITheme;

namespace
{
class EditorLookAndFeelHW final : public juce::LookAndFeel_V4
{
public:
    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return UITheme::fontLabel();
    }

    juce::Label* createComboBoxTextBox (juce::ComboBox& box) override
    {
        auto* label = juce::LookAndFeel_V4::createComboBoxTextBox (box);
        label->setFont (UITheme::fontLabel());
        label->setColour (juce::Label::textColourId, UITheme::textMain());
        label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        return label;
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (12, 0, box.getWidth() - 34, box.getHeight());
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto area = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        UITheme::fillPanel (g, area);
        auto rail = area.reduced (12.0f, 8.0f).removeFromTop (2.0f);
        UITheme::drawLED (g, rail, false, UITheme::accentOrange());

        juce::Path arrow;
        const float cx = area.getRight() - 16.0f;
        const float cy = area.getCentreY() + 1.0f;
        arrow.startNewSubPath (cx - 4.5f, cy - 3.0f);
        arrow.lineTo (cx, cy + 2.0f);
        arrow.lineTo (cx + 4.5f, cy - 3.0f);
        g.setColour (UITheme::accentOrange());
        g.strokePath (arrow, juce::PathStrokeType (1.8f));
    }

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour&, bool isMouseOverButton,
                               bool isButtonDown) override
    {
        auto area = button.getLocalBounds().toFloat().reduced (0.5f);
        UITheme::fillPanel (g, area);

        if (button.getToggleState())
            UITheme::drawLED (g, area.reduced (12.0f, 0.0f).removeFromBottom (3.0f), true, UITheme::accentOrange());
        else if (isMouseOverButton)
            UITheme::drawLED (g, area.reduced (14.0f, 0.0f).removeFromBottom (2.0f), true, UITheme::accentOrange().withAlpha (0.35f));

        if (isButtonDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.14f));
            g.fillRoundedRectangle (area, UITheme::cornerRadius());
        }
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return UITheme::fontLabel();
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool,
                         bool isButtonDown) override
    {
        g.setColour (UITheme::textMain().withAlpha (isButtonDown ? 0.78f : 1.0f));
        g.setFont (UITheme::fontLabel());
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }

    juce::Font getPopupMenuFont() override
    {
        return UITheme::fontLabel();
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        UITheme::fillPanel (g, juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height));
    }

    void drawPopupMenuItem (juce::Graphics& g,
                            const juce::Rectangle<int>& area,
                            bool isSeparator,
                            bool isActive,
                            bool isHighlighted,
                            bool,
                            bool,
                            const juce::String& text,
                            const juce::String&, const juce::Drawable*,
                            const juce::Colour* textColourToUse) override
    {
        if (isSeparator)
        {
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.drawHorizontalLine (area.getCentreY(), (float) area.getX() + 8.0f, (float) area.getRight() - 8.0f);
            return;
        }

        auto row = area.toFloat().reduced (4.0f, 1.0f);
        if (isHighlighted)
            UITheme::fillInset (g, row);

        g.setColour (textColourToUse != nullptr ? *textColourToUse
                                                : (isActive ? UITheme::textMain() : UITheme::textDim()));
        g.setFont (UITheme::fontLabel());
        g.drawText (text, area.reduced (12, 0), juce::Justification::centredLeft, false);
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
        const auto centre = bounds.getCentre();
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;

        g.setColour (juce::Colours::black.withAlpha (0.34f));
        g.fillEllipse (bounds.translated (0.0f, 5.0f));

        juce::ColourGradient outer (juce::Colour::fromRGB (54, 61, 71), centre.x, bounds.getY(),
                                    juce::Colour::fromRGB (20, 24, 31), centre.x, bounds.getBottom(), false);
        outer.addColour (0.5, juce::Colour::fromRGB (35, 39, 47));
        g.setGradientFill (outer);
        g.fillEllipse (bounds);

        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawEllipse (bounds, 1.0f);

        juce::Path arc;
        const auto angle = juce::jmap (sliderPosProportional, rotaryStartAngle, rotaryEndAngle);
        arc.addCentredArc (centre.x, centre.y, radius * 0.96f, radius * 0.96f, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (juce::Colour::fromRGB (255, 107, 53).withAlpha(0.82f));
        g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto p1 = centre.getPointOnCircumference (radius * 0.18f, angle);
        auto p2 = centre.getPointOnCircumference (radius * 0.70f, angle);
        g.setColour (juce::Colour::fromRGB (255, 241, 215));
        g.drawLine ({ p1, p2 }, 3.1f);
    }
};

struct FxModuleDef
{
    const char* label;
    const char* summary;
    const char* enableParamId;
    std::array<const char*, 4> paramIds;
    juce::Colour accent;
};

inline FxModuleDef makeFxModule(
    const char* label, const char* summary,
    const char* enableParamId,
    std::array<const char*, 4> paramIds,
    juce::Colour accent)
{
    return { label, summary, enableParamId, paramIds, accent };
}

const std::array<FxModuleDef, 8> kFxModules =
{
    makeFxModule("SATURATOR", "Harmonic warmth",            "fx_saturator_on",  {"fx_saturator_drive", "fx_saturator_tone", "fx_saturator_mix", "fx_saturator_clip"},           juce::Colour::fromRGB(255, 107, 53)),
    makeFxModule("TRANSIENT", "Snappy attack shaping",     "fx_transient_on",  {"fx_transient_attack", "fx_transient_decay", "fx_transient_sustain", "fx_transient_volume"},     juce::Colour::fromRGB(255, 184, 77)),
    makeFxModule("COMPRESSOR", "Dynamic control",          "fx_comp_on",       {"fx_comp_threshold", "fx_comp_ratio", "fx_comp_attack", "fx_comp_release"},           juce::Colour::fromRGB(255, 61, 61)),
    makeFxModule("EQ",         "Sculpt your tone",         "fx_eq_on",         {"fx_eq_low", "fx_eq_mid", "fx_eq_high", "fx_eq_q"},                                   juce::Colour::fromRGB(61, 255, 127)),
    makeFxModule("CHORUS",    "Wide modulation",           "fx_chorus_on",     {"fx_chorus_rate", "fx_chorus_depth", "fx_chorus_mix", "fx_chorus_delay"},           juce::Colour::fromRGB(74, 158, 255)),
    makeFxModule("DELAY",     "Echo with character",       "fx_delay_on",      {"fx_delay_time", "fx_delay_feedback", "fx_delay_mix", "fx_delay_filter"},           juce::Colour::fromRGB(200, 100, 255)),
    makeFxModule("REVERB",    "Spatial depth",            "fx_reverb_on",     {"fx_reverb_size", "fx_reverb_decay", "fx_reverb_mix", "fx_reverb_preDelay"},         juce::Colour::fromRGB(61, 220, 180)),
    makeFxModule("LIMITER",   "Protect your output",      "fx_limiter_on",    {"fx_limiter_threshold", "fx_limiter_release", "fx_limiter_ceiling", "fx_limiter_knee"}, juce::Colour::fromRGB(255, 230, 77))
};
}

// =============================================================================
DrumSynthAudioProcessorEditor::DrumSynthAudioProcessorEditor(DrumSynthAudioProcessor& processor)
    : AudioProcessorEditor(&processor), proc(processor)
{
    setOpaque(true);
    editorLookAndFeel = std::make_unique<EditorLookAndFeelHW>();
    setLookAndFeel(editorLookAndFeel.get());

    presetBox.setTextWhenNothingSelected("Select kit");
    familyFilterBox.setTextWhenNothingSelected("All families");
    prevBtn.setButtonText("<");
    nextBtn.setButtonText(">");
    addAndMakeVisible(presetBox);
    addAndMakeVisible(familyFilterBox);
    addAndMakeVisible(prevBtn);
    addAndMakeVisible(nextBtn);
    addAndMakeVisible(utilityDrawerBtn);
    addAndMakeVisible(masterGainKnob);

    for (int i = 0; i < kNumPads; ++i)
    {
        pads[i].configure(i, juce::String(mds::makePadName(i)), padCatColour(i));
        pads[i].onClicked = [this](int idx) { selectPad(idx); };
        addAndMakeVisible(pads[i]);
    }

    addAndMakeVisible(levelKnob);
    addAndMakeVisible(tuneKnob);
    addAndMakeVisible(decayKnob);
    addAndMakeVisible(attackKnob);
    addAndMakeVisible(pitchDropKnob);
    addAndMakeVisible(pitchDecayKnob);
    addAndMakeVisible(noiseKnob);
    addAndMakeVisible(clickKnob);
    addAndMakeVisible(driveKnob);
    addAndMakeVisible(cutoffKnob);

    addAndMakeVisible(macroPunch);
    addAndMakeVisible(macroWeight);
    addAndMakeVisible(macroAir);
    addAndMakeVisible(macroDirt);

    fxModuleSelector.clear(juce::dontSendNotification);
    for (int i = 0; i < (int) kFxModules.size(); ++i)
        fxModuleSelector.addItem(kFxModules[(std::size_t) i].label, i + 1);
    fxModuleSelector.setSelectedId(1, juce::dontSendNotification);
    addAndMakeVisible(fxModuleSelector);
    addAndMakeVisible(fxEnableBtn);
    addAndMakeVisible(fxParam1Knob);
    addAndMakeVisible(fxParam2Knob);
    addAndMakeVisible(fxParam3Knob);
    addAndMakeVisible(fxParam4Knob);

    addAndMakeVisible(mainMeter);
    addAndMakeVisible(singleModeBtn);

    addAndMakeVisible(velToClickKnob);
    addAndMakeVisible(reverbSendKnob);
    addAndMakeVisible(delaySendKnob);
    addAndMakeVisible(envDisplay);
    padPresetBox.setTextWhenNothingSelected("Factory Pad Preset");
    addAndMakeVisible(padPresetBox);
    padOutputBox.addItem("Master", 1);
    for (int b = 1; b <= DrumSynthAudioProcessor::kNumAuxOutputs; ++b)
        padOutputBox.addItem("Out " + juce::String(b), b + 1);
    padOutputBox.setTextWhenNothingSelected("Routing");
    addAndMakeVisible(padOutputBox);

    addAndMakeVisible(delaySyncBtn);
    delayNoteDivBox.addItemList({ "1/4", "1/8", "1/16", "dotted 1/8", "triplet 1/8" }, 1);
    delayNoteDivBox.setTextWhenNothingSelected("Division");
    addAndMakeVisible(delayNoteDivBox);

    presetBox.onChange = [this]
    {
        if (presetUiRefreshing) return;
        const int idx = presetBox.getSelectedItemIndex();
        if (idx < 0 || idx >= visiblePresetEntries.size()) return;
        const auto& entry = visiblePresetEntries.getReference(idx);
        if (entry.isFactory)
            proc.applyFactoryPreset(entry.factoryIndex);
        else if (entry.presetFile.existsAsFile())
            proc.loadUserPreset(entry.presetFile);
        refreshPresetSelectionFromProcessor();
        refreshPadPresets();
        refreshPadSelection();
        rebindPadRoutingForSelected();
        refreshEnvelopeDisplay();
    };
    familyFilterBox.onChange = [this] { refreshPresetList(); };
    prevBtn.onClick = [this] { navigateVisiblePreset(-1); };
    nextBtn.onClick = [this] { navigateVisiblePreset(1); };
    utilityDrawerBtn.onClick = [this]
    {
        utilityDrawerOpen = !utilityDrawerOpen;
        utilityDrawerBtn.setButtonText(utilityDrawerOpen ? "Hide Utility" : "Utility");
        repaint();
    };
    fxModuleSelector.onChange = [this]
    {
        selectedFxModule = juce::jlimit(0, (int) kFxModules.size() - 1, fxModuleSelector.getSelectedItemIndex());
        refreshFxModuleUi();
    };
    padPresetBox.onChange = [this]
    {
        const int idx = padPresetBox.getSelectedItemIndex();
        if (idx > 0)
            proc.applyFactoryPadPreset(selectedPadFromParam(), idx - 1);
        refreshPadPresets();
        refreshEnvelopeDisplay();
    };

    auto& apvts = proc.getAPVTS();
    singleNoteAttach = std::make_unique<ButtonAttach>(apvts, "singleNoteMode", singleModeBtn);
    levelAttach = std::make_unique<SliderAttach>(apvts, "level", levelKnob.getSlider());
    tuneAttach = std::make_unique<SliderAttach>(apvts, "tune", tuneKnob.getSlider());
    decayAttach = std::make_unique<SliderAttach>(apvts, "decay", decayKnob.getSlider());
    attackAttach = std::make_unique<SliderAttach>(apvts, "attack", attackKnob.getSlider());
    pitchDropAttach = std::make_unique<SliderAttach>(apvts, "pitchDrop", pitchDropKnob.getSlider());
    pitchDecayAttach = std::make_unique<SliderAttach>(apvts, "pitchDecay", pitchDecayKnob.getSlider());
    noiseAttach = std::make_unique<SliderAttach>(apvts, "noise", noiseKnob.getSlider());
    clickAttach = std::make_unique<SliderAttach>(apvts, "click", clickKnob.getSlider());
    driveAttach = std::make_unique<SliderAttach>(apvts, "drive", driveKnob.getSlider());
    cutoffAttach = std::make_unique<SliderAttach>(apvts, "cutoff", cutoffKnob.getSlider());
    macroPunchAttach = std::make_unique<SliderAttach>(apvts, "macroPunch", macroPunch.getSlider());
    macroWeightAttach = std::make_unique<SliderAttach>(apvts, "macroWeight", macroWeight.getSlider());
    macroAirAttach = std::make_unique<SliderAttach>(apvts, "macroAir", macroAir.getSlider());
    macroDirtAttach = std::make_unique<SliderAttach>(apvts, "macroDirt", macroDirt.getSlider());
    masterGainAttach = std::make_unique<SliderAttach>(apvts, "masterGain", masterGainKnob.getSlider());
    delaySyncAttach = std::make_unique<ButtonAttach>(apvts, "delaySyncOn", delaySyncBtn);
    delayNoteDivAttach = std::make_unique<ComboBoxAttach>(apvts, "delayNoteDiv", delayNoteDivBox);

    refreshPresetList();
    refreshFxModuleUi();
    refreshPadSelection();
    refreshPadPresets();
    rebindPadRoutingForSelected();
    refreshEnvelopeDisplay();

    setSize(kDefaultW, kDefaultH);
    startTimerHz(30);
}

DrumSynthAudioProcessorEditor::~DrumSynthAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

// =============================================================================
// PAINT
// =============================================================================
void DrumSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    drawBackground(g);

    const auto selPad = selectedPadFromParam();
    const auto selCol = padCatColour(selPad);

    drawSectionPanel(g, padZoneBounds, "PAD BANK", UITheme::accentCyan());
    drawSectionPanel(g, voiceZoneBounds, "VOICE / " + juce::String(padCatName(selPad)), selCol);
    {
        const auto& fxMod = kFxModules[(std::size_t) juce::jlimit(0, (int) kFxModules.size() - 1, selectedFxModule)];
        drawSectionPanel(g, fxZoneBounds, juce::String("FX / ") + fxMod.label, fxMod.accent);
    }

    // Header bar
    {
        auto hb = headerBounds.toFloat();
        UITheme::fillPanel(g, hb, UITheme::bgElevated(), 0.0f);
        g.setColour(UITheme::accentOrange().withAlpha(0.28f));
        g.drawHorizontalLine((int)hb.getBottom() - 1, hb.getX(), hb.getRight());

        auto logo = hb.reduced(14.0f, 0.0f);
        g.setColour(UITheme::textMain());
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 16.0f, juce::Font::bold));
        g.drawText("DRUM SYNTH", logo, juce::Justification::centredLeft);

        g.setColour(UITheme::accentOrange().withAlpha(0.65f));
        g.setFont(UITheme::fontMicro().withHeight(8.0f));
        g.drawText("V5", logo.translated(96.0f, 6.0f), juce::Justification::centredLeft);
    }

    // Footer
    {
        auto fb = footerBounds.toFloat();
        g.setColour(UITheme::textDark());
        g.setFont(UITheme::fontMicro());
        g.drawText("UWdeVST  \u2014  MDS DRUM ENGINE", fb.reduced(12.0f, 0.0f),
                   juce::Justification::centredRight);
    }

    // Selected pad family badge overlay on pad zone
    {
        auto badgeArea = padZoneBounds.toFloat().removeFromBottom(28.0f).reduced(12.0f, 0.0f);
        drawFamilyBadge(g, badgeArea.getCentre(), padCatName(selPad), selCol);
    }
}

void DrumSynthAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    g.fillAll(UITheme::bgDeep());

    const float W = (float)getWidth(), H = (float)getHeight();
    juce::ColourGradient radial(juce::Colour::fromRGB(28, 24, 38).withAlpha(0.55f),
                                W * 0.5f, H * 0.22f,
                                juce::Colours::transparentBlack,
                                W * 0.5f, H * 0.78f, true);
    g.setGradientFill(radial);
    g.fillAll();

    g.setColour(juce::Colours::white.withAlpha(0.020f));
    for (int x = 20; x < getWidth(); x += 28)
        for (int y = 20; y < getHeight(); y += 28)
            g.fillEllipse((float)x - 0.5f, (float)y - 0.5f, 1.0f, 1.0f);
}

void DrumSynthAudioProcessorEditor::drawSectionPanel(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                    const juce::String& title, juce::Colour accentCol)
{
    UITheme::drawSectionCard(g, area.toFloat(), title, accentCol);
}

void DrumSynthAudioProcessorEditor::drawFamilyBadge(juce::Graphics& g, juce::Point<float> centre,
                                                     const juce::String& text, juce::Colour col)
{
    const float w = 70.0f, h = 18.0f;
    auto area = juce::Rectangle<float>(w, h).withCentre(centre);
    g.setColour(col.withAlpha(0.18f));
    g.fillRoundedRectangle(area, h * 0.5f);
    g.setColour(col.withAlpha(0.70f));
    g.drawRoundedRectangle(area, h * 0.5f, 0.9f);
    g.setColour(col);
    g.setFont(UITheme::fontLabel());
    g.drawText(text, area, juce::Justification::centred, false);
}

// =============================================================================
// RESIZED — 3-zone layout
// =============================================================================
void DrumSynthAudioProcessorEditor::resized()
{
    const int M  = 10;
    const int G  = 8;
    const int HH = 52;
    const int FH = 20;

    auto full = getLocalBounds().reduced(M);

    headerBounds = full.removeFromTop(HH);
    full.removeFromTop(G);
    footerBounds = full.removeFromBottom(FH);
    full.removeFromBottom(G);

    {
        auto header = headerBounds.reduced(14, 6);
        header.removeFromLeft(132);

        prevBtn.setBounds(header.removeFromLeft(28));
        header.removeFromLeft(6);
        nextBtn.setBounds(header.removeFromLeft(28));
        header.removeFromLeft(8);

        const int masterW = juce::jlimit(58, 76, header.getWidth() / 7);
        auto masterArea = header.removeFromRight(masterW);
        masterGainKnob.setBounds(masterArea.reduced(0, 1));
        header.removeFromRight(8);

        singleModeBtn.setBounds(header.removeFromRight(76).reduced(0, 4));
        header.removeFromRight(8);
        utilityDrawerBtn.setBounds(header.removeFromRight(92).reduced(0, 4));
        header.removeFromRight(8);

        const int filterW = juce::jlimit(116, 160, header.getWidth() / 3);
        familyFilterBox.setBounds(header.removeFromRight(filterW).reduced(0, 4));
        header.removeFromRight(8);
        presetBox.setBounds(header.reduced(0, 4));
    }

    // Three zones: 40% | 35% | 25%
    const int totalW = full.getWidth();
    const int padW   = (int)(totalW * 0.40f);
    const int voiceW = (int)(totalW * 0.35f);

    auto padArea = full.removeFromLeft(padW);
    full.removeFromLeft(G);
    auto voiceArea = full.removeFromLeft(voiceW);
    full.removeFromLeft(G);
    fxZoneBounds = full;

    padZoneBounds = padArea;
    voiceZoneBounds = voiceArea;

    // ---- Pad zone: 4×3 grid + envelope display ----
    {
        auto area = padZoneBounds.reduced(G);
        area.removeFromTop(20);

        const int envH = 70;
        envBounds = area.removeFromBottom(envH);
        area.removeFromBottom(G);

        const int cols = 4, rows = 3, gap = 8;
        const int pw = (area.getWidth()  - gap * (cols - 1)) / cols;
        const int ph = (area.getHeight() - gap * (rows - 1)) / rows;
        for (int i = 0; i < kNumPads; ++i)
            pads[i].setBounds(area.getX() + (i % cols) * (pw + gap),
                              area.getY() + (i / cols) * (ph + gap),
                              pw, ph);
    }

    // Envelope display
    envDisplay.setBounds(envBounds.reduced(4, 2));

    // ---- Voice zone: MIX (2×3) | TIMBRE (1×4) | MACROS (1×4) ----
    {
        auto area = voiceZoneBounds.reduced(G);
        area.removeFromTop(20);

        // MIX group (LEVEL, TUNE, ATTACK, DECAY, PITCH DROP, PITCH DECAY)
        {
            auto mixArea = area.removeFromTop((int)(area.getHeight() * 0.48f));
            area.removeFromTop(G);
            const int cols = 3, rows = 2, gap = 6;
            const int kw = (mixArea.getWidth()  - gap * (cols - 1)) / cols;
            const int kh = (mixArea.getHeight() - gap * (rows - 1)) / rows;
            std::array<DrumKnob*, 6> mk = { &levelKnob, &tuneKnob, &attackKnob, &decayKnob, &pitchDropKnob, &pitchDecayKnob };
            for (int i = 0; i < 6; ++i)
                mk[(std::size_t)i]->setBounds(mixArea.getX() + (i % cols) * (kw + gap),
                                              mixArea.getY() + (i / cols) * (kh + gap),
                                              kw, kh);
        }

        // TIMBRE group (NOISE, CLICK, DRIVE, CUTOFF) — 1 row × 4
        {
            auto timbreArea = area.removeFromTop((int)(area.getHeight() * 0.50f));
            area.removeFromTop(G);
            const int cols = 4, gap = 6;
            const int kw = (timbreArea.getWidth() - gap * (cols - 1)) / cols;
            std::array<DrumKnob*, 4> tk = { &noiseKnob, &clickKnob, &driveKnob, &cutoffKnob };
            for (int i = 0; i < 4; ++i)
                tk[(std::size_t)i]->setBounds(timbreArea.getX() + i * (kw + gap),
                                              timbreArea.getY(), kw, timbreArea.getHeight());
        }

        // MACROS group — 4 knobs horizontal
        {
            const int cols = 4, gap = 6;
            const int kw = (area.getWidth() - gap * (cols - 1)) / cols;
            std::array<DrumKnob*, 4> mk = { &macroPunch, &macroWeight, &macroAir, &macroDirt };
            for (int i = 0; i < 4; ++i)
                mk[(std::size_t)i]->setBounds(area.getX() + i * (kw + gap),
                                              area.getY(), kw, area.getHeight());
        }
    }

    // ---- FX zone: selector + enable + 4 knobs + inspector ----
    {
        auto area = fxZoneBounds.reduced(G);
        area.removeFromTop(20);

        // FX module selector + enable
        auto fxTopRow = area.removeFromTop(28);
        fxModuleSelector.setBounds(fxTopRow.removeFromLeft(fxTopRow.getWidth() - 80));
        fxTopRow.removeFromLeft(G);
        fxEnableBtn.setBounds(fxTopRow.reduced(0, 2));
        area.removeFromTop(G);

        // FX knobs: 2x2 grid, capped so the inspector remains testable at 1280x760.
        {
            auto knobArea = area.removeFromTop(juce::jlimit(150, 260, area.getHeight() - 120));
            const int cols = 2, rows = 2, gap = 6;
            const int kw = (knobArea.getWidth()  - gap * (cols - 1)) / cols;
            const int kh = (knobArea.getHeight() - gap * (rows - 1)) / rows;
            std::array<DrumKnob*, 4> fk = { &fxParam1Knob, &fxParam2Knob, &fxParam3Knob, &fxParam4Knob };
            for (int i = 0; i < 4; ++i)
                fk[(std::size_t)i]->setBounds(knobArea.getX() + (i % cols) * (kw + gap),
                                              knobArea.getY() + (i / cols) * (kh + gap),
                                              kw, kh);
            area.removeFromTop(G);
        }

        // Inspector section
        {
            auto inspArea = area;
            if (inspArea.getHeight() < 100) return;

            inspectorBounds = inspArea;

            auto presetRow = inspArea.removeFromTop(26);
            padPresetBox.setBounds(presetRow.removeFromLeft((presetRow.getWidth() * 3) / 5));
            presetRow.removeFromLeft(G);
            padOutputBox.setBounds(presetRow);
            inspArea.removeFromTop(G);

            auto sendRow = inspArea.removeFromTop(50);
            const int kw3 = (sendRow.getWidth() - G * 2) / 3;
            velToClickKnob.setBounds(sendRow.removeFromLeft(kw3));
            sendRow.removeFromLeft(G);
            reverbSendKnob.setBounds(sendRow.removeFromLeft(kw3));
            sendRow.removeFromLeft(G);
            delaySendKnob.setBounds(sendRow);
            inspArea.removeFromTop(G);

            auto delayRow = inspArea.removeFromTop(26);
            delaySyncBtn.setBounds(delayRow.removeFromLeft(72).reduced(0, 2));
            delayRow.removeFromLeft(G);
            delayNoteDivBox.setBounds(delayRow.reduced(0, 2));
            inspArea.removeFromTop(G);

            mainMeter.setBounds(inspArea);
        }
    }
}

// =============================================================================
// TIMER
// =============================================================================
void DrumSynthAudioProcessorEditor::timerCallback()
{
    for (int i = 0; i < kNumPads; ++i)
    {
        const float v = proc.consumePadTriggerActivity(i);
        if (v > 0.0f)
        {
            pads[i].setActivityLevel(v);
            pads[i].flash();
        }
    }
    for (auto& p : pads) p.tickFlash();

    mainMeter.setLevels(proc.getMainPeakMeter(), proc.getAuxPeakMeter());

    const int liveSelectedPad = selectedPadFromParam();
    if (liveSelectedPad != selectedPadIdx)
    {
        selectedPadIdx = liveSelectedPad;
        rebindPadRoutingForSelected();
        refreshPadPresets();
    }

    refreshPadSelection();
    refreshPresetSelectionFromProcessor();
    refreshEnvelopeDisplay();
}

// =============================================================================
// PAD SELECTION
// =============================================================================
void DrumSynthAudioProcessorEditor::selectPad(int idx)
{
    if (idx < 0 || idx >= kNumPads) return;
    selectedPadIdx = idx;
    if (auto* selectedPadParam = proc.getAPVTS().getParameter("selected_pad"))
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(selectedPadParam))
            selectedPadParam->setValueNotifyingHost(ranged->convertTo0to1((float) idx));
    refreshPadSelection();
    rebindPadRoutingForSelected();
    refreshEnvelopeDisplay();
    refreshFxModuleUi();
    repaint();
}

int DrumSynthAudioProcessorEditor::selectedPadFromParam() const
{
    if (auto* value = proc.getAPVTS().getRawParameterValue("selected_pad"))
        return juce::jlimit(0, kNumPads - 1, static_cast<int>(value->load() + 0.5f));
    return 0;
}

void DrumSynthAudioProcessorEditor::refreshPadSelection()
{
    int sel = selectedPadFromParam();
    selectedPadIdx = sel;
    for (int i = 0; i < kNumPads; ++i) pads[i].setSelected(i == sel);
}

juce::Colour DrumSynthAudioProcessorEditor::padCatColour(int i) const
{
    switch (i / 3)
    {
        case 0: return UITheme::accentOrange();
        case 1: return juce::Colour::fromRGB(255, 184, 77);
        case 2: return UITheme::accentCyan();
        case 3: return juce::Colour::fromRGB(100, 180, 255);
        default: return juce::Colour::fromRGB(180, 100, 220);
    }
}

const char* DrumSynthAudioProcessorEditor::padCatName(int i) const
{
    switch (i / 3)
    {
        case 0: return "KICK";
        case 1: return "SNARE";
        case 2: return "HIHAT";
        case 3: return "TOM";
        default: return "FX";
    }
}

// =============================================================================
// PRESETS
// =============================================================================
void DrumSynthAudioProcessorEditor::refreshPresetList()
{
    const auto selectedFamily = familyFilterBox.getText();
    presetUiRefreshing = true;

    visiblePresetEntries.clear();
    presetBox.clear(juce::dontSendNotification);

    const auto families = proc.getPresetFamilyChoices();
    familyFilterBox.clear(juce::dontSendNotification);
    familyFilterBox.addItem("All Families", 1);
    for (int i = 0; i < families.size(); ++i)
        familyFilterBox.addItem(families[i], i + 2);

    int familyId = 1;
    for (int i = 1; i < familyFilterBox.getNumItems(); ++i)
    {
        if (familyFilterBox.getItemText(i) == selectedFamily)
        {
            familyId = familyFilterBox.getItemId(i);
            break;
        }
    }
    familyFilterBox.setSelectedId(familyId, juce::dontSendNotification);

    const auto entries = proc.scanPresetLibrary();
    for (const auto& entry : entries)
    {
        const bool familyMatches = selectedFamily.isEmpty()
            || selectedFamily == "All Families"
            || entry.familyLabel == selectedFamily;

        if (! familyMatches) continue;

        visiblePresetEntries.add(entry);
        auto label = entry.name;
        if (entry.familyLabel.isNotEmpty())
            label += "  |  " + entry.familyLabel;
        if (! entry.isFactory)
            label = "[USER] " + label;
        presetBox.addItem(label, visiblePresetEntries.size());
    }

    presetUiRefreshing = false;
    presetBox.setEnabled(visiblePresetEntries.size() > 0);
    prevBtn.setEnabled(visiblePresetEntries.size() > 1);
    nextBtn.setEnabled(visiblePresetEntries.size() > 1);
    refreshPresetSelectionFromProcessor();
}

void DrumSynthAudioProcessorEditor::refreshPresetSelectionFromProcessor()
{
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
    {
        presetUiRefreshing = true;
        presetBox.setSelectedItemIndex(targetIndex, juce::dontSendNotification);
        presetUiRefreshing = false;
    }
}

void DrumSynthAudioProcessorEditor::navigateVisiblePreset(int delta)
{
    if (visiblePresetEntries.size() <= 1) return;
    int idx = presetBox.getSelectedItemIndex();
    if (idx < 0) idx = 0;
    idx = (idx + delta + visiblePresetEntries.size()) % visiblePresetEntries.size();
    presetBox.setSelectedItemIndex(idx, juce::sendNotificationSync);
}

// =============================================================================
// PAD PRESETS
// =============================================================================
void DrumSynthAudioProcessorEditor::refreshPadPresets()
{
    const int padIndex = selectedPadFromParam();
    const auto currentIdx = proc.getCurrentPadPresetIndex(padIndex);
    const auto factoryNames = proc.getFactoryPadPresetNames(padIndex);

    padPresetBox.clear(juce::dontSendNotification);
    padPresetBox.addItem("Custom", 1);
    for (int i = 0; i < factoryNames.size(); ++i)
        padPresetBox.addItem(factoryNames[i], i + 2);

    const int selectedId = currentIdx >= 0 ? currentIdx + 2 : 1;
    padPresetBox.setSelectedId(selectedId, juce::dontSendNotification);
}

// =============================================================================
// ROUTING / FX
// =============================================================================
void DrumSynthAudioProcessorEditor::rebindPadRoutingForSelected()
{
    const int sel = selectedPadFromParam();
    auto& apvts = proc.getAPVTS();

    velToClickAttach.reset();
    reverbSendAttach.reset();
    delaySendAttach.reset();
    padOutputAttach.reset();

    velToClickAttach = std::make_unique<SliderAttach>(
        apvts, "pad" + juce::String(sel) + "_velToClick", velToClickKnob.getSlider());
    reverbSendAttach = std::make_unique<SliderAttach>(
        apvts, "pad" + juce::String(sel) + "_reverbSend", reverbSendKnob.getSlider());
    delaySendAttach = std::make_unique<SliderAttach>(
        apvts, "pad" + juce::String(sel) + "_delaySend", delaySendKnob.getSlider());
    padOutputAttach = std::make_unique<ComboBoxAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "output"), padOutputBox);
}

void DrumSynthAudioProcessorEditor::refreshFxModuleUi()
{
    selectedFxModule = juce::jlimit(0, (int) kFxModules.size() - 1,
                                    fxModuleSelector.getSelectedItemIndex());

    const auto& fx = kFxModules[(std::size_t) selectedFxModule];
    fxEnableBtn.setButtonText("MODULE");

    fxEnableAttach.reset();
    if (fx.enableParamId != nullptr)
        fxEnableAttach = std::make_unique<ButtonAttach>(proc.getAPVTS(), fx.enableParamId, fxEnableBtn);

    std::array<DrumKnob*, 4> knobs = { &fxParam1Knob, &fxParam2Knob, &fxParam3Knob, &fxParam4Knob };
    for (int i = 0; i < 4; ++i)
    {
        fxParamAttach[(std::size_t) i].reset();
        const char* pid = fx.paramIds[(std::size_t) i];
        if (pid == nullptr)
        {
            knobs[(std::size_t) i]->setVisible(false);
            continue;
        }
        knobs[(std::size_t) i]->setVisible(true);
        fxParamAttach[(std::size_t) i] = std::make_unique<SliderAttach>(
            proc.getAPVTS(), pid, knobs[(std::size_t) i]->getSlider());
    }

    repaint();
}

// =============================================================================
// ENVELOPE
// =============================================================================
void DrumSynthAudioProcessorEditor::refreshEnvelopeDisplay()
{
    auto& apvts = proc.getAPVTS();
    const int sel = selectedPadFromParam();
    auto fetch = [&apvts, sel](const char* suffix, float fallback)
    {
        const auto id = "pad" + juce::String(sel) + "_" + suffix;
        if (auto* p = apvts.getRawParameterValue(id)) return p->load();
        return fallback;
    };
    const float a  = fetch("attack", 0.01f);
    const float d  = fetch("decay", 0.5f);
    const float pd = fetch("pitchDecay", 0.1f);
    const float n  = fetch("noise", 0.0f);
    envDisplay.updateFromADSR(juce::jlimit(0.0f, 1.0f, a / 2.0f),
                               juce::jlimit(0.0f, 1.0f, d / 5.0f),
                               juce::jlimit(0.0f, 1.0f, n),
                               juce::jlimit(0.0f, 1.0f, pd / 2.0f));
}
