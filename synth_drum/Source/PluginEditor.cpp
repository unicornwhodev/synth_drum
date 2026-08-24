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

        juce::ColourGradient outer (UITheme::panelHover(), centre.x, bounds.getY(),
                                    UITheme::bgBase(), centre.x, bounds.getBottom(), false);
        outer.addColour (0.5, UITheme::panelBase());
        g.setGradientFill (outer);
        g.fillEllipse (bounds);

        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawEllipse (bounds, 1.0f);

        juce::Path arc;
        const auto angle = juce::jmap (sliderPosProportional, rotaryStartAngle, rotaryEndAngle);
        arc.addCentredArc (centre.x, centre.y, radius * 0.96f, radius * 0.96f, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (UITheme::accentOrange().withAlpha(0.82f));
        g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto p1 = centre.getPointOnCircumference (radius * 0.18f, angle);
        auto p2 = centre.getPointOnCircumference (radius * 0.70f, angle);
        g.setColour (UITheme::textMain());
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
    makeFxModule("SATURATOR",  "Harmonic warmth",        "fx_saturator_en", {"sat_drive", "sat_mix", nullptr, nullptr},                                      UITheme::accentOrange()),
    makeFxModule("TRANSIENT",  "Snappy attack shaping",  "fx_transient_en", {"transient_attack", "transient_sustain", "transient_mix", nullptr},              UITheme::accentAmber()),
    makeFxModule("COMPRESSOR", "Dynamic control",        "fx_comp_en",      {"comp_threshold", "comp_ratio", "comp_attack", "comp_release"},                  UITheme::accentRed()),
    makeFxModule("EQ",         "Sculpt your tone",       "fx_eq_en",        {"eq_low_gain", "eq_mid_gain", "eq_high_gain", "eq_mid_q"},                       UITheme::accentGreen()),
    makeFxModule("CHORUS",     "Wide modulation",        "fx_chorus_en",    {"chorus_rate", "chorus_depth", "chorus_mix", nullptr},                          UITheme::accentCyan()),
    makeFxModule("DELAY",      "Echo with character",    "fx_delay_en",     {"delay_time", "delay_feedback", "delay_mix", nullptr},                          UITheme::accentPurple()),
    makeFxModule("REVERB",     "Spatial depth",          "fx_reverb_en",    {"reverb_size", "reverb_damping", "reverb_mix", "reverb_predelay"},               UITheme::accentTeal()),
    makeFxModule("LIMITER",    "Protect your output",    "fx_limiter_en",   {"limiter_threshold", "limiter_release", nullptr, nullptr},                      UITheme::accentAmber())
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
    singleNoteAttach = std::make_unique<ButtonAttach>(apvts, "single_note_mode", singleModeBtn);
    macroPunchAttach = std::make_unique<SliderAttach>(apvts, "macro_punch", macroPunch.getSlider());
    macroWeightAttach = std::make_unique<SliderAttach>(apvts, "macro_weight", macroWeight.getSlider());
    macroAirAttach = std::make_unique<SliderAttach>(apvts, "macro_air", macroAir.getSlider());
    macroDirtAttach = std::make_unique<SliderAttach>(apvts, "macro_dirt", macroDirt.getSlider());
    masterGainAttach = std::make_unique<SliderAttach>(apvts, "output_gain", masterGainKnob.getSlider());
    delaySyncAttach = std::make_unique<ButtonAttach>(apvts, "delay_sync", delaySyncBtn);
    delayNoteDivAttach = std::make_unique<ComboBoxAttach>(apvts, "delay_note_div", delayNoteDivBox);

    refreshPresetList();
    refreshFxModuleUi();
    refreshPadSelection();
    refreshPadPresets();
    rebindPadRoutingForSelected();
    refreshEnvelopeDisplay();

    setResizable(true, true);
    setResizeLimits(kMinW, kMinH, 2560, 1600);
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
        drawGroupFrame(g, fxKnobGroupBounds, "MODULE CONTROLS", fxMod.accent);
        drawGroupFrame(g, inspectorBounds, "PAD ROUTING / SENDS", UITheme::accentGreen());
    }

    drawGroupFrame(g, padGridBounds, "TRIGGER PADS", UITheme::accentCyan());
    drawGroupFrame(g, envBounds, "ENVELOPE SNAPSHOT", selCol);
    drawGroupFrame(g, voiceCoreBounds, "CORE", selCol);
    drawGroupFrame(g, voiceTimbreBounds, "TIMBRE", UITheme::accentCyan());
    drawGroupFrame(g, voiceMacroBounds, "MACROS", UITheme::accentGreen());

    // Header bar
    {
        auto hb = headerBounds.toFloat();
        UITheme::fillPanel(g, hb, UITheme::bgElevated().interpolatedWith(selCol, 0.05f), UITheme::cornerRadius());
        g.setColour(selCol.withAlpha(0.28f));
        g.drawHorizontalLine((int) hb.getBottom() - 1, hb.getX() + 10.0f, hb.getRight() - 10.0f);

        auto logo = hb.reduced(18.0f, 0.0f);
        logo.setWidth(getWidth() < 1080 ? 154.0f : 194.0f);
        auto titleRow = logo.removeFromTop(hb.getHeight() * 0.55f);
        auto badge = titleRow.removeFromRight(34.0f).withSizeKeepingCentre(34.0f, 18.0f);

        g.setColour(UITheme::textMain());
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(getWidth() < 1080 ? 14.0f : 16.0f).withStyle("Bold")));
        g.drawText("UWdeVST DRUM", titleRow, juce::Justification::centredLeft);

        g.setColour(UITheme::textDim());
        g.setFont(UITheme::fontMicro().withHeight(8.5f));
        g.drawText("MDS DRUM ENGINE", logo, juce::Justification::centredLeft);

        g.setColour(selCol.withAlpha(0.15f));
        g.fillRoundedRectangle(badge, 4.0f);
        g.setColour(selCol.withAlpha(0.72f));
        g.drawRoundedRectangle(badge, 4.0f, 0.9f);
        g.setFont(UITheme::fontMicro().withHeight(8.0f));
        g.drawText("V5", badge, juce::Justification::centred);
    }

    // Footer
    {
        auto fb = footerBounds.toFloat();
        g.setColour(UITheme::textDark());
        g.setFont(UITheme::fontMicro());
        g.drawText("UWdeVST - MDS DRUM ENGINE", fb.reduced(12.0f, 0.0f),
                   juce::Justification::centredRight);
    }

    // Selected pad family badge in the pad bank header area.
    {
        auto badgeArea = padZoneBounds.toFloat().reduced(16.0f, 5.0f).removeFromTop(18.0f).removeFromRight(80.0f);
        drawFamilyBadge(g, badgeArea.getCentre(), padCatName(selPad), selCol);
    }
}

void DrumSynthAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    const float W = (float) getWidth();
    const float H = (float) getHeight();

    juce::ColourGradient base(UITheme::bgElevated(), 0.0f, 0.0f,
                              UITheme::bgDeep(), 0.0f, H, false);
    base.addColour(0.52, UITheme::bgBase());
    g.setGradientFill(base);
    g.fillAll();

    g.setColour(juce::Colours::white.withAlpha(0.018f));
    for (int y = 24; y < getHeight(); y += 28)
        g.drawHorizontalLine(y, 24.0f, W - 24.0f);

    g.setColour(juce::Colours::black.withAlpha(0.18f));
    g.fillRect(0, 0, getWidth(), 2);
    g.fillRect(0, getHeight() - 2, getWidth(), 2);
}

void DrumSynthAudioProcessorEditor::drawSectionPanel(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                    const juce::String& title, juce::Colour accentCol)
{
    UITheme::drawSectionCard(g, area.toFloat(), title, accentCol);
}

void DrumSynthAudioProcessorEditor::drawGroupFrame(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                   const juce::String& title, juce::Colour accentCol)
{
    if (area.getWidth() <= 4 || area.getHeight() <= 4)
        return;

    auto frame = area.toFloat();
    g.setColour(UITheme::panelInset().withAlpha(0.56f));
    g.fillRoundedRectangle(frame, UITheme::cornerRadiusSmall());

    g.setColour(juce::Colours::white.withAlpha(0.024f));
    g.fillRoundedRectangle(frame.reduced(1.0f).removeFromTop(1.0f), UITheme::cornerRadiusSmall());

    g.setColour(accentCol.withAlpha(0.18f));
    g.drawRoundedRectangle(frame.reduced(0.5f), UITheme::cornerRadiusSmall(), 0.9f);

    auto labelArea = frame.reduced(9.0f, 4.0f).removeFromTop(12.0f);
    g.setColour(accentCol.withAlpha(0.76f));
    g.setFont(UITheme::fontMicro().withHeight(8.4f));
    g.drawText(title, labelArea, juce::Justification::centredLeft, false);

    g.setColour(accentCol.withAlpha(0.18f));
    g.drawHorizontalLine((int) labelArea.getBottom() + 2, labelArea.getX(), frame.getRight() - 9.0f);
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
    const bool compact = getWidth() < 1080 || getHeight() < 740;
    const int M = compact ? 14 : 22;
    const int G = compact ? 10 : 14;
    const int headerH = compact ? 78 : 86;
    const int footerH = 18;
    const int sectionPad = compact ? 8 : 10;
    const int groupGap = juce::jmin(G, 8);

    auto shell = getLocalBounds().reduced(M);
    headerBounds = shell.removeFromTop(headerH);
    shell.removeFromTop(G);
    footerBounds = shell.removeFromBottom(footerH);
    shell.removeFromBottom(G);

    const int bodyW = juce::jmin(shell.getWidth(), compact ? 1180 : 1260);
    auto body = shell.withSizeKeepingCentre(bodyW, shell.getHeight());

    auto placeKnobs = [groupGap](const juce::Rectangle<int>& bounds, const auto& knobs, int cols)
    {
        if (bounds.isEmpty() || cols <= 0)
            return;

        auto area = bounds.reduced(8, 7);
        area.removeFromTop(16);

        const int count = (int) knobs.size();
        const int rows = juce::jmax(1, (count + cols - 1) / cols);
        const int kw = juce::jmax(1, (area.getWidth() - groupGap * (cols - 1)) / cols);
        const int kh = juce::jmax(1, (area.getHeight() - groupGap * (rows - 1)) / rows);

        for (int i = 0; i < count; ++i)
        {
            if (knobs[(std::size_t) i] == nullptr)
                continue;

            knobs[(std::size_t) i]->setBounds(area.getX() + (i % cols) * (kw + groupGap),
                                              area.getY() + (i / cols) * (kh + groupGap),
                                              kw, kh);
        }
    };

    // Header: brand | preset browser | utility/status controls.
    {
        auto header = headerBounds.reduced(compact ? 12 : 16, compact ? 8 : 10);
        header.removeFromLeft(compact ? 158 : 198);
        header.removeFromLeft(G);

        auto right = header.removeFromRight(compact ? 266 : 314);
        const int controlH = compact ? 28 : 30;
        const int masterSize = compact ? 54 : 60;

        masterGainKnob.setBounds(right.removeFromRight(masterSize).withSizeKeepingCentre(masterSize, right.getHeight()));
        right.removeFromRight(8);
        const int singleW = compact ? 84 : 92;
        singleModeBtn.setBounds(right.removeFromRight(singleW).withSizeKeepingCentre(singleW, controlH));
        right.removeFromRight(8);
        utilityDrawerBtn.setBounds(right.removeFromRight(compact ? 78 : 88).withSizeKeepingCentre(compact ? 78 : 88, controlH));
        header.removeFromRight(8);

        const int navW = compact ? 28 : 30;
        prevBtn.setBounds(header.removeFromLeft(navW).withSizeKeepingCentre(navW, controlH));
        header.removeFromLeft(4);
        nextBtn.setBounds(header.removeFromLeft(navW).withSizeKeepingCentre(navW, controlH));
        header.removeFromLeft(8);

        const int filterW = juce::jlimit(compact ? 120 : 142, compact ? 150 : 172, header.getWidth() / 4);
        familyFilterBox.setBounds(header.removeFromRight(filterW).withSizeKeepingCentre(filterW, controlH));
        header.removeFromRight(8);
        presetBox.setBounds(header.withSizeKeepingCentre(header.getWidth(), controlH));
    }

    // Main body: three equal columns, matching the Bass/Piano visual rhythm.
    const int colW = (body.getWidth() - G * 2) / 3;
    padZoneBounds = body.removeFromLeft(colW);
    body.removeFromLeft(G);
    voiceZoneBounds = body.removeFromLeft(colW);
    body.removeFromLeft(G);
    fxZoneBounds = body;

    // Pad bank: 3 x 4 pads + full-width envelope readout.
    {
        auto area = padZoneBounds.reduced(sectionPad);
        area.removeFromTop(compact ? 24 : 28);

        const int envH = compact ? 86 : 102;
        envBounds = area.removeFromBottom(envH);
        area.removeFromBottom(G);
        padGridBounds = area;

        auto grid = padGridBounds.reduced(8, 7);
        grid.removeFromTop(16);
        const int cols = 3;
        const int rows = 4;
        const int pw = juce::jmax(1, (grid.getWidth() - groupGap * (cols - 1)) / cols);
        const int ph = juce::jmax(1, (grid.getHeight() - groupGap * (rows - 1)) / rows);

        for (int i = 0; i < kNumPads; ++i)
            pads[(std::size_t) i].setBounds(grid.getX() + (i % cols) * (pw + groupGap),
                                            grid.getY() + (i / cols) * (ph + groupGap),
                                            pw, ph);

        envDisplay.setBounds(envBounds.reduced(8, 7).withTrimmedTop(16));
    }

    // Voice: core controls, timbre shaping, then macros.
    {
        auto area = voiceZoneBounds.reduced(sectionPad);
        area.removeFromTop(compact ? 24 : 28);

        const int availableH = area.getHeight();
        const int coreH = juce::jlimit(compact ? 190 : 220, compact ? 218 : 250,
                                       (int) (availableH * 0.40f));
        voiceCoreBounds = area.removeFromTop(coreH);
        area.removeFromTop(G);

        const int timbreH = juce::jlimit(compact ? 142 : 160, compact ? 168 : 188,
                                         (int) (area.getHeight() * 0.52f));
        voiceTimbreBounds = area.removeFromTop(timbreH);
        area.removeFromTop(G);
        voiceMacroBounds = area;

        std::array<DrumKnob*, 6> core = { &levelKnob, &tuneKnob, &attackKnob, &decayKnob, &pitchDropKnob, &pitchDecayKnob };
        std::array<DrumKnob*, 4> timbre = { &noiseKnob, &clickKnob, &driveKnob, &cutoffKnob };
        std::array<DrumKnob*, 4> macros = { &macroPunch, &macroWeight, &macroAir, &macroDirt };
        placeKnobs(voiceCoreBounds, core, 3);
        placeKnobs(voiceTimbreBounds, timbre, 2);
        placeKnobs(voiceMacroBounds, macros, 4);
    }

    // FX: module selector, 2 x 2 module controls, then selected-pad utility.
    {
        auto area = fxZoneBounds.reduced(sectionPad);
        area.removeFromTop(compact ? 24 : 28);

        auto fxTopRow = area.removeFromTop(compact ? 30 : 32);
        const int moduleW = compact ? 84 : 92;
        fxEnableBtn.setBounds(fxTopRow.removeFromRight(moduleW).withSizeKeepingCentre(moduleW, compact ? 26 : 28));
        fxTopRow.removeFromRight(G);
        fxModuleSelector.setBounds(fxTopRow.withSizeKeepingCentre(fxTopRow.getWidth(), compact ? 26 : 28));
        area.removeFromTop(G);

        const int fxKnobH = juce::jlimit(compact ? 158 : 224, compact ? 196 : 252,
                                         (int) (area.getHeight() * (compact ? 0.42f : 0.48f)));
        fxKnobGroupBounds = area.removeFromTop(fxKnobH);
        area.removeFromTop(G);
        inspectorBounds = area;

        std::array<DrumKnob*, 4> fxKnobs = { &fxParam1Knob, &fxParam2Knob, &fxParam3Knob, &fxParam4Knob };
        placeKnobs(fxKnobGroupBounds, fxKnobs, 2);

        auto inspArea = inspectorBounds.reduced(8, 7);
        inspArea.removeFromTop(16);
        const int inspectorGap = compact ? 8 : 10;

        auto presetRow = inspArea.removeFromTop(compact ? 28 : 30);
        const int presetW = (presetRow.getWidth() * 3) / 5;
        padPresetBox.setBounds(presetRow.removeFromLeft(presetW));
        presetRow.removeFromLeft(G);
        padOutputBox.setBounds(presetRow);
        inspArea.removeFromTop(inspectorGap);

        const int delayH = compact ? 28 : 30;
        const int meterTargetH = compact ? 24 : 28;
        const int meterBottomPad = compact ? 4 : 6;
        const int minSendH = compact ? 48 : 56;
        const int maxSendH = compact ? 78 : 90;
        const int preferredSendH = compact ? 64 : 74;
        const int sendBottomRequirement = inspectorGap + delayH + inspectorGap + 2 + meterTargetH + meterBottomPad;
        const int availableSendH = inspArea.getHeight() - sendBottomRequirement;
        const int sendH = juce::jlimit(minSendH, maxSendH,
                                       juce::jmin(preferredSendH, juce::jmax(minSendH, availableSendH)));
        auto sendRow = inspArea.removeFromTop(sendH);
        const int kw3 = juce::jmax(1, (sendRow.getWidth() - G * 2) / 3);
        velToClickKnob.setBounds(sendRow.removeFromLeft(kw3));
        sendRow.removeFromLeft(G);
        reverbSendKnob.setBounds(sendRow.removeFromLeft(kw3));
        sendRow.removeFromLeft(G);
        delaySendKnob.setBounds(sendRow);
        inspArea.removeFromTop(inspectorGap);

        auto delayRow = inspArea.removeFromTop(delayH);
        const int delaySyncW = compact ? 84 : 92;
        delaySyncBtn.setBounds(delayRow.removeFromLeft(delaySyncW).withSizeKeepingCentre(delaySyncW, compact ? 24 : 26));
        delayRow.removeFromLeft(G);
        delayNoteDivBox.setBounds(delayRow.withSizeKeepingCentre(delayRow.getWidth(), compact ? 24 : 26));
        inspArea.removeFromTop(inspectorGap);

        inspArea.removeFromTop(2);
        auto meterArea = inspArea;
        meterArea.removeFromBottom(juce::jmin(meterBottomPad, juce::jmax(0, meterArea.getHeight() - 22)));
        const int meterH = juce::jmin(meterTargetH, meterArea.getHeight());
        mainMeter.setBounds(meterArea.removeFromTop(meterH));
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
    return UITheme::padCategoryColour(i);
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

    levelAttach.reset();
    tuneAttach.reset();
    decayAttach.reset();
    attackAttach.reset();
    pitchDropAttach.reset();
    pitchDecayAttach.reset();
    noiseAttach.reset();
    clickAttach.reset();
    driveAttach.reset();
    cutoffAttach.reset();
    velToClickAttach.reset();
    reverbSendAttach.reset();
    delaySendAttach.reset();
    padOutputAttach.reset();

    levelAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "level"), levelKnob.getSlider());
    tuneAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "tune"), tuneKnob.getSlider());
    decayAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "decay"), decayKnob.getSlider());
    attackAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "attack"), attackKnob.getSlider());
    pitchDropAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "pitch_drop"), pitchDropKnob.getSlider());
    pitchDecayAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "pitch_decay"), pitchDecayKnob.getSlider());
    noiseAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "noise"), noiseKnob.getSlider());
    clickAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "click"), clickKnob.getSlider());
    driveAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "drive"), driveKnob.getSlider());
    cutoffAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "cutoff"), cutoffKnob.getSlider());
    velToClickAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "vel_to_click"), velToClickKnob.getSlider());
    reverbSendAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "rev_send"), reverbSendKnob.getSlider());
    delaySendAttach = std::make_unique<SliderAttach>(
        apvts, DrumSynthAudioProcessor::makePadParamId(sel, "dly_send"), delaySendKnob.getSlider());
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
        const auto id = DrumSynthAudioProcessor::makePadParamId(sel, suffix);
        if (auto* p = apvts.getRawParameterValue(id)) return p->load();
        return fallback;
    };
    const float a  = fetch("attack", 0.01f);
    const float d  = fetch("decay", 0.5f);
    const float pd = fetch("pitch_decay", 0.1f);
    const float n  = fetch("noise", 0.0f);
    envDisplay.updateFromADSR(juce::jlimit(0.0f, 1.0f, a / 2.0f),
                               juce::jlimit(0.0f, 1.0f, d / 5.0f),
                               juce::jlimit(0.0f, 1.0f, n),
                               juce::jlimit(0.0f, 1.0f, pd / 2.0f));
}
