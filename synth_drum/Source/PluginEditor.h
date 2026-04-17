#pragma once

#include <JuceHeader.h>
#include <array>
#include <functional>
#include <memory>

#include "PluginProcessor.h"

// =============================================================================
// Look-and-Feel — MIS Drum Synth theme (chartreuse accent #B5C200)
// =============================================================================
class MdsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MdsLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour&, bool, bool) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool, bool) override;
    void drawComboBox(juce::Graphics&, int w, int h, bool,
                      int, int, int, int, juce::ComboBox&) override;
    void drawProgressBar(juce::Graphics&, juce::ProgressBar&, int w, int h,
                         double progress, const juce::String& textToShow) override;
};

// =============================================================================
// Pad component — square pad with category accent, flash animation
// =============================================================================
class PadComponent : public juce::Component
{
public:
    void configure(int index, const juce::String& padName, const juce::String& padSummary, juce::Colour catCol);
    void setSelected(bool s);
    void flash();
    void tick();

    std::function<void(int)> onClicked;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    int          idx     = 0;
    juce::String name;
    juce::String summary;
    juce::Colour cat    { 0xff888888 };
    bool         sel     = false;
    float        flashA  = 0.0f;
    bool         hover   = false;
    bool         pressed = false;
};

// =============================================================================
// FX rack item — compact selectable module row with enabled state
// =============================================================================
class FxRackItem : public juce::Component
{
public:
    void configure(int index, const juce::String& moduleName, const juce::String& moduleSummary, juce::Colour moduleAccent);
    void setSelected(bool s);
    void setEnabledState(bool s);

    std::function<void(int)> onClicked;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    int          idx          = 0;
    juce::String name;
    juce::String summary;
    juce::Colour accent       { 0xff888888 };
    bool         sel          = false;
    bool         enabledState = true;
    bool         hover        = false;
};

// =============================================================================
// Main editor
// =============================================================================
class DrumSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit DrumSynthAudioProcessorEditor(DrumSynthAudioProcessor&);
    ~DrumSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Resizable support
    static constexpr int kMinW = 900;
    static constexpr int kMinH = 500;
    static constexpr float kAspectRatio = 1340.0f / 760.0f;

