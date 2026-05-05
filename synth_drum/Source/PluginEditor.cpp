#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

using namespace UIThemeHW;

namespace
{
class EditorLookAndFeelHW final : public juce::LookAndFeel_V4
{
public:
    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return UIThemeHW::labelFont();
    }

    juce::Label* createComboBoxTextBox (juce::ComboBox& box) override
    {
        auto* label = juce::LookAndFeel_V4::createComboBoxTextBox (box);
        label->setFont (UIThemeHW::labelFont());
        label->setColour (juce::Label::textColourId, UIThemeHW::textMain());
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
        UIThemeHW::fillPanel (g, area, UIThemeHW::panelRadius());
        auto rail = area.reduced (12.0f, 8.0f).removeFromTop (2.0f);
        UIThemeHW::drawLED (g, rail, 1.0f, 0.42f);

        juce::Path arrow;
        const float cx = area.getRight() - 16.0f;
        const float cy = area.getCentreY() + 1.0f;
        arrow.startNewSubPath (cx - 4.5f, cy - 3.0f);
        arrow.lineTo (cx, cy + 2.0f);
        arrow.lineTo (cx + 4.5f, cy - 3.0f);
        g.setColour (UIThemeHW::accentOrange());
        g.strokePath (arrow, juce::PathStrokeType (1.8f));
    }

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour&, bool isMouseOverButton,
                               bool isButtonDown) override
    {
        auto area = button.getLocalBounds().toFloat().reduced (0.5f);
        UIThemeHW::fillPanel (g, area, UIThemeHW::panelRadius());

        if (button.getToggleState())
            UIThemeHW::drawLED (g, area.reduced (12.0f, 0.0f).removeFromBottom (3.0f), 1.5f, 0.72f);
        else if (isMouseOverButton)
            UIThemeHW::drawLED (g, area.reduced (14.0f, 0.0f).removeFromBottom (2.0f), 1.5f, 0.28f);

        if (isButtonDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.14f));
            g.fillRoundedRectangle (area, UIThemeHW::panelRadius());
        }
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return UIThemeHW::labelFont();
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool,
                         bool isButtonDown) override
    {
        g.setColour (UIThemeHW::textMain().withAlpha (isButtonDown ? 0.78f : 1.0f));
        g.setFont (UIThemeHW::labelFont());
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, false);
    }

    juce::Font getPopupMenuFont() override
    {
        return UIThemeHW::labelFont();
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        UIThemeHW::fillPanel (g, juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height), UIThemeHW::panelRadius());
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
            UIThemeHW::fillRecess (g, row, 8.0f);

        g.setColour (textColourToUse != nullptr ? *textColourToUse
                                                : (isActive ? UIThemeHW::textMain() : UIThemeHW::textDim()));
        g.setFont (UIThemeHW::labelFont());
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

constexpr FxModuleDef makeFxModule(
    const char* label, const char* summary,
    const char* enableParamId,
    std::array<const char*, 4> paramIds,
    juce::Colour accent)
{
    return { label, summary, enableParamId, paramIds, accent };
}

