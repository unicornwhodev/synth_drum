#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <memory>

#include "PluginProcessor.h"
#include "../new composants/UITheme.h"
#include "../new composants/DrumKnob.h"
#include "../new composants/DrumPad.h"
#include "../new composants/DrumMeter.h"
#include "../new composants/DrumEnvelope.h"
#include "../new composants/DrumSelector.h"
#include "../new composants/DrumToggle.h"

// =============================================================================
// UtilityDrawerPanel — overlay hosting the global engine settings
// =============================================================================
class UtilityDrawerPanel : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        g.setColour(UITheme::bgDeep().withAlpha(0.55f));
        g.fillRoundedRectangle(area, UITheme::cornerRadius());
        UITheme::drawSectionCard(g, area, "UTILITY — GLOBAL ENGINE", UITheme::accent());
    }
};

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

    static constexpr int kDefaultW = 1180;
    static constexpr int kDefaultH = 800;
    static constexpr int kMinW = 960;
    static constexpr int kMinH = 700;

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
    juce::Rectangle<int> padGridBounds;
    juce::Rectangle<int> envBounds;
    juce::Rectangle<int> voiceCoreBounds;
    juce::Rectangle<int> voiceTimbreBounds;
    juce::Rectangle<int> voiceModelBounds;
    juce::Rectangle<int> voiceMacroBounds;
    juce::Rectangle<int> fxKnobGroupBounds;
    juce::Rectangle<int> inspectorBounds;

    juce::Array<DrumSynthAudioProcessor::PresetLibraryEntry> visiblePresetEntries;
    bool presetUiRefreshing = false;
    int selectedFxModule = 0;

    // ---- Header ----
    DrumSelector presetBox;
    DrumSelector familyFilterBox;
    juce::TextButton prevBtn{ "<" }, nextBtn{ ">" };
    juce::TextButton utilityDrawerBtn{ "Utility" };
    DrumKnob masterGainKnob{ "GAIN", 0.0, 2.0, 1.0, UITheme::accent(), "x" };
    DrumToggle singleModeBtn{ "SINGLE", UITheme::accentCyan() };
    bool utilityDrawerOpen = false;

    // ---- Pad Grid ----
    static constexpr int kNumPads = 12;
    std::array<DrumPad, kNumPads> pads;
    int selectedPadIdx = 0;

    // ---- Voice Design ----
    DrumKnob levelKnob{ "LEVEL", 0.0, 1.0, 0.8, UITheme::accent(), "%" };
    DrumKnob tuneKnob{ "TUNE", -24.0, 24.0, 0.0, UITheme::accent(), "st" };
    DrumKnob decayKnob{ "DECAY", 0.004, 2.5, 0.8, UITheme::accent(), "s" };
    DrumKnob attackKnob{ "ATTACK", 0.0, 0.05, 0.005, UITheme::accent(), "s" };
    DrumKnob pitchDropKnob{ "PITCH DROP", 0.0, 48.0, 0.0, UITheme::accent(), "st" };
    DrumKnob pitchDecayKnob{ "PITCH DECAY", 0.002, 1.2, 0.1, UITheme::accent(), "s" };
    DrumKnob noiseKnob{ "NOISE", 0.0, 1.0, 0.0, UITheme::accentCyan(), "%" };
    DrumKnob clickKnob{ "CLICK", 0.0, 1.0, 0.3, UITheme::accentCyan(), "%" };
    DrumKnob driveKnob{ "DRIVE", 1.0, 12.0, 1.0, UITheme::accent(), "x" };
    DrumKnob cutoffKnob{ "CUTOFF", 120.0, 18000.0, 8000.0, UITheme::accentCyan(), "Hz" };
    DrumKnob panKnob{ "PAN", -1.0, 1.0, 0.0, UITheme::accent(), "%" };
    DrumKnob modelKnob1{ "MODEL 1", 0.0, 1.0, 0.5, UITheme::accent(), "%" };
    DrumKnob modelKnob2{ "MODEL 2", 0.0, 1.0, 0.5, UITheme::accent(), "%" };

    // ---- Macros ----
    DrumKnob macroPunch{ "PUNCH", 0.0, 1.0, 0.5, UITheme::accentGreen(), "%" };
    DrumKnob macroWeight{ "WEIGHT", 0.0, 1.0, 0.5, UITheme::accentGreen(), "%" };
    DrumKnob macroAir{ "AIR", 0.0, 1.0, 0.5, UITheme::accentGreen(), "%" };
    DrumKnob macroDirt{ "DIRT", 0.0, 1.0, 0.5, UITheme::accentGreen(), "%" };

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
    DrumSelector padPresetBox;
    DrumSelector padOutputBox;
    DrumKnob velToClickKnob{ "VEL CLICK", 0.0, 1.0, 0.6, UITheme::accentCyan(), "%" };
    DrumKnob reverbSendKnob{ "RVB SEND", 0.0, 1.0, 0.0, UITheme::accentGreen(), "%" };
    DrumKnob delaySendKnob{ "DLY SEND", 0.0, 1.0, 0.0, UITheme::accentGreen(), "%" };
    DrumEnvelope envDisplay;
    DrumToggle delaySyncBtn{ "DLY SYNC", UITheme::accentAmber() };
    DrumSelector delayNoteDivBox;
    std::unique_ptr<SliderAttach> velToClickAttach;
    std::unique_ptr<SliderAttach> reverbSendAttach;
    std::unique_ptr<SliderAttach> delaySendAttach;
    std::unique_ptr<ButtonAttach> delaySyncAttach;
    std::unique_ptr<ComboBoxAttach> delayNoteDivAttach;
    std::unique_ptr<ComboBoxAttach> padOutputAttach;

    // ---- Utility drawer (global engine settings) ----
    UtilityDrawerPanel utilityDrawerPanel;
    DrumKnob humanizeTimeKnob{ "HUM TIME", 0.0, 50.0, 0.0, UITheme::accent(), "ms" };
    DrumKnob humanizeLvlKnob{ "HUM LEVEL", 0.0, 0.2, 0.0, UITheme::accent(), "%" };
    DrumKnob lfoRateKnob{ "LFO RATE", 0.1, 20.0, 2.0, UITheme::accentCyan(), "Hz" };
    DrumKnob lfoDepthKnob{ "LFO DEPTH", 0.0, 1.0, 0.0, UITheme::accentCyan(), "%" };
    DrumSelector velocityCurveBox;
    DrumSelector lfoWaveBox;
    DrumSelector qualityModeBox;
    DrumToggle auxPostFxBtn{ "AUX POST-FX", UITheme::accentGreen() };
    juce::Label velocityCurveLabel, lfoWaveLabel, qualityModeLabel;

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
    std::unique_ptr<SliderAttach> panAttach;
    std::unique_ptr<SliderAttach> modelKnob1Attach;
    std::unique_ptr<SliderAttach> modelKnob2Attach;
    std::unique_ptr<SliderAttach> macroPunchAttach;
    std::unique_ptr<SliderAttach> macroWeightAttach;
    std::unique_ptr<SliderAttach> macroAirAttach;
    std::unique_ptr<SliderAttach> macroDirtAttach;
    std::unique_ptr<SliderAttach> masterGainAttach;
    std::unique_ptr<ButtonAttach> fxEnableAttach;
    std::array<std::unique_ptr<SliderAttach>, 4> fxParamAttach;
    std::unique_ptr<SliderAttach> humanizeTimeAttach, humanizeLvlAttach, lfoRateAttach, lfoDepthAttach;
    std::unique_ptr<ComboBoxAttach> velocityCurveAttach, lfoWaveAttach, qualityModeAttach;
    std::unique_ptr<ButtonAttach> auxPostFxAttach;

    // Internal
    void timerCallback() override;
    void selectPad(int idx);
    void togglePadBoolParam(int pad, const char* suffix);
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
    void layoutUtilityDrawer();

    void drawBackground(juce::Graphics& g);
    void drawSectionPanel(juce::Graphics& g, const juce::Rectangle<int>& area,
                          const juce::String& title, juce::Colour accentCol);
    void drawGroupFrame(juce::Graphics& g, const juce::Rectangle<int>& area,
                        const juce::String& title, juce::Colour accentCol);
    void drawFamilyBadge(juce::Graphics& g, juce::Point<float> centre, const juce::String& text, juce::Colour col);
};
