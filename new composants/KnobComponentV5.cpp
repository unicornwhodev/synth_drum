#include "KnobComponentV5.h"

namespace
{
    juce::String formatKnobValue (const juce::Slider& slider)
    {
        const auto range = slider.getRange();
        const double minValue = range.getStart();
        const double maxValue = range.getEnd();
        const double value = slider.getValue();

        if (maxValue <= 1.001 && minValue >= -1.001)
            return juce::String (juce::roundToInt (value * 100.0)) + "%";

        if (maxValue >= 1000.0)
        {
            if (value >= 1000.0)
                return juce::String (value / 1000.0, value >= 10000.0 ? 1 : 2) + "k";
            return juce::String (juce::roundToInt (value));
        }

        if (maxValue - minValue <= 10.0)
            return juce::String (value, 2);

        if (maxValue - minValue <= 100.0)
            return juce::String (value, std::abs (value) < 10.0 ? 1 : 0);

        return juce::String (juce::roundToInt (value));
    }

    void drawKnobTicksV5 (juce::Graphics& g, juce::Point<float> c, float radius, float value)
    {
        const float start = juce::MathConstants<float>::pi * 1.19f;
        const float end   = juce::MathConstants<float>::pi * 2.81f;
        const int count = 25;

        for (int i = 0; i < count; ++i)
        {
            float t = (float) i / (float) (count - 1);
            float a = juce::jmap (t, start, end);
            auto p1 = c.getPointOnCircumference (radius * 1.01f, a);
            auto p2 = c.getPointOnCircumference (radius * 1.13f, a);

            bool active = i <= juce::roundToInt (value * (float) (count - 1));
            g.setColour (active ? UIThemeV5::accent().withAlpha (0.95f)
                                : juce::Colours::white.withAlpha (0.12f));
            g.drawLine ({ p1, p2 }, active ? 1.9f : 1.0f);
        }
    }
}

KnobComponentV5::KnobComponentV5 (juce::String labelText, double min, double max, double value)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange (min, max);
    slider.setValue (value);
    slider.setLookAndFeel (&lnf);

    label.setText (labelText, juce::dontSendNotification);
    label.setFont (UIThemeV5::labelFont());
    label.setColour (juce::Label::textColourId, UIThemeV5::textMain());
    label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (slider);
    addAndMakeVisible (label);
}

void KnobComponentV5::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat().reduced (1.0f);
    UIThemeV5::fillRecess (g, area, 15.0f);

    auto topRail = area.reduced (10.0f, 9.0f).removeFromTop (3.0f);
    UIThemeV5::drawGlowStrip (g, topRail, 1.5f, 0.55f);
}

void KnobComponentV5::resized()
{
    auto area = getLocalBounds().reduced (6);
    label.setBounds (area.removeFromTop (18));
    area.removeFromTop (2);
    slider.setBounds (area);
}

void KnobComponentV5::setLabelText (const juce::String& newText)
{
    label.setText (newText, juce::dontSendNotification);
}

void KnobComponentV5::LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                     float sliderPosProportional, float, float, juce::Slider& slider)
{
    auto full = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    auto area = full.reduced (10.0f, 6.0f);
    auto c = area.getCentre();
    auto radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;

    g.setColour (juce::Colours::black.withAlpha (0.38f));
    g.fillEllipse (area.translated (0.0f, 5.0f));

    auto ring = area.reduced (1.0f);
    juce::ColourGradient ringGrad (juce::Colour::fromRGB (59, 67, 79), ring.getCentreX(), ring.getY(),
                                   juce::Colour::fromRGB (20, 24, 31), ring.getCentreX(), ring.getBottom(), false);
    ringGrad.addColour (0.50, juce::Colour::fromRGB (36, 42, 52));
    g.setGradientFill (ringGrad);
    g.fillEllipse (ring);

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawEllipse (ring, 1.0f);

    auto middle = ring.reduced (ring.getWidth() * 0.13f);
    juce::ColourGradient midGrad (UIThemeV5::metalUpper().withMultipliedAlpha (0.70f), c.x, middle.getY(),
                                  UIThemeV5::metalLo().darker (0.30f), c.x, middle.getBottom(), false);
    g.setGradientFill (midGrad);
    g.fillEllipse (middle);

    auto face = middle.reduced (middle.getWidth() * 0.18f);
    juce::ColourGradient faceGrad (juce::Colour::fromRGB (52, 58, 67), c.x, face.getY(),
                                   juce::Colour::fromRGB (25, 29, 35), c.x, face.getBottom(), false);
    faceGrad.addColour (0.5, juce::Colour::fromRGB (38, 43, 50));
    g.setGradientFill (faceGrad);
    g.fillEllipse (face);

    auto centreCap = face.reduced (face.getWidth() * 0.34f);
    g.setColour (UIThemeV5::accentStrong().withAlpha (0.16f));
    g.fillEllipse (centreCap);

    drawKnobTicksV5 (g, c, radius, sliderPosProportional);

    const float start = juce::MathConstants<float>::pi * 1.19f;
    const float end   = juce::MathConstants<float>::pi * 2.81f;
    const float angle = juce::jmap (sliderPosProportional, start, end);

    juce::Path arc;
    arc.addCentredArc (c.x, c.y, radius * 0.98f, radius * 0.98f, 0.0f, start, angle, true);
    g.setColour (UIThemeV5::accentGlow().withAlpha (0.82f));
    g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    auto p1 = c.getPointOnCircumference (radius * 0.18f, angle);
    auto p2 = c.getPointOnCircumference (radius * 0.69f, angle);
    g.setColour (juce::Colour::fromRGB (255, 246, 226));
    g.drawLine ({ p1, p2 }, 3.0f);

    auto valueArea = juce::Rectangle<float> (radius * 1.18f, 16.0f).withCentre ({ c.x, c.y + radius * 0.30f });
    UIThemeV5::drawValuePill (g, valueArea, formatKnobValue (slider), UIThemeV5::accentStrong(), 0.16f);
}