private:
    using APVTS          = juce::AudioProcessorValueTreeState;
    using SliderAttach   = APVTS::SliderAttachment;
    using ButtonAttach   = APVTS::ButtonAttachment;
    using ComboBoxAttach = APVTS::ComboBoxAttachment;

    struct CtrlDef
    {
        const char* label;
        const char* suffix;
        const char* shortTooltip;
        const char* noviceTooltip;
    };
    struct FxDef   { const char* label; const char* paramId; };
    struct ToggleDef { const char* label; const char* paramId; };
    struct AdvancedFxPageDef
    {
        const char* label;
        ToggleDef toggleA;
        ToggleDef toggleB;
        std::array<FxDef, 4> dials;
        juce::Colour accent;
    };

    void timerCallback() override;
    void rebuildPadAttachments();
    void rebuildAdvancedFxPage();
    void refreshPadSelection();
    void refreshPresetList();
    void refreshPadPresetList();
    void refreshPresetMetadata();
    void showSaveAsDialog();
    void saveCurrentPreset();
    void deleteCurrentUserPreset();
    void navigatePreset(int direction);
    void importPresetsFromZip();
    void refreshPresetActionButtons();
    void refreshStatusPanel();
    void refreshMidiLearnPanel();
    void refreshFxRackState();
    void setUtilityDrawerOpen(bool shouldOpen);
    bool presetEntryMatchesFilters(const DrumSynthAudioProcessor::PresetLibraryEntry& entry) const;
    int  selectedPadFromParam() const;
    void setupDial(juce::Slider& s, juce::Colour fill) const;
    void setupSmallDial(juce::Slider& s, juce::Colour fill) const;
    void updateSliderTextFormat(juce::Slider& s, const juce::String& paramId) const;

    static juce::String formatValueForParam(const juce::String& paramId, double value);

    static juce::Colour padCatColour(int i);
    static const char*  padCatName(int i);

    DrumSynthAudioProcessor& proc;
    MdsLookAndFeel lnf;
    std::unique_ptr<juce::FileChooser> fileChooser_;

    juce::Image bgTexture;

    // Icons
    juce::Image iconGain, iconOutput, iconSingle;
    std::array<juce::Image, 6> categoryIcons;
    std::array<juce::Image, 11> padCtrlIcons;
    std::array<juce::Image, 4> macroIcons;
    juce::Image fxSectionIconSat, fxSectionIconTrans, fxSectionIconComp, fxSectionIconReverb;
    std::array<juce::Image, 15> fxDialIcons;

    // Header
    juce::ComboBox     presetBox;
    juce::ComboBox     presetSourceFilterBox;
    juce::ComboBox     presetFamilyFilterBox;
    juce::ComboBox     presetRoleFilterBox;
    juce::ComboBox     presetTagFilterBox;
    juce::TextButton   prevPresetBtn, nextPresetBtn;
    juce::TextButton   saveBtn, saveAsBtn, deleteBtn, importBtn;
    juce::Slider       gainDial;
    juce::ToggleButton singleNoteBtn;
    juce::Label        presetMetaLabel;

    // Pad grid (4x3 = 12)
    std::array<PadComponent, mds::kNumPads> pads;
    juce::ComboBox padSelector;   // hidden APVTS-bound

    // Instrument panel
    juce::Label    instrTitle;
    juce::Label    padSummaryLabel;
    juce::ComboBox padPresetBox;
    juce::ComboBox outputBox;

    // Pad parameter dials (11)
    static constexpr int kPadCtrlN = 11;
    std::array<juce::Slider, kPadCtrlN> padDials;
    std::array<juce::Label,  kPadCtrlN> padDlLabels;
    std::array<std::unique_ptr<SliderAttach>, kPadCtrlN> padDlAttach;

    // Macros (4 visible)
    static constexpr int kMacroN = 4;
    std::array<juce::Slider, kMacroN> macroDials;
    std::array<juce::Label,  kMacroN> macroLbls;
    std::array<std::unique_ptr<SliderAttach>, kMacroN> macroAtt;

    // FX dials (full runtime chain, including reverb)
    static constexpr int kFxN = 15;
    std::array<juce::Slider, kFxN> fxDials;
    std::array<juce::Label,  kFxN> fxLbls;
    std::array<std::unique_ptr<SliderAttach>, kFxN> fxAtt;

    // APVTS bindings
    std::unique_ptr<SliderAttach>   gainAtt;
    std::unique_ptr<ButtonAttach>   singleAtt;
    std::unique_ptr<ComboBoxAttach> selPadAtt;
    std::unique_ptr<ComboBoxAttach> outAtt;
    std::unique_ptr<ComboBoxAttach> qualityModeAtt;
    std::unique_ptr<ComboBoxAttach> velocityCurveAtt;
    std::unique_ptr<ComboBoxAttach> advancedChoiceAtt;

    juce::ComboBox   velocityCurveBox;

    int cachedPad = -1;
    int factoryPresetCount = 0;
    juce::Array<juce::File> userPresetFiles;
    juce::Array<DrumSynthAudioProcessor::PresetLibraryEntry> visiblePresetEntries;
    juce::StringArray midiLearnTargetIds;
    juce::StringArray midiLearnMappedParamIds;
    bool presetUiRefreshing = false;
    int cachedPadPresetListPad = -1;
    int cachedPadPresetFactoryIndex = -2;
    int cachedPadPresetFactoryCount = -1;

    static const std::array<CtrlDef, kPadCtrlN> kPadCtrls;
    static const std::array<FxDef,   kMacroN>   kMacroCtrls;
    static const std::array<FxDef,   kFxN>      kFxCtrls;
    static constexpr int kFxRackCount  = 8;
    static constexpr int kFxDetailPool = 7;

    std::array<FxRackItem, kFxRackCount> fxRackItems;
    int  selectedFxModule = 0;
    bool utilityDrawerOpen = false;

    // Advanced FX page strip
    juce::ComboBox advancedFxPageBox;
    juce::ToggleButton advancedToggleA, advancedToggleB;
    std::array<juce::Slider, 4> advancedFxDials;
    std::array<juce::Label, 4> advancedFxLabels;
    std::array<std::unique_ptr<SliderAttach>, 4> advancedFxAtt;
    std::unique_ptr<ButtonAttach> advancedToggleAAtt;
    std::unique_ptr<ButtonAttach> advancedToggleBAtt;
    static const std::array<AdvancedFxPageDef, 5> kAdvancedFxPages;

    // FX section mappings.
    static constexpr int kSatIndices[2]  = { 6, 7 };
    static constexpr int kTransIndices[3] = { 8, 9, 10 };
    static constexpr int kCompIndices[6] = { 0, 1, 2, 3, 4, 5 };
    static constexpr int kReverbIndices[4] = { 11, 12, 13, 14 };

    // FX section enable toggles (sat, trans, comp, reverb)
    juce::ToggleButton fxSatEnBtn, fxTransEnBtn, fxCompEnBtn, fxReverbEnBtn;
    std::unique_ptr<ButtonAttach> fxSatEnAtt, fxTransEnAtt, fxCompEnAtt, fxReverbEnAtt;

    // LFO controls
    juce::Slider     lfoRateDial, lfoDepthDial;
    juce::ComboBox   lfoWaveBox;
    juce::Label      lfoLabel;
    std::unique_ptr<SliderAttach>   lfoRateAtt, lfoDepthAtt;
    std::unique_ptr<ComboBoxAttach> lfoWaveAtt;

    juce::Slider     humanizeTimingDial, humanizeLevelDial;
    juce::Label      humanizeLabel;
    std::unique_ptr<SliderAttach> humanizeTimingAtt, humanizeLevelAtt;

    // Tooltip / MIDI CC page overlay
    static constexpr int kTooltipCount = 31; // 11 pad + 4 macro + 15 FX + 1 gain
    juce::TooltipWindow tooltipWindow { this, 600 };
    int tooltipMode = 0; // 0=off, 1=short, 2=novice
    juce::TextButton tooltipModeBtn;
    juce::Label      midiCCPageLabel;
    juce::ComboBox   qualityModeBox;
    juce::Label      engineStatusLabel;
    juce::Label      mainMeterLabel;
    juce::Label      auxMeterLabel;
    double           mainMeterValue = 0.0;
    double           auxMeterValue = 0.0;
    juce::ProgressBar mainMeterBar { mainMeterValue };
    juce::ProgressBar auxMeterBar { auxMeterValue };
    juce::TextButton clipResetBtn;
    juce::ComboBox   midiLearnTargetBox;
    juce::ComboBox   midiLearnMappingsBox;
    juce::TextButton midiLearnArmBtn;
    juce::TextButton midiLearnClearBtn;
    juce::TextButton midiLearnResetBtn;
    juce::Label      midiLearnStatusLabel;
    juce::TextButton utilityDrawerBtn;

    void cycleTooltipMode();
    void applyTooltips();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSynthAudioProcessorEditor)
};
