#include "OutputMeterComponentV5.h"

void OutputMeterComponentV5::setValue (float normalized)
{
    value = juce::jlimit (0.0f, 1.0f, normalized);
    repaint();
}

void OutputMeterComponentV5::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    UIThemeV5::fillPanel (g, area, 18.0f);

    auto inner = area.reduced (16.0f, 16.0f);
    UIThemeV5::fillRecess (g, inner, 13.0f);

    auto ruler = inner.reduced (22.0f, 18.0f);
    auto header = ruler.removeFromTop (17.0f);

    static constexpr int dbMarks[] = { -60, -36, -24, -12, -6, 0 };
    for (int db : dbMarks)
    {
        float t = juce::jmap ((float) db, -60.0f, 0.0f, 0.0f, 1.0f);
        float x = juce::jmap (t, ruler.getX(), ruler.getRight());
        g.setColour (UIThemeV5::textDim());
        g.setFont (UIThemeV5::smallFont());
        g.drawText (juce::String (db), (int) x - 14, (int) header.getY(), 28, 14, juce::Justification::centred);

        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawVerticalLine ((int) x, ruler.getY() + 2.0f, ruler.getBottom());
    }

    auto bars = ruler.withTrimmedTop (18.0f).withHeight (22.0f);
    const int segs = 24;
    const float gap = 3.0f;
    const float segW = (bars.getWidth() - gap * (segs - 1)) / segs;
    const int lit = juce::roundToInt (value * segs);

    for (int i = 0; i < segs; ++i)
    {
        auto seg = juce::Rectangle<float> (bars.getX() + i * (segW + gap), bars.getY(), segW, bars.getHeight());
        bool active = i < lit;
        juce::Colour activeCol = UIThemeV5::accentStrong();
        if (i > segs * 0.8f)
            activeCol = juce::Colour::fromRGB (255, 96, 76);
        else if (i > segs * 0.6f)
            activeCol = juce::Colour::fromRGB (255, 178, 64);

        g.setColour (active ? activeCol : juce::Colour::fromRGB (34, 41, 52));
        g.fillRoundedRectangle (seg, 2.0f);

        if (active)
        {
            g.setColour (activeCol.withAlpha (0.34f));
            g.drawRoundedRectangle (seg, 2.0f, 0.8f);
        }
    }

    auto peakTag = juce::Rectangle<float> (52.0f, 16.0f).withCentre ({ inner.getRight() - 40.0f, inner.getY() + 12.0f });
    UIThemeV5::drawValuePill (g, peakTag, juce::String (juce::roundToInt (value * 100.0)) + "%", UIThemeV5::accentStrong(), 0.14f);
}
