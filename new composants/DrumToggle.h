#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumToggle — Hardware dark toggle switch with LED indicator
// =============================================================================
class DrumToggle : public juce::TextButton
{
public:
    explicit DrumToggle(juce::String text, juce::Colour accentCol = UITheme::accent());
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

    // LED indicator (fixed left slot, so compact labels keep their full text area).
    const float ledSlotW = juce::jlimit(16.0f, 22.0f, b.getHeight() * 0.72f);
    auto ledSlot = b.removeFromLeft(ledSlotW);
    UITheme::drawLED(g, ledSlot.getCentre(), juce::jlimit(2.0f, 3.0f, ledSlotW * 0.13f),
                     on, accentColour, UITheme::ledOff());
    b.removeFromLeft(2.0f);

    // Text
    g.setColour(on ? UITheme::textMain() : UITheme::textDim());
    g.setFont(getWidth() < 86 ? UITheme::fontSmall().withHeight(9.0f) : UITheme::fontLabel());
    g.drawFittedText(getButtonText(), b.reduced(3.0f, 0.0f).toNearestInt(),
                     juce::Justification::centred, 1, 0.85f);

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
