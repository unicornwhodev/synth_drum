#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <memory>

#include "PluginProcessor.h"
#include "../new composants/UITheme.h"
#include "../new composants/LayoutGrid.h"
#include "../new composants/DrumKnob.h"
#include "../new composants/DrumPad.h"
#include "../new composants/DrumMeter.h"
#include "../new composants/DrumEnvelope.h"
#include "../new composants/DrumSelector.h"
#include "../new composants/DrumToggle.h"
#include "../new composants/DrumWaveform.h"

// =============================================================================
// Main editor — MIS Drum Synth v5 — UI/UX Redesign
// =============================================================================
class DrumSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit DrumSynthAudioProcessorEditor(DrumSynthAudioProcessor&);
    ~DrumSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    static constexpr int kDefaultW = 1000;
    static constexpr int kDefaultH = 820;
    static constexpr int kMinW = 800;
    static constexpr int kMinH = 650;

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttach = APVTS::SliderAttachment;
    using ButtonAttach = APVTS::ButtonAttachment;
    using ComboBoxAttach = APVTS::ComboBoxAttachment;

    DrumSynthAudioProcessor& proc;
    std::unique_ptr<juce::LookAndFeel_V4> editorLookAndFeel;

    // Layout regions
    juce::Rectangle<int> headerBounds;
    juce::Rectangle<int> footerBounds;
    juce::Rectangle<int> padZoneBounds;
    juce::Rectangle<int> voiceZoneBounds;
    juce::Rectangle<int> fxZoneBounds;
    juce::Rectangle<int> envBounds;
    juce::Rectangle<int> inspectorBounds;

    juce::Array<DrumSynthAudioProcessor::PresetLibraryEntry> visiblePresetEntries;
    bool presetUiRefreshing = false;
    int selectedFxModule = 0;
    bool singleMode = false;

    // ---- Header ----
    DrumSelector presetBox;
    DrumSelector familyFilterBox;
    juce::TextButton prevBtn{ "<" }, nextBtn{ ">" };
    juce::TextButton utilityDrawerBtn{ "Utility" };
    DrumKnob masterGainKnob{ "GAIN", 0.0, 2.0, 1.0, UITheme::accentOrange() };
    DrumToggle singleModeBtn{ "SINGLE", UITheme::accentCyan() };
    bool utilityDrawerOpen = false;

    // ---- Pad Grid ----
    static constexpr int kNumPads = 12;
    std::array<DrumPad, kNumPads> pads;
    int selectedPadIdx = 0;

    // ---- Voice Design ----
    DrumKnob levelKnob{ "LEVEL", 0.0, 1.0, 0.8, UITheme::accentOrange() };
    DrumKnob tuneKnob{ "TUNE", -24.0, 24.0, 0.0, UITheme::accentOrange() };
    DrumKnob decayKnob{ "DECAY", 0.001, 5.0, 0.8, UITheme::accentOrange() };
    DrumKnob attackKnob{ "ATTACK", 0.001, 2.0, 0.01, UITheme::accentOrange() };
    DrumKnob pitchDropKnob{ "PITCH DROP", 0.0, 48.0, 0.0, UITheme::accentOrange() };
    DrumKnob pitchDecayKnob{ "PITCH DECAY", 0.001, 2.0, 0.1, UITheme::accentOrange() };
    DrumKnob noiseKnob{ "NOISE", 0.0, 1.0, 0.0, UITheme::accentCyan() };
    DrumKnob clickKnob{ "CLICK", 0.0, 1.0, 0.3, UITheme::accentCyan() };
    DrumKnob driveKnob{ "DRIVE", 0.0, 1.0, 0.0, UITheme::accentOrange() };
    DrumKnob cutoffKnob{ "CUTOFF", 20.0, 20000.0, 8000.0, UITheme::accentCyan() };

    // ---- Macros ----
    DrumKnob macroPunch{ "PUNCH", 0.0, 1.0, 0.5, UITheme::accentGreen() };
    DrumKnob macroWeight{ "WEIGHT", 0.0, 1.0, 0.5, UITheme::accentGreen() };
    DrumKnob macroAir{ "AIR", 0.0, 1.0, 0.5, UITheme::accentGreen() };
    DrumKnob macroDirt{ "DIRT", 0.0, 1.0, 0.5, UITheme::accentGreen() };

    // ---- FX Section ----
    DrumSelector fxModuleSelector;
    DrumToggle fxEnableBtn{ "MODULE", UITheme::accentAmber() };
    DrumKnob fxParam1Knob{ "PARAM 1", 0.0, 1.0, 0.5, UITheme::accentAmber() };
    DrumKnob fxParam2Knob{ "PARAM 2", 0.0, 1.0, 0.5, UITheme::accentAmber() };
    DrumKnob fxParam3Knob{ "PARAM 3", 0.0, 1.0, 0.5, UITheme::accentAmber() };
    DrumKnob fxParam4Knob{ "PARAM 4", 0.0, 1.0, 0.5, UITheme::accentAmber() };

    // ---- Meters ----
    DrumMeter mainMeter;

    // ---- Inspector / Routing ----
    DrumToggle synthSampleToggle{ "SYNTH", UITheme::accentCyan() };
    DrumWaveform waveformDisplay;
    DrumSelector padPresetBox;
    DrumSelector padOutputBox;
    DrumKnob velToClickKnob{ "VEL→CLICK", 0.0, 1.0, 0.6, UITheme::accentCyan() };
    DrumKnob reverbSendKnob{ "RVB SEND", 0.0, 1.0, 0.0, UITheme::accentGreen() };
    DrumKnob delaySendKnob{ "DLY SEND", 0.0, 1.0, 0.0, UITheme::accentGreen() };
    DrumEnvelope envDisplay;
    DrumToggle delaySyncBtn{ "DLY SYNC", UITheme::accentAmber() };
    DrumSelector delayNoteDivBox;
    std::unique_ptr<SliderAttach> velToClickAttach;
    std::unique_ptr<SliderAttach> reverbSendAttach;
    std::unique_ptr<SliderAttach> delaySendAttach;
    std::unique_ptr<ButtonAttach> delaySyncAttach;
    std::unique_ptr<ComboBoxAttach> delayNoteDivAttach;
    std::unique_ptr<ComboBoxAttach> padOutputAttach;

    // ---- Attachments ----
    std::unique_ptr<ButtonAttach> singleNoteAttach;
    std::unique_ptr<SliderAttach> levelAttach;
    std::unique_ptr<SliderAttach> tuneAttach;
    std::unique_ptr<SliderAttach> decayAttach;
    std::unique_ptr<SliderAttach> attackAttach;
    std::unique_ptr<SliderAttach> pitchDropAttach;
    std::unique_ptr<SliderAttach> pitchDecayAttach;
    std::unique_ptr<SliderAttach> noiseAttach;
    std::unique_ptr<SliderAttach> clickAttach;
    std::unique_ptr<SliderAttach> driveAttach;
    std::unique_ptr<SliderAttach> cutoffAttach;
    std::unique_ptr<SliderAttach> macroPunchAttach;
    std::unique_ptr<SliderAttach> macroWeightAttach;
    std::unique_ptr<SliderAttach> macroAirAttach;
    std::unique_ptr<SliderAttach> macroDirtAttach;
    std::unique_ptr<SliderAttach> masterGainAttach;
    std::unique_ptr<ButtonAttach> fxEnableAttach;
    std::array<std::unique_ptr<SliderAttach>, 4> fxParamAttach;

    // Internal
    void timerCallback() override;
    void selectPad(int idx);
    int selectedPadFromParam() const;
    juce::Colour padCatColour(int i) const;
    const char* padCatName(int i) const;
    void refreshPresetList();
    void refreshPresetSelectionFromProcessor();
    void navigateVisiblePreset(int delta);
    void refreshPadSelection();
    void refreshPadPresets();
    void refreshFxModuleUi();
    void rebindPadRoutingForSelected();
    void refreshEnvelopeDisplay();

    void drawBackground(juce::Graphics& g);
    void drawSectionPanel(juce::Graphics& g, const juce::Rectangle<int>& area,
                          const juce::String& title, juce::Colour accentCol);
    void drawPadWithActivity(juce::Graphics& g, const juce::Rectangle<int>& area, int padIdx);
    void drawFamilyBadge(juce::Graphics& g, juce::Point<float> centre, const juce::String& text, juce::Colour col);
};
