#pragma once
#include <JuceHeader.h>
#include "UIThemeHW.h"

// =============================================================================
// HardwareKnob — clean hardware-style rotary control
// No tick marks, LED ring indicator, pointer line, monospace value
// =============================================================================
class HardwareKnob : public juce::Component
{
public:
    HardwareKnob(juce::String labelText,
                 double min = 0.0, double max = 1.0, double value = 0.5,
                 juce::Colour ledColour = UIThemeHW::ledGreen());

    juce::Slider& getSlider() noexcept { return slider; }
    void setLabelText(const juce::String& newText);
    juce::String getLabelText() const { return label.getText(); }
    void setLEDColour(juce::Colour c) { ledColour = c; lnf.ledCol = c; repaint(); }
    void resized() override;

private:
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel(juce::Colour lc) : ledCol(lc) {}

        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle,
                              float rotaryEndAngle, juce::Slider&) override;

        juce::Colour ledCol;
    };

    static juce::String formatValue(const juce::Slider& slider);
    static float knobAngle(float proportion, float start, float end);

    LookAndFeel lnf;
    juce::Slider slider;
    juce::Label label;
    juce::Colour ledColour;
};

// =============================================================================
// HardwareKnob implementation
// =============================================================================
inline HardwareKnob::HardwareKnob(juce::String labelText, double min, double max, double value, juce::Colour lc)
    : lnf(lc), ledColour(lc)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(min, max);
    slider.setValue(value);
    slider.setLookAndFeel(&lnf);
    slider.setScrollWheelEnabled(false);
    slider.setMouseDragSensitivity(280);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(UIThemeHW::labelFont());
    label.setColour(juce::Label::textColourId, UIThemeHW::creamPanel());
    label.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

inline void HardwareKnob::resized()
{
    auto area = getLocalBounds().reduced(4);
    label.setBounds(area.removeFromTop(16));
    area.removeFromTop(2);
    slider.setBounds(area);
}

inline void HardwareKnob::setLabelText(const juce::String& newText)
{
    label.setText(newText, juce::dontSendNotification);
}

inline juce::String HardwareKnob::formatValue(const juce::Slider& slider)
{
    const auto range = slider.getRange();
    const double maxValue = range.getEnd();
    const double minValue = range.getStart();
    const double value = slider.getValue();

    if (maxValue <= 1.001 && minValue >= -1.001)
        return juce::String(juce::roundToInt(value * 100.0)) + "%";

    if (maxValue >= 1000.0)
    {
        if (value >= 1000.0)
            return juce::String(value / 1000.0, value >= 10000.0 ? 1 : 2) + "k";
        return juce::String(juce::roundToInt(value));
    }

    if (maxValue - minValue <= 10.0)
        return juce::String(value, 2);

    if (maxValue - minValue <= 100.0)
        return juce::String(value, std::abs(value) < 10.0 ? 1 : 0);

    return juce::String(juce::roundToInt(value));
}

inline float HardwareKnob::knobAngle(float proportion, float start, float end)
{
    return juce::jmap(proportion, start, end);
}

inline void HardwareKnob::LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                                        float sliderPosProportional, float, float, juce::Slider& slider)
{
    auto full = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
    auto area = full.reduced(8.0f, 4.0f);
    auto c = area.getCentre();
    auto radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f;

    // --- LED ring (behind knob) ---
    const float startAngle = juce::MathConstants<float>::pi * 1.25f;  // ~225 degrees
    const float endAngle   = juce::MathConstants<float>::pi * 2.75f;    // ~495 degrees (270 degree sweep)
    const int numLEDs = 16;

    for (int i = 0; i < numLEDs; ++i)
    {
        float t = (float)i / (float)(numLEDs - 1);
        float a = juce::jmap(t, startAngle, endAngle);
        auto p = c.getPointOnCircumference(radius * 1.05f, a);
        bool active = t <= sliderPosProportional;

        if (active)
        {
            // Glow
            g.setColour(ledCol.withAlpha(0.25f));
            g.fillEllipse(juce::Rectangle<float>(8.0f, 8.0f).withCentre(p));
        }

        // LED dot
        g.setColour(active ? ledCol : UIThemeHW::ledOff());
        g.fillEllipse(juce::Rectangle<float>(5.0f, 5.0f).withCentre(p));
    }

    // --- Knob shadow ---
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(area.translated(0.0f, 4.0f));

    // --- Outer ring (knob housing) ---
    auto outerRing = area.reduced(1.0f);
    juce::ColourGradient outerGrad(
        UIThemeHW::knobMetal().withBrightness(0.35f), c.x, outerRing.getY(),
        UIThemeHW::knobMetal().withBrightness(0.28f), c.x, outerRing.getBottom(), false);
    g.setGradientFill(outerGrad);
    g.fillEllipse(outerRing);

    // --- Middle band (brushed metal) ---
    auto middleBand = outerRing.reduced(outerRing.getWidth() * 0.08f);
    juce::ColourGradient midGrad(
        UIThemeHW::brushedAlum().withBrightness(1.1f), c.x, middleBand.getY(),
        UIThemeHW::brushedAlum().withBrightness(0.85f), c.x, middleBand.getBottom(), false);
    g.setGradientFill(midGrad);
    g.fillEllipse(middleBand);

    // --- Inner body (dark cap) ---
    auto innerBody = middleBand.reduced(middleBand.getWidth() * 0.22f);
    juce::ColourGradient bodyGrad(
        UIThemeHW::knobDark().withBrightness(1.15f), c.x, innerBody.getY(),
        UIThemeHW::knobDark(), c.x, innerBody.getBottom(), false);
    g.setGradientFill(bodyGrad);
    g.fillEllipse(innerBody);

    // --- Pointer line ---
    const float pointerStart = juce::MathConstants<float>::pi * 1.25f;
    const float pointerEnd   = juce::MathConstants<float>::pi * 2.75f;
    const float angle = knobAngle(sliderPosProportional, pointerStart, pointerEnd);

    auto p1 = c.getPointOnCircumference(radius * 0.22f, angle);
    auto p2 = c.getPointOnCircumference(radius * 0.65f, angle);

    // Pointer glow
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawLine(p1.x, p1.y, p2.x, p2.y, 5.0f);

    // Pointer line
    g.setColour(UIThemeHW::knobPointer());
    g.drawLine(p1.x, p1.y, p2.x, p2.y, 2.0f);

    // --- Value readout below ---
    auto textArea = juce::Rectangle<float>(radius * 1.8f, 14.0f).withCentre(
        juce::Point<float>(c.x, c.y + radius * 0.35f));

    // Background pill
    g.setColour(juce::Colour::fromRGB(15, 15, 18).withAlpha(0.85f));
    g.fillRoundedRectangle(textArea.expanded(4.0f, 2.0f), 2.0f);

    // Value text
    g.setColour(UIThemeHW::textWhite());
    g.setFont(UIThemeHW::valueFont());
    g.drawText(formatValue(slider), textArea, juce::Justification::centred);
}