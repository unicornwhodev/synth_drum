#include "ToggleButtonComponentV5.h"

ToggleButtonComponentV5::ToggleButtonComponentV5 (juce::String text) : juce::TextButton (text)
{
    setClickingTogglesState (true);
}

void ToggleButtonComponentV5::paintButton (juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    auto area = getLocalBounds().toFloat().reduced (2.0f);
    bool on = getToggleState();

    const auto baseTop = on ? UIThemeV5::panelTop().brighter (0.12f)
                            : UIThemeV5::panelTop().brighter (isMouseOverButton ? 0.04f : 0.0f);
    const auto baseBottom = on ? UIThemeV5::panelBottom().brighter (0.08f)
                               : UIThemeV5::panelBottom();
    juce::ColourGradient grad (baseTop, area.getCentreX(), area.getY(),
                               baseBottom, area.getCentreX(), area.getBottom(), false);
    grad.addColour (0.45, UIThemeV5::bgMid());
    g.setGradientFill (grad);
    g.fillRoundedRectangle (area, 12.0f);

    g.setColour (juce::Colours::black.withAlpha (0.56f));
    g.drawRoundedRectangle (area, 12.0f, 1.4f);

    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawRoundedRectangle (area.reduced (1.0f), 11.0f, 0.7f);

    auto stateDot = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ area.getX() + 14.0f, area.getCentreY() });
    g.setColour ((on ? UIThemeV5::accentStrong() : UIThemeV5::textMuted()).withAlpha (isButtonDown ? 0.8f : 1.0f));
    g.fillEllipse (stateDot);

    if (on)
    {
        g.setColour (UIThemeV5::accentGlow().withAlpha (0.42f));
        g.drawEllipse (stateDot.expanded (4.0f), 1.2f);
    }

    if (on)
    {
        auto glow = area.reduced (16.0f, 0.0f).removeFromBottom (4.0f);
        UIThemeV5::drawGlowStrip (g, glow, 2.0f, 0.92f);
    }

    g.setColour (UIThemeV5::textMain().withAlpha (isButtonDown ? 0.82f : 1.0f));
    g.setFont (UIThemeV5::labelFont());
    g.drawText (getButtonText(), getLocalBounds().reduced (22, 0), juce::Justification::centred);
}