constexpr std::array<FxModuleDef, 8> kFxModules =
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
// DrumSynthAudioProcessorEditor
// =============================================================================
DrumSynthAudioProcessorEditor::DrumSynthAudioProcessorEditor(DrumSynthAudioProcessor& processor)
    : AudioProcessorEditor(&processor), proc(processor)
{
    setOpaque(true);
    editorLookAndFeel = std::make_unique<EditorLookAndFeelHW>();
    setLookAndFeel(editorLookAndFeel.get());

    // Header controls
    presetBox.setTextWhenNothingSelected("Select kit");
    familyFilterBox.setTextWhenNothingSelected("All families");
    prevBtn.setButtonText("<");
    nextBtn.setButtonText(">");
    masterGainDial.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    masterGainDial.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterGainDial.setScrollWheelEnabled(false);
    masterGainDial.setMouseDragSensitivity(280);
    addAndMakeVisible(presetBox);
    addAndMakeVisible(familyFilterBox);
    addAndMakeVisible(prevBtn);
    addAndMakeVisible(nextBtn);
    addAndMakeVisible(masterGainDial);

    // Pad grid
    for (int i = 0; i < kNumPads; ++i)
    {
        pads[i].configure(i, juce::String(mds::makePadName(i)), padCatColour(i));
        pads[i].onClicked = [this](int idx) { selectPad(idx); };
        addAndMakeVisible(pads[i]);
    }

    // Voice design knobs
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

    // Macros
    addAndMakeVisible(macroPunch);
    addAndMakeVisible(macroWeight);
    addAndMakeVisible(macroAir);
    addAndMakeVisible(macroDirt);

    // FX section
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

    // Meters
    addAndMakeVisible(mainMeter);
    addAndMakeVisible(auxMeter);

    // Utility
    addAndMakeVisible(singleNoteBtn);
    addAndMakeVisible(utilToggleBtn);

    // Inspector
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

    // Delay sync controls
    addAndMakeVisible(delaySyncBtn);
    delayNoteDivBox.addItemList({ "1/4", "1/8", "1/16", "dotted 1/8", "triplet 1/8" }, 1);
    delayNoteDivBox.setTextWhenNothingSelected("Division");
    addAndMakeVisible(delayNoteDivBox);

    // Handlers
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
    utilToggleBtn.onClick = [this]
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Reload current preset");
        menu.addItem(2, "Clear MIDI Learn mappings");
        menu.addSeparator();
        menu.addItem(3, "Open user presets folder");
        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(&utilToggleBtn),
                           [this](int choice)
                           {
                               if (choice == 1)
                                   proc.applyFactoryPreset(juce::jmax(0, proc.getCurrentFactoryPresetIndex()));
                               else if (choice == 2)
                                   proc.clearAllMidiLearn();
                               else if (choice == 3)
                                   DrumSynthAudioProcessor::getUserPresetsDirectory().revealToUser();
                           });
    };

    // APVTS attachments
    auto& apvts = proc.getAPVTS();
    singleNoteAttach = std::make_unique<ButtonAttach>(apvts, "singleNoteMode", singleNoteBtn);
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
    masterGainAttach = std::make_unique<SliderAttach>(apvts, "masterGain", masterGainDial);
    delaySyncAttach = std::make_unique<ButtonAttach>(apvts, "delaySyncOn", delaySyncBtn);
    delayNoteDivAttach = std::make_unique<ComboBoxAttach>(apvts, "delayNoteDiv", delayNoteDivBox);

    // Initial refresh
    refreshPresetList();
    refreshFxModuleUi();
    refreshPadSelection();
    refreshPadPresets();
    rebindPadRoutingForSelected();
    refreshEnvelopeDisplay();

    setSize(1100, 700);
    startTimerHz(30);
}

DrumSynthAudioProcessorEditor::~DrumSynthAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void DrumSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    drawBackground(g);

    drawSectionPanel(g, padBankBounds, "PAD BANK", UIThemeHW::accentBlue());
    drawSectionPanel(g, voiceBounds, "VOICE SHAPER", UIThemeHW::accentOrange());
    drawSectionPanel(g, macroBounds, "PERFORMER", UIThemeHW::ledGreen());
    const auto& fxMod = kFxModules[(std::size_t) juce::jlimit(0, (int) kFxModules.size() - 1, selectedFxModule)];
    drawSectionPanel(g, fxBounds, juce::String("BUS FX / ") + fxMod.label, fxMod.accent);
    drawSectionPanel(g, outputBounds, "OUTPUT", UIThemeHW::accentBlue());
    drawSectionPanel(g, inspectorBounds, "INSPECTOR", padCatColour(selectedPadFromParam()));

    // Header title
    auto headerArea = headerBounds.toFloat().reduced(16.0f, 12.0f);
    g.setColour(UIThemeHW::textWhite());
    g.setFont(UIThemeHW::labelFont().withHeight(22.0f));
    g.drawText("DRUM SYNTH HW", headerArea.removeFromTop(32.0f), juce::Justification::centredLeft, false);

    // Footer
    auto footerArea = footerBounds.toFloat().reduced(16.0f, 8.0f);
    g.setColour(UIThemeHW::textDim());
    g.setFont(UIThemeHW::smallFont());
    g.drawText("DRUM SYNTH HW \u2014 V5", footerArea, juce::Justification::centred, false);
}

