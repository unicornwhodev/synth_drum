#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <memory>

#include "PluginProcessor.h"
#include "new composants/KnobComponentV5.h"
#include "new composants/FaderComponentV5.h"
#include "new composants/VUMeterComponentV5.h"
#include "new composants/EnvelopeDisplayComponentV5.h"
#include "new composants/OutputMeterComponentV5.h"
#include "new composants/ToggleButtonComponentV5.h"
#include "new composants/SelectorComponentV5.h"
#include "new composants/UIThemeV5.h"

// =============================================================================
// Pad component — clean modern pad with accent glow
// =============================================================================
class PadComponentV5 : public juce::Component
{
public:
    void configure(int index, const juce::String& padName, juce::Colour catCol);
    void setSelected(bool s);
    void flash();

    // Audit Phase 4.4a: per-pad activity meter (mini-VU strip at bottom).
    // Smoothed activity envelope updated by editor timer; paints a 3 px
    // horizontal bar tinted with the pad's category colour.
    void setActivityLevel(float linear) noexcept;

    std::function<void(int)> onClicked;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

    // Per-frame flash decay update (called by parent timer).
    void tickFlash();

private:
    int idx = 0;
    juce::String name;
    juce::Colour cat { 0xff888888 };
    bool sel = false;
    float flashA = 0.0f;
    bool hover = false;
    bool pressed = false;
    float activity = 0.0f;        // smoothed, displayed mini-VU value
    float activityTarget = 0.0f;  // raw incoming value, smoothed in tickFlash
};

// =============================================================================
// Main editor — MIS Drum Synth v5
// =============================================================================
class DrumSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit DrumSynthAudioProcessorEditor(DrumSynthAudioProcessor&);
    ~DrumSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    static constexpr int kMinW = 900;
    static constexpr int kMinH = 580;
    static constexpr float kAspectRatio = 1340.0f / 760.0f;

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttach = APVTS::SliderAttachment;
    using ButtonAttach = APVTS::ButtonAttachment;
    using ComboBoxAttach = APVTS::ComboBoxAttachment;

    // Parameter layout helpers
    static juce::String formatValueForParam(const juce::String& paramId, double value);
    void setupKnob(juce::Slider& s);
    void setupFader(juce::Slider& s);

    // State
    DrumSynthAudioProcessor& proc;

    // ---- Header ----
    juce::ComboBox presetBox;
    juce::ComboBox familyFilterBox;
    juce::TextButton prevBtn, nextBtn;
    juce::Slider masterGainDial;

    // ---- Pad Grid ----
    static constexpr int kNumPads = 12;
    std::array<PadComponentV5, kNumPads> pads;
    int selectedPadIdx = 0;

    // ---- Voice Design ----
    KnobComponentV5 levelKnob{ "LEVEL" };
    KnobComponentV5 tuneKnob{ "TUNE", -24.0, 24.0, 0.0 };
    KnobComponentV5 decayKnob{ "DECAY", 0.001, 5.0, 0.8 };
    KnobComponentV5 attackKnob{ "ATTACK", 0.001, 2.0, 0.01 };
    KnobComponentV5 pitchDropKnob{ "PITCH DROP", 0.0, 48.0, 0.0 };
    KnobComponentV5 pitchDecayKnob{ "PITCH DECAY", 0.001, 2.0, 0.1 };
    KnobComponentV5 noiseKnob{ "NOISE", 0.0, 1.0, 0.0 };
    KnobComponentV5 clickKnob{ "CLICK", 0.0, 1.0, 0.3 };
    KnobComponentV5 driveKnob{ "DRIVE", 0.0, 1.0, 0.0 };
    KnobComponentV5 cutoffKnob{ "CUTOFF", 20.0, 20000.0, 8000.0 };

    // ---- Macros ----
    KnobComponentV5 macroPunch{ "PUNCH" };
    KnobComponentV5 macroWeight{ "WEIGHT" };
    KnobComponentV5 macroAir{ "AIR" };
    KnobComponentV5 macroDirt{ "DIRT" };

    // ---- FX Section ----
    SelectorComponentV5 fxModuleSelector;
    KnobComponentV5 fxParam1Knob{ "PARAM 1" };
    KnobComponentV5 fxParam2Knob{ "PARAM 2" };
    KnobComponentV5 fxParam3Knob{ "PARAM 3" };
    KnobComponentV5 fxParam4Knob{ "PARAM 4" };

    // ---- Meters ----
    OutputMeterComponentV5 mainMeter;
    OutputMeterComponentV5 auxMeter;

    // ---- Utility ----
    ToggleButtonComponentV5 singleNoteBtn{ "SINGLE" };
    ToggleButtonComponentV5 utilToggleBtn{ "UTILITY" };

    // Audit Phase 4.7: pad-level routing & preset selectors required by the
    // editor smoke test and the Phase 4 UX plan (§4.4 / §4.5).
    juce::ComboBox padPresetBox;   // factory pad preset (per-pad recall)
    juce::ComboBox padOutputBox;   // pad output bus routing (Master / Out 1..N)
    std::unique_ptr<ComboBoxAttach> padOutputAttach;  // re-bound on selectedPad change

    // Audit Phase 4.4b: ADSR display under the pad grid.
    EnvelopeDisplayComponentV5 envDisplay;

    // ---- Attachments ----
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

    // Internal
    void timerCallback() override;
    void selectPad(int idx);
    int selectedPadFromParam() const;
    juce::Colour padCatColour(int i) const;
    const char* padCatName(int i) const;
    void refreshPresetList();
    void refreshPadSelection();
    void refreshPadPresets();

    // Audit Phase 4.4b/4.7: rebind padOutputBox to the currently selected pad
    // and refresh the envelope display from APVTS attack/decay/pitch_decay.
    void rebindPadRoutingForSelected();
    void refreshEnvelopeDisplay();

    // Helpers
    static void drawPanel(juce::Graphics& g, juce::Rectangle<int> area, juce::Colour accent);
};