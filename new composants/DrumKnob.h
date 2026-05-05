#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumKnob — Hardware dark rotary control with LED ring indicator
// =============================================================================
class DrumKnob : public juce::Component
{
public:
    DrumKnob(juce::String labelText,
             double min = 0.0, double max = 1.0, double value = 0.5,
             juce::Colour accentCol = UITheme::accentOrange());

    juce::Slider& getSlider() noexcept { return slider; }
    void setLabelText(const juce::String& newText);
    juce::String getLabelText() const { return label.getText(); }
    void setAccentColour(juce::Colour c) { accentColour = c; lnf.accentCol = c; repaint(); }
    void resized() override;

private:
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel(juce::Colour ac) : accentCol(ac) {}

        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPosProportional, float rotaryStartAngle,
                              float rotaryEndAngle, juce::Slider&) override;

        juce::Colour accentCol;
    };

    static juce::String formatValue(const juce::Slider& slider);
    static float knobAngle(float proportion, float start, float end);

    LookAndFeel lnf;
    juce::Slider slider;
    juce::Label label;
    juce::Colour accentColour;
};

// =============================================================================
// Implementation
// =============================================================================
inline DrumKnob::DrumKnob(juce::String labelText, double min, double max, double value, juce::Colour ac)
    : lnf(ac), accentColour(ac)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(min, max);
    slider.setValue(value);
    slider.setLookAndFeel(&lnf);
    slider.setScrollWheelEnabled(false);
    slider.setMouseDragSensitivity(280);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(UITheme::fontLabel());
    label.setColour(juce::Label::textColourId, UITheme::textDim());
    label.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

inline void DrumKnob::resized()
{
    auto area = getLocalBounds().reduced(2);
    label.setBounds(area.removeFromTop(14));
    area.removeFromTop(2);
    slider.setBounds(area);
}

inline void DrumKnob::setLabelText(const juce::String& newText)
{
    label.setText(newText, juce::dontSendNotification);
}

inline juce::String DrumKnob::formatValue(const juce::Slider& slider)
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
        return juce::String(value, 1);

    return juce::String(juce::roundToInt(value));
}

inline float DrumKnob::knobAngle(float proportion, float start, float end)
{
    return start + proportion * (end - start);
}

inline void DrumKnob::LookAndFeel::drawRotarySlider(juce::Graphics& g,
                                                     int x, int y, int width, int height,
                                                     float sliderPosProportional,
                                                     float rotaryStartAngle,
                                                     float rotaryEndAngle,
                                                     juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(6.0f);
    auto centre = bounds.getCentre();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f - 4.0f;

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillEllipse(bounds.translated(0.0f, 3.0f).reduced(4.0f));

    // Knob body — dark brushed metal look
    juce::ColourGradient bodyGrad(UITheme::panelBase().withBrightness(1.08f), centre.x, centre.y - radius * 0.7f,
                                   UITheme::panelBase().withBrightness(0.85f), centre.x, centre.y + radius * 0.7f, false);
    g.setGradientFill(bodyGrad);
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // Inner ring
    g.setColour(UITheme::panelInset());
    g.fillEllipse(centre.x - radius * 0.78f, centre.y - radius * 0.78f,
                  radius * 1.56f, radius * 1.56f);

    // LED arc indicator
    const float arcRadius = radius * 0.88f;
    const float arcThickness = 3.5f;
    const auto angle = juce::jmap(sliderPosProportional, rotaryStartAngle, rotaryEndAngle);

    // Background arc (dim)
    juce::Path bgArc;
    bgArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                        rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(accentCol.withAlpha(0.12f));
    g.strokePath(bgArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Active arc (bright)
    if (sliderPosProportional > 0.001f)
    {
        juce::Path activeArc;
        activeArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, angle, true);
        g.setColour(accentCol.withAlpha(0.9f));
        g.strokePath(activeArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Glow at the tip
        auto tip = centre.getPointOnCircumference(arcRadius, angle);
        g.setColour(accentCol.withAlpha(0.3f));
        g.fillEllipse(tip.x - 5.0f, tip.y - 5.0f, 10.0f, 10.0f);
    }

    // Pointer line
    auto p1 = centre.getPointOnCircumference(radius * 0.22f, angle);
    auto p2 = centre.getPointOnCircumference(radius * 0.68f, angle);
    g.setColour(UITheme::textMain());
    g.drawLine({p1, p2}, 2.5f);

    // Centre dot
    g.setColour(UITheme::panelBase().withBrightness(1.1f));
    g.fillEllipse(centre.x - 3.5f, centre.y - 3.5f, 7.0f, 7.0f);
    g.setColour(accentCol.withAlpha(0.5f));
    g.fillEllipse(centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);

    // Value text (shown when dragging or on hover)
    if (slider.isMouseOverOrDragging())
    {
        g.setColour(UITheme::textMain());
        g.setFont(UITheme::fontValue());
        auto valText = formatValue(slider);
        g.drawText(valText, bounds.reduced(8.0f), juce::Justification::centred, false);
    }
}