void DrumSynthAudioProcessorEditor::drawBackground(juce::Graphics& g)
{
    g.fillAll(UIThemeHW::rackBlack());

    // Subtle horizontal lines
    for (int y = 28; y < getHeight(); y += 28)
    {
        g.setColour(juce::Colours::white.withAlpha(0.018f));
        g.drawHorizontalLine(y, 0.0f, (float)getWidth());
    }

    // Corner screws
    auto drawScrew = [&](float x, float y)
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillEllipse(x - 5.0f, y - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.drawEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f, 1.0f);
    };
    drawScrew(18.0f, 18.0f);
    drawScrew((float)getWidth() - 18.0f, 18.0f);
    drawScrew(18.0f, (float)getHeight() - 18.0f);
    drawScrew((float)getWidth() - 18.0f, (float)getHeight() - 18.0f);
}

void DrumSynthAudioProcessorEditor::drawSectionPanel(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                    const juce::String& title, juce::Colour accentCol)
{
    auto r = area.toFloat();
    UIThemeHW::fillPanel(g, r, UIThemeHW::panelGrey(), accentCol, UIThemeHW::panelRadius());

    // Header strip
    auto headerStrip = r.removeFromTop(28.0f);
    g.setColour(UIThemeHW::panelDark());
    g.fillRect(headerStrip);

    // LED indicator
    auto ledRect = headerStrip.removeFromLeft(8.0f).reduced(4.0f, 6.0f);
    UIThemeHW::drawLED(g, ledRect.getCentre(), 3.5f, true, accentCol, UIThemeHW::ledOff());

    // Title text
    g.setColour(UIThemeHW::creamPanel());
    g.setFont(UIThemeHW::labelFont());
    g.drawText(title, headerStrip, juce::Justification::centredLeft, false);

    // Accent line at bottom of header
    g.setColour(accentCol.withAlpha(0.5f));
    g.drawHorizontalLine((float)area.getY() + 27.0f, (float)area.getX() + 4.0f, (float)area.getRight() - 4.0f);
}

void DrumSynthAudioProcessorEditor::resized()
{
    // Enforce 1100x700 aspect ratio
    const int M = 10;
    const int G = 8;
    const int H_H = 60;
    const int F_H = 40;
    const int leftW = 280;
    const int rightW = 260;

    auto r = getLocalBounds().reduced(M);

    headerBounds = r.removeFromTop(H_H);
    r.removeFromTop(G);
    footerBounds = r.removeFromBottom(F_H);
    r.removeFromBottom(G);

    auto left = r.removeFromLeft(leftW);
    r.removeFromLeft(G);
    auto center = r;
    auto right = r.removeFromRight(rightW);

    padBankBounds = left;
    inspectorBounds = center.removeFromTop((center.getHeight() - G) * 3 / 5);
    center.removeFromTop(G);
    macroBounds = center;

    voiceBounds = right.removeFromTop((right.getHeight() - G) * 2 / 3);
    right.removeFromTop(G);
    fxBounds = right.removeFromTop((right.getHeight() - G) * 3 / 4);
    right.removeFromTop(G);
    outputBounds = right;

    // Header: preset, family filter, prev/next, master gain
    auto header = headerBounds.reduced(16, 10);
    presetBox.setBounds(header.removeFromRight(220).reduced(0, 8));
    header.removeFromRight(8);
    familyFilterBox.setBounds(header.removeFromRight(140).reduced(0, 8));
    header.removeFromRight(8);
    prevBtn.setBounds(header.removeFromRight(32).reduced(0, 8));
    header.removeFromRight(4);
    nextBtn.setBounds(header.removeFromRight(32).reduced(0, 8));
    header.removeFromRight(12);
    masterGainDial.setBounds(header.removeFromRight(60));

    // Pad grid: 4x3
    auto padArea = padBankBounds.reduced(12, 12);
    padArea.removeFromTop(32);
    const int cols = 4;
    const int rows = 3;
    const int padGap = 8;
    const int padW = (padArea.getWidth() - padGap * (cols - 1)) / cols;
    const int padH = (padArea.getHeight() - padGap * (rows - 1)) / rows;
    for (int i = 0; i < kNumPads; ++i)
    {
        pads[i].setBounds(padArea.getX() + (i % cols) * (padW + padGap),
                          padArea.getY() + (i / cols) * (padH + padGap),
                          padW, padH);
    }

    // Voice: 5x2 knobs
    auto voiceArea = voiceBounds.reduced(12, 12);
    voiceArea.removeFromTop(32);
    const int vCols = 5;
    const int vRows = 2;
    const int vGap = 6;
    const int vCellW = (voiceArea.getWidth() - vGap * (vCols - 1)) / vCols;
    const int vCellH = (voiceArea.getHeight() - vGap * (vRows - 1)) / vRows;
    std::array<HardwareKnob*, 10> voiceKnobs = {
        &levelKnob, &tuneKnob, &attackKnob, &decayKnob, &pitchDropKnob,
        &pitchDecayKnob, &noiseKnob, &clickKnob, &driveKnob, &cutoffKnob
    };
    for (int i = 0; i < 10; ++i)
    {
        const int c = i % vCols;
        const int row = i / vCols;
        voiceKnobs[(std::size_t) i]->setBounds(
            voiceArea.getX() + c * (vCellW + vGap),
            voiceArea.getY() + row * (vCellH + vGap),
            vCellW, vCellH);
    }

    // Macros: 4 in a row
    auto macroArea = macroBounds.reduced(12, 12);
    macroArea.removeFromTop(32);
    const int macroGap = 8;
    const int macroW = (macroArea.getWidth() - macroGap * 3) / 4;
    macroPunch.setBounds(macroArea.removeFromLeft(macroW));
    macroArea.removeFromLeft(macroGap);
    macroWeight.setBounds(macroArea.removeFromLeft(macroW));
    macroArea.removeFromLeft(macroGap);
    macroAir.setBounds(macroArea.removeFromLeft(macroW));
    macroArea.removeFromLeft(macroGap);
    macroDirt.setBounds(macroArea);

    // FX: selector + enable + 4 knobs in 2x2
    auto fxArea = fxBounds.reduced(12, 12);
    fxArea.removeFromTop(32);
    auto fxTop = fxArea.removeFromTop(30);
    fxModuleSelector.setBounds(fxTop.removeFromLeft(fxTop.getWidth() - 100));
    fxTop.removeFromLeft(8);
    fxEnableBtn.setBounds(fxTop);
    fxArea.removeFromTop(8);

    std::array<HardwareKnob*, 4> fxKnobs = { &fxParam1Knob, &fxParam2Knob, &fxParam3Knob, &fxParam4Knob };
    const int fxCols = 2;
    const int fxRows = 2;
    const int fxGap = 8;
    const int fxCellW = (fxArea.getWidth() - fxGap) / fxCols;
    const int fxCellH = (fxArea.getHeight() - fxGap) / fxRows;
    for (int i = 0; i < 4; ++i)
    {
        const int c = i % fxCols;
        const int row = i / fxCols;
        fxKnobs[(std::size_t) i]->setBounds(
            fxArea.getX() + c * (fxCellW + fxGap),
            fxArea.getY() + row * (fxCellH + fxGap),
            fxCellW, fxCellH);
    }

    // Output: stacked meters
    auto meterArea = outputBounds.reduced(12, 12);
    meterArea.removeFromTop(32);
    mainMeter.setBounds(meterArea.removeFromTop((meterArea.getHeight() - 8) / 2));
    meterArea.removeFromTop(8);
    auxMeter.setBounds(meterArea);

    // Inspector: pad preset, output, 3 knobs, envelope
    auto inspectArea = inspectorBounds.reduced(12, 12);
    inspectArea.removeFromTop(32);
    auto selectorsRow = inspectArea.removeFromTop(28);
    padPresetBox.setBounds(selectorsRow.removeFromLeft((selectorsRow.getWidth() * 3) / 5));
    selectorsRow.removeFromLeft(8);
    padOutputBox.setBounds(selectorsRow);
    inspectArea.removeFromTop(8);
    envDisplay.setBounds(inspectArea.removeFromTop(100));
    inspectArea.removeFromTop(8);
    auto padKnobs = inspectArea;
    const int inspGap = 8;
    const int inspW = (padKnobs.getWidth() - inspGap * 2) / 3;
    velToClickKnob.setBounds(padKnobs.removeFromLeft(inspW));
    padKnobs.removeFromLeft(inspGap);
    reverbSendKnob.setBounds(padKnobs.removeFromLeft(inspW));
    padKnobs.removeFromLeft(inspGap);
    delaySendKnob.setBounds(padKnobs);
}

void DrumSynthAudioProcessorEditor::timerCallback()
{
    // Update pad activity levels
    for (int i = 0; i < kNumPads; ++i)
    {
        const float v = proc.consumePadTriggerActivity(i);
        if (v > 0.0f)
        {
            pads[i].setActivityLevel(v);
            pads[i].flash();
        }
    }

    // Tick pad flash animations
    for (auto& p : pads) p.tickFlash();

    // Update meters
    mainMeter.setValue(proc.getMainPeakMeter());
    auxMeter.setValue(proc.getAuxPeakMeter());

    // Sync pad selection
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

void DrumSynthAudioProcessorEditor::selectPad(int idx)
{
    if (idx < 0 || idx >= kNumPads) return;
    selectedPadIdx = idx;
    proc.getAPVTS().getParameter("selectedPad")->setValueNotifyingHost((float)idx);
    refreshPadSelection();
    rebindPadRoutingForSelected();
    refreshEnvelopeDisplay();
    refreshFxModuleUi();
}

int DrumSynthAudioProcessorEditor::selectedPadFromParam() const
{
    if (auto* p = proc.getAPVTS().getParameter("selectedPad"))
        return juce::jlimit(0, kNumPads - 1, static_cast<int>(p->getValue() + 0.5f));
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
        case 0: return UIThemeHW::accentOrange();  // kick
        case 1: return juce::Colour::fromRGB(255, 184, 77);  // snare
        case 2: return UIThemeHW::ledGreen();   // hihat
        case 3: return UIThemeHW::accentBlue();  // tom
        default: return juce::Colour::fromRGB(180, 100, 220);  // fx
    }
}

const char* DrumSynthAudioProcessorEditor::padCatName(int i) const
{
    switch (i / 3)
    {
        case 0: return "KICK";
        case 1: return "SNARE";
        case 2: return "HH";
        case 3: return "TOM";
        default: return "FX";
    }
}

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

void DrumSynthAudioProcessorEditor::rebindPadRoutingForSelected()
{
    const int sel = selectedPadFromParam();
    auto& apvts = proc.getAPVTS();

    velToClickAttach.reset();
    reverbSendAttach.reset();
    delaySendAttach.reset();

    velToClickAttach = std::make_unique<SliderAttach>(
        apvts, "pad" + juce::String(sel) + "_velToClick", velToClickKnob.getSlider());
    reverbSendAttach = std::make_unique<SliderAttach>(
        apvts, "pad" + juce::String(sel) + "_reverbSend", reverbSendKnob.getSlider());
    delaySendAttach = std::make_unique<SliderAttach>(
        apvts, "pad" + juce::String(sel) + "_delaySend", delaySendKnob.getSlider());
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

    std::array<HardwareKnob*, 4> knobs = { &fxParam1Knob, &fxParam2Knob, &fxParam3Knob, &fxParam4Knob };
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