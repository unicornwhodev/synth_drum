#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

using namespace UIThemeV5;

// =============================================================================
// PadComponentV5
// =============================================================================
void PadComponentV5::configure(int index, const juce::String& padName, juce::Colour catCol)
{
    idx = index;
    name = padName;
    cat = catCol;
}

void PadComponentV5::setSelected(bool s)
{
    if (sel != s) { sel = s; repaint(); }
}

void PadComponentV5::flash()
{
    flashA = 1.0f;
    repaint();
}

void PadComponentV5::setActivityLevel(float linear) noexcept
{
    // Audit Phase 4.4a: caller supplies a 0..1 linear envelope value.
    // Stored as target; tickFlash() (called every timer tick) smooths it.
    activityTarget = juce::jlimit(0.0f, 1.0f, linear);
}

void PadComponentV5::tickFlash()
{
    bool needsRepaint = false;
    if (flashA > 0.01f)
    {
        flashA *= 0.85f;
        if (flashA < 0.01f) flashA = 0.0f;
        needsRepaint = true;
    }
    // Smooth activity meter: fast attack (towards target), slow release.
    if (activityTarget > activity)
    {
        activity += (activityTarget - activity) * 0.55f;
        needsRepaint = true;
    }
    else if (activity > 0.005f)
    {
        activity *= 0.82f;
        if (activity < 0.005f) activity = 0.0f;
        needsRepaint = true;
    }
    activityTarget *= 0.6f;  // decay incoming target so a single trigger fades out cleanly
    if (needsRepaint)
        repaint();
}

void PadComponentV5::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float radius = 10.0f;

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(b.translated(0.0f, 3.0f), radius);

    // Base fill
    auto baseCol = sel ? cat.withAlpha(0.25f) : juce::Colour(0xff1A1D24);
    if (hover && !pressed) baseCol = baseCol.brighter(0.06f);
    g.setColour(baseCol);
    g.fillRoundedRectangle(b, radius);

    // Accent strip at top
    auto strip = b.removeFromTop(4.0f);
    g.setColour(sel ? cat : cat.withAlpha(0.5f));
    g.fillRoundedRectangle(strip, radius);

    // Inner recess
    auto recess = b.reduced(6.0f);
    g.setColour(juce::Colour(0xff0D0F14));
    g.fillRoundedRectangle(recess, 6.0f);

    // Pad number
    g.setColour(sel ? cat : juce::Colour(0xff6A7080));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f).withStyle("Bold")));
    g.drawText(juce::String(idx + 1), b.reduced(8.0f, 6.0f), juce::Justification::topLeft);

    // Name
    g.setColour(sel ? juce::Colour(0xffE8EAF0) : juce::Colour(0xff9098A8));
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.5f).withStyle("Bold")));
    g.drawText(name, b.reduced(8.0f, 18.0f), juce::Justification::centred);

    // Flash overlay
    if (flashA > 0.01f)
    {
        g.setColour(cat.withAlpha(flashA * 0.35f));
        g.fillRoundedRectangle(b, radius);
    }

    // Audit Phase 4.4a: mini-VU strip at bottom of the pad recess.
    // 3 px tall, full width minus 4 px padding, tinted with the category
    // colour. Width = activity (0..1) of the pad's recess width.
    if (activity > 0.005f)
    {
        const float trackY = recess.getBottom() - 5.0f;
        const float trackW = recess.getWidth() - 6.0f;
        const float trackX = recess.getX() + 3.0f;
        // Track background (subtle)
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.fillRoundedRectangle(trackX, trackY, trackW, 3.0f, 1.5f);
        // Active fill
        g.setColour(cat.withAlpha(0.85f));
        g.fillRoundedRectangle(trackX, trackY, trackW * activity, 3.0f, 1.5f);
    }
}

void PadComponentV5::resized()
{
    // Nothing dynamic — setBounds handles placement
}

