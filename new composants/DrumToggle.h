#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumToggle — Hardware dark toggle switch with LED indicator
// =============================================================================
class DrumToggle : public juce::TextButton
{
public:
    explicit DrumToggle(juce::String text, juce::Colour accentCol = UITheme::accentOrange());
    ~DrumToggle() override = default;

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;

private:
    juce::Colour accentColour;
};

// =============================================================================
// Implementation
// =============================================================================
inline DrumToggle::DrumToggle(juce::String text, juce::Colour ac)
    : accentColour(ac)
{
    setButtonText(text);
    setClickingTogglesState(true);
}

inline void DrumToggle::paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    auto b = getLocalBounds().toFloat().reduced(0.5f);
    bool on = getToggleState();

    // Background
    UITheme::fillPanel(g, b, on ? UITheme::panelActive() : UITheme::panelBase(),
                       UITheme::cornerRadiusSmall());

    // LED indicator (left side)
    auto ledArea = b.removeFromLeft(b.getHeight()).reduced(6.0f);
    UITheme::drawLED(g, ledArea.getCentre(), ledArea.getWidth() * 0.22f, on, accentColour, UITheme::ledOff());

    // Text
    g.setColour(on ? UITheme::textMain() : UITheme::textDim());
    g.setFont(UITheme::fontLabel());
    g.drawText(getButtonText(), b.reduced(4.0f, 0.0f), juce::Justification::centredLeft, false);

    // Press overlay
    if (isButtonDown)
    {
        g.setColour(juce::Colours::black.withAlpha(0.12f));
        g.fillRoundedRectangle(b, UITheme::cornerRadiusSmall());
    }

    // Hover glow
    if (isMouseOverButton && !on)
    {
        g.setColour(accentColour.withAlpha(0.06f));
        g.fillRoundedRectangle(b, UITheme::cornerRadiusSmall());
    }
}
