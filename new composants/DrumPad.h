#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumPad — Hardware dark drum pad with category colours and activity flash
// =============================================================================
class DrumPad : public juce::Component
{
public:
    DrumPad();

    void configure(int index, const juce::String& padName, juce::Colour catColour);
    void setSelected(bool s);
    void setActivityLevel(float linear);  // 0..1 for mini LED meter
    void flash();
    void tickFlash();

    std::function<void(int)> onClicked;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    int idx = 0;
    juce::String name;
    juce::Colour catColour = UITheme::ledOff();
    bool selected = false;
    bool hovered = false;
    bool pressed = false;
    float activity = 0.0f;
    float flashAlpha = 0.0f;
};

// =============================================================================
// Implementation
// =============================================================================
inline DrumPad::DrumPad()
{
    setInterceptsMouseClicks(true, false);
}

inline void DrumPad::configure(int index, const juce::String& padName, juce::Colour colour)
{
    idx = index;
    name = padName;
    catColour = colour;
    repaint();
}

inline void DrumPad::setSelected(bool s)
{
    if (selected != s)
    {
        selected = s;
        repaint();
    }
}

inline void DrumPad::setActivityLevel(float linear)
{
    if (std::abs(activity - linear) > 0.005f)
    {
        activity = linear;
        repaint();
    }
}

inline void DrumPad::flash()
{
    flashAlpha = 1.0f;
    repaint();
}

inline void DrumPad::tickFlash()
{
    if (flashAlpha > 0.0f)
    {
        flashAlpha = UITheme::flashDecay(flashAlpha);
        repaint();
    }
}

inline void DrumPad::resized()
{
    // Layout handled by parent
}

inline void DrumPad::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float radius = UITheme::cornerRadius();

    // === Background ===
    auto bgCol = selected ? UITheme::panelActive() : UITheme::panelBase();
    if (hovered && !selected)
        bgCol = UITheme::panelHover();

    g.setColour(bgCol);
    g.fillRoundedRectangle(b, radius);

    // === Selected border ===
    if (selected)
    {
        g.setColour(catColour.withAlpha(0.7f));
        g.drawRoundedRectangle(b.reduced(0.5f), radius, 2.0f);

        // Subtle glow
        g.setColour(catColour.withAlpha(0.08f));
        g.fillRoundedRectangle(b.reduced(2.0f), radius);
    }
    else
    {
        g.setColour(UITheme::borderSubtle());
        g.drawRoundedRectangle(b.reduced(0.5f), radius, 1.0f);
    }

    // === Pad number (top-left) ===
    g.setColour(selected ? catColour : UITheme::textMuted());
    g.setFont(UITheme::fontSmall());
    g.drawText(juce::String(idx + 1), b.reduced(6.0f, 4.0f), juce::Justification::topLeft);

    // === Category LED dot (top-right) ===
    auto ledCentre = juce::Point<float>(b.getRight() - 10.0f, b.getY() + 10.0f);
    UITheme::drawLED(g, ledCentre, 3.0f, selected, catColour, UITheme::ledOff());

    // === Pad name (centered) ===
    g.setColour(selected ? UITheme::textMain() : UITheme::textDim());
    g.setFont(UITheme::fontLabel());
    g.drawText(name, b.reduced(4.0f, 0.0f).removeFromTop(b.getHeight() * 0.55f + 6.0f),
               juce::Justification::centred);

    // === Activity bar (bottom) ===
    auto track = b.reduced(8.0f, 4.0f);
    track = track.withY(b.getBottom() - 10.0f).withHeight(5.0f);

    // Track background
    g.setColour(UITheme::panelInset());
    g.fillRoundedRectangle(track, 1.5f);

    if (activity > 0.01f)
    {
        auto activeWidth = track.getWidth() * activity;
        auto activeTrack = track.withWidth(activeWidth);

        juce::Colour activityCol;
        if (activity < 0.6f)
            activityCol = catColour;
        else if (activity < 0.85f)
            activityCol = UITheme::accentAmber();
        else
            activityCol = UITheme::accentRed();

        g.setColour(activityCol.withAlpha(0.85f));
        g.fillRoundedRectangle(activeTrack, 1.5f);

        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.fillRoundedRectangle(activeTrack.withHeight(2.0f).withY(activeTrack.getY()), 1.5f);
    }

    // === Flash overlay ===
    if (flashAlpha > 0.0f)
    {
        g.setColour(catColour.withAlpha(0.25f * flashAlpha));
        g.fillRoundedRectangle(b.reduced(1.0f), radius);
    }

    // === Press overlay ===
    if (pressed)
    {
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.fillRoundedRectangle(b, radius);
    }
}

inline void DrumPad::mouseDown(const juce::MouseEvent&)
{
    pressed = true;
    repaint();
}

inline void DrumPad::mouseUp(const juce::MouseEvent&)
{
    pressed = false;
    if (hovered && onClicked)
        onClicked(idx);
    repaint();
}

inline void DrumPad::mouseEnter(const juce::MouseEvent&)
{
    hovered = true;
    repaint();
}

inline void DrumPad::mouseExit(const juce::MouseEvent&)
{
    hovered = false;
    pressed = false;
    repaint();
}