void PadComponentV5::mouseDown(const juce::MouseEvent&) { pressed = true; repaint(); }
void PadComponentV5::mouseUp(const juce::MouseEvent&) { pressed = false; repaint(); }
void PadComponentV5::mouseEnter(const juce::MouseEvent&) { hover = true; repaint(); }
void PadComponentV5::mouseExit(const juce::MouseEvent&) { hover = false; pressed = false; repaint(); }

// =============================================================================
// DrumSynthAudioProcessorEditor
// =============================================================================
DrumSynthAudioProcessorEditor::DrumSynthAudioProcessorEditor(DrumSynthAudioProcessor& processor)
    : AudioProcessorEditor(&processor), proc(processor)
{
    setOpaque(true);

    // Header controls
    addAndMakeVisible(presetBox);
    addAndMakeVisible(familyFilterBox);
    addAndMakeVisible(prevBtn);
    addAndMakeVisible(nextBtn);
    addAndMakeVisible(masterGainDial);

    // Pad grid
    for (int i = 0; i < kNumPads; ++i)
    {
        auto name = juce::String("Pad ") + juce::String(i + 1);
        pads[i].configure(i, name, padCatColour(i));
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
    addAndMakeVisible(fxModuleSelector);
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

    // Audit Phase 4.5: utility button must own a non-null onClick handler
    // (covered by editor smoke test). Until the utility drawer ships, this
    // popup lists the available helper actions.
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

    // Audit Phase 4.4b: envelope visualization for the selected pad.
    addAndMakeVisible(envDisplay);

    // Audit Phase 4.7: pad preset + output routing selectors.
    padPresetBox.setTextWhenNothingSelected("Factory Pad Preset");
    addAndMakeVisible(padPresetBox);

    padOutputBox.addItem("Master", 1);
    for (int b = 1; b <= DrumSynthAudioProcessor::kNumAuxOutputs; ++b)
        padOutputBox.addItem("Out " + juce::String(b), b + 1);
    addAndMakeVisible(padOutputBox);

    // Bind to APVTS
    auto& apvts = proc.getAPVTS();
    levelAttach = std::make_unique<SliderAttach>(apvts, "level", levelKnob.getSlider());
    tuneAttach = std::make_unique<SliderAttach>(apvts, "tune", tuneKnob.getSlider());
    decayAttach = std::make_unique<SliderAttach>(apvts, "decay", decayKnob.getSlider());
    attackAttach = std::make_unique<SliderAttach>(apvts, "attack", attackKnob.getSlider());
    pitchDropAttach = std::make_unique<SliderAttach>(apvts, "pitch_drop", pitchDropKnob.getSlider());
    pitchDecayAttach = std::make_unique<SliderAttach>(apvts, "pitch_decay", pitchDecayKnob.getSlider());
    noiseAttach = std::make_unique<SliderAttach>(apvts, "noise", noiseKnob.getSlider());
    clickAttach = std::make_unique<SliderAttach>(apvts, "click", clickKnob.getSlider());
    driveAttach = std::make_unique<SliderAttach>(apvts, "drive", driveKnob.getSlider());
    cutoffAttach = std::make_unique<SliderAttach>(apvts, "cutoff", cutoffKnob.getSlider());
    macroPunchAttach = std::make_unique<SliderAttach>(apvts, "macro_punch", macroPunch.getSlider());
    macroWeightAttach = std::make_unique<SliderAttach>(apvts, "macro_weight", macroWeight.getSlider());
    macroAirAttach = std::make_unique<SliderAttach>(apvts, "macro_air", macroAir.getSlider());
    macroDirtAttach = std::make_unique<SliderAttach>(apvts, "macro_dirt", macroDirt.getSlider());
    masterGainAttach = std::make_unique<SliderAttach>(apvts, "output_gain", masterGainDial);

    // Initial state
    refreshPresetList();
    refreshPadSelection();
    rebindPadRoutingForSelected();
    refreshEnvelopeDisplay();

    startTimerHz(30);
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(kAspectRatio);
    getConstrainer()->setMinimumSize(kMinW, kMinH);
    setSize(1340, 760);
}

DrumSynthAudioProcessorEditor::~DrumSynthAudioProcessorEditor() {}

void DrumSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background gradient
    juce::ColourGradient bg(bgTop(), 0.0f, 0.0f, bgBottom(), 0.0f, (float)getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Subtle grid texture
    g.setColour(juce::Colours::white.withAlpha(0.018f));
    for (int x = 0; x < getWidth(); x += 24)
        g.drawLine((float)x, 0.0f, (float)x, (float)getHeight(), 0.5f);
    for (int y = 0; y < getHeight(); y += 24)
        g.drawLine(0.0f, (float)y, (float)getWidth(), (float)y, 0.5f);

    // Audit Phase 4.4c: voice-family contextual badge above the centre
    // (voice design) column. Tells the user which drum family they are
    // editing without changing the parameter set.
    if (levelKnob.getBounds().getWidth() > 0)
    {
        const int sel = selectedPadFromParam();
        const auto bounds = levelKnob.getBounds();
        const auto badge = juce::Rectangle<int>(bounds.getX(),
                                                juce::jmax(0, bounds.getY() - 22),
                                                bounds.getWidth() * 2 + 16,
                                                18);
        const auto col = padCatColour(sel);
        g.setColour(col.withAlpha(0.18f));
        g.fillRoundedRectangle(badge.toFloat(), 4.0f);
        g.setColour(col);
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
        g.drawText(juce::String("VOICE / ") + padCatName(sel),
                   badge.reduced(8, 0), juce::Justification::centredLeft);
    }
}

void DrumSynthAudioProcessorEditor::resized()
{
    const int M = 16;  // margin
    const int G = 12;  // gutter
    auto r = getLocalBounds().reduced(M);

    // ========== HEADER ==========
    auto header = r.removeFromTop(64);
    auto presetArea = header.removeFromLeft(280);
    auto filterArea = header.removeFromLeft(180);
    header.removeFromLeft(G);
    masterGainDial.setBounds(header.removeFromRight(80));

    presetBox.setBounds(presetArea.reduced(4));
    familyFilterBox.setBounds(filterArea.reduced(4));
    prevBtn.setBounds(header.removeFromLeft(36));
    header.removeFromLeft(G);
    nextBtn.setBounds(header.removeFromLeft(36));

    // ========== LEFT COLUMN ==========
    auto left = r.removeFromLeft(320);
    r.removeFromLeft(G);

    // Pad grid (4x3) — reduced height to leave room for envelope display
    const int envH = 80;  // Audit Phase 4.4b: ADSR strip below the pad grid
    auto gridArea = left.removeFromTop(left.getHeight() - 100 - envH - G);
    const int cols = 4, rows = 3;
    const int padGap = 10;
    const int padSide = juce::jmin((gridArea.getWidth() - padGap * (cols - 1)) / cols,
                                  (gridArea.getHeight() - padGap * (rows - 1)) / rows);
    const int gridX = gridArea.getX() + (gridArea.getWidth() - (cols * padSide + (cols - 1) * padGap)) / 2;
    const int gridY = gridArea.getY() + (gridArea.getHeight() - (rows * padSide + (rows - 1) * padGap)) / 2;
    for (int i = 0; i < kNumPads; ++i)
    {
        pads[i].setBounds(gridX + (i % cols) * (padSide + padGap),
                          gridY + (i / cols) * (padSide + padGap),
                          padSide, padSide);
    }

    // Envelope display strip + per-pad selectors row
    left.removeFromTop(G);
    envDisplay.setBounds(left.removeFromTop(envH));
    left.removeFromTop(G);
    auto selectorsRow = left.removeFromTop(28);
    padPresetBox.setBounds(selectorsRow.removeFromLeft(selectorsRow.getWidth() / 2 - G / 2));
    selectorsRow.removeFromLeft(G);
    padOutputBox.setBounds(selectorsRow);
    left.removeFromTop(G);

    // Macros row
    auto macros = left;
    const int macroW = (macros.getWidth() - 3 * G) / 4;
    macroPunch.setBounds(macros.removeFromLeft(macroW));
    macros.removeFromLeft(G);
    macroWeight.setBounds(macros.removeFromLeft(macroW));
    macros.removeFromLeft(G);
    macroAir.setBounds(macros.removeFromLeft(macroW));
    macros.removeFromLeft(G);
    macroDirt.setBounds(macros.removeFromLeft(macroW));

    // ========== CENTER ==========
    auto center = r.removeFromLeft(340);
    r.removeFromLeft(G);

    // Voice design knobs (2 cols x 5 rows)
    auto voiceArea = center;
    const int vCols = 2, vRows = 5;
    const int vGap = 8;
    const int vCellW = (voiceArea.getWidth() - vGap * (vCols - 1)) / vCols;
    const int vCellH = (voiceArea.getHeight() - vGap * (vRows - 1)) / vRows;
    std::array<KnobComponentV5*, 10> voiceKnobs = {
        &levelKnob, &tuneKnob, &decayKnob, &attackKnob, &pitchDropKnob,
        &pitchDecayKnob, &noiseKnob, &clickKnob, &driveKnob, &cutoffKnob
    };
    for (int i = 0; i < 10; ++i)
    {
        int c = i % vCols, row = i / vCols;
        voiceKnobs[i]->setBounds(voiceArea.getX() + c * (vCellW + vGap),
                                  voiceArea.getY() + row * (vCellH + vGap),
                                  vCellW, vCellH);
    }

    // ========== RIGHT ==========
    auto right = r;

    // FX module + params
    auto fxTop = right.removeFromTop(160);
    fxModuleSelector.setBounds(fxTop.removeFromTop(32));
    fxTop.removeFromTop(G);
    auto fxParams = fxTop;
    const int fpW = (fxParams.getWidth() - 3 * G) / 4;
    fxParam1Knob.setBounds(fxParams.removeFromLeft(fpW));
    fxParams.removeFromLeft(G);
    fxParam2Knob.setBounds(fxParams.removeFromLeft(fpW));
    fxParams.removeFromLeft(G);
    fxParam3Knob.setBounds(fxParams.removeFromLeft(fpW));
    fxParams.removeFromLeft(G);
    fxParam4Knob.setBounds(fxParams.removeFromLeft(fpW));

    // Meters
    auto meterArea = right;
    mainMeter.setBounds(meterArea.removeFromTop(meterArea.getHeight() / 2));
    auxMeter.setBounds(meterArea);

    // ========== BOTTOM BAR ==========
    // Audit Phase 4.5: utility buttons only. presetBox / prev / next stay in
    // the header (the duplicate placement below was overriding the header).
    auto bottom = getLocalBounds().removeFromBottom(48).reduced(M, 8);
    singleNoteBtn.setBounds(bottom.removeFromLeft(100));
    bottom.removeFromLeft(G);
    utilToggleBtn.setBounds(bottom.removeFromLeft(100));
}

void DrumSynthAudioProcessorEditor::timerCallback()
{
    // Audit Phase 4.4a: feed pad mini-VU from processor stamped activity.
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

    // Meter updates driven by processor state
    mainMeter.setValue(proc.getMainPeakMeter());

    refreshPadSelection();
    refreshPresetList();
    refreshEnvelopeDisplay();
}

void DrumSynthAudioProcessorEditor::selectPad(int idx)
{
    if (idx < 0 || idx >= kNumPads) return;
    selectedPadIdx = idx;

    // Update processor's selected pad
    proc.getAPVTS().getParameter("selected_pad")->setValueNotifyingHost((float)idx / 11.0f);

    refreshPadSelection();
    rebindPadRoutingForSelected();
    refreshEnvelopeDisplay();
}

int DrumSynthAudioProcessorEditor::selectedPadFromParam() const
{
    // Audit Phase 4: selected_pad is an AudioParameterChoice — its raw value
    // is already the choice INDEX (0..kNumPads-1), no normalisation needed.
    if (auto* p = proc.getAPVTS().getRawParameterValue("selected_pad"))
        return juce::jlimit(0, kNumPads - 1, static_cast<int>(p->load() + 0.5f));
    return 0;
}

void DrumSynthAudioProcessorEditor::refreshPadSelection()
{
    int sel = selectedPadFromParam();
    for (int i = 0; i < kNumPads; ++i) pads[i].setSelected(i == sel);
}

void DrumSynthAudioProcessorEditor::refreshPresetList()
{
    // Populate preset box from processor's scanned preset library.
    auto entries = proc.scanPresetLibrary();
    presetBox.clear();
    for (int i = 0; i < entries.size(); ++i)
        presetBox.addItem(entries[i].name, i + 1);
}

juce::Colour DrumSynthAudioProcessorEditor::padCatColour(int i) const
{
    static const juce::Colour cats[] = {
        juce::Colour(0xff9DBE47),  // kick
        juce::Colour(0xff70AED2),  // snare
        juce::Colour(0xff37D5C2),  // hat
        juce::Colour(0xff37D5C2),  // hat
        juce::Colour(0xff729EBA),  // tom
        juce::Colour(0xffAF9842),  // fx
        juce::Colour(0xff9A82C2),  // perc
        juce::Colour(0xff9DBE47),  // kick
        juce::Colour(0xff70AED2),  // snare
        juce::Colour(0xff37D5C2),  // hat
        juce::Colour(0xff729EBA),  // tom
        juce::Colour(0xffAF9842),  // fx
    };
    return cats[i % 12];
}

const char* DrumSynthAudioProcessorEditor::padCatName(int i) const
{
    static const char* names[] = { "Kick", "Snare", "HH", "HH", "Tom", "FX",
                                   "Perc", "Kick", "Snare", "HH", "Tom", "FX" };
    return names[i % 12];
}

void DrumSynthAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<int> area, juce::Colour accent)
{
    auto r = area.toFloat();
    fillPanel(g, r, 16.0f);
    g.setColour(accent.withAlpha(0.12f));
    g.drawRoundedRectangle(r, 16.0f, 1.5f);
}

