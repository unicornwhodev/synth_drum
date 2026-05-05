#include "SelectorComponentV5.h"

SelectorComponentV5::SelectorComponentV5()
{
    setTextWhenNothingSelected ("Select module");

    setColour (juce::ComboBox::textColourId, UIThemeV5::textMain());
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
}

void SelectorComponentV5::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    UIThemeV5::fillRecess (g, area, 12.0f);

    auto topRail = area.reduced (12.0f, 8.0f).removeFromTop (2.0f);
    UIThemeV5::drawGlowStrip (g, topRail, 1.0f, 0.42f);

    g.setColour (UIThemeV5::textMain());
    g.setFont (UIThemeV5::labelFont());
    g.drawText (getText(), getLocalBounds().reduced (14, 0), juce::Justification::centredLeft);

    juce::Path arrow;
    float cx = (float) getWidth() - 18.0f;
    float cy = (float) getHeight() * 0.5f;
    arrow.startNewSubPath (cx - 5.0f, cy - 2.0f);
    arrow.lineTo (cx, cy + 3.0f);
    arrow.lineTo (cx + 5.0f, cy - 2.0f);

    g.setColour (UIThemeV5::accentStrong());
    g.strokePath (arrow, juce::PathStrokeType (2.0f));
}