void DrumSynthAudioProcessorEditor::refreshPadPresets()
{
    // Audit Phase 4.7: pad-level factory preset menu placeholder. The drum
    // engine does not currently ship per-pad factory snapshots, so the box
    // stays empty (textWhenNothingSelected covers the smoke-test contract).
    padPresetBox.clear();
}

void DrumSynthAudioProcessorEditor::rebindPadRoutingForSelected()
{
    // Audit Phase 4.7: rebind padOutputBox to the selected pad's APVTS param.
    const int sel = selectedPadFromParam();
    const auto paramId = DrumSynthAudioProcessor::makePadParamId(sel, "output");
    padOutputAttach.reset();  // detach previous
    padOutputAttach = std::make_unique<ComboBoxAttach>(proc.getAPVTS(), paramId, padOutputBox);
}

void DrumSynthAudioProcessorEditor::refreshEnvelopeDisplay()
{
    // Audit Phase 4.4b: drive the ADSR display from the live attack / decay /
    // pitch_decay parameters of the currently-selected pad. Per-pad params
    // use the "pad_X_<suffix>" naming convention; the bare names exposed via
    // SliderAttachment are not registered as APVTS params.
    auto& apvts = proc.getAPVTS();
    const int sel = selectedPadFromParam();
    auto fetch = [&apvts, sel](const char* suffix, float fallback)
    {
        const auto id = DrumSynthAudioProcessor::makePadParamId(sel, suffix);
        if (auto* p = apvts.getRawParameterValue(id)) return p->load();
        return fallback;
    };
    const float a  = fetch("attack",      0.01f);
    const float d  = fetch("decay",       0.5f);
    const float pd = fetch("pitch_decay", 0.1f);
    const float n  = fetch("noise",       0.0f);
    // Normalise rough seconds → 0..1 display range (display is purely visual).
    envDisplay.updateFromADSR(juce::jlimit(0.0f, 1.0f, a / 2.0f),
                               juce::jlimit(0.0f, 1.0f, d / 5.0f),
                               juce::jlimit(0.0f, 1.0f, n),
                               juce::jlimit(0.0f, 1.0f, pd / 2.0f));
}