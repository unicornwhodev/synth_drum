#pragma once
#include <JuceHeader.h>
#include "UIThemeHW.h"

// =============================================================================
// HardwarePad — hardware-style drum pad
// Raised bevel button with LED indicator and activity meter
// =============================================================================
class HardwarePad : public juce::Component
{
public:
    HardwarePad();

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
    juce::Colour catColour = UIThemeHW::ledOff();
    bool selected = false;
    bool hovered = false;
    bool pressed = false;
    float activity = 0.0f;
    float flashAlpha = 0.0f;
};

// =============================================================================
// HardwarePad implementation
// =============================================================================
inline HardwarePad::HardwarePad()
{
    setInterceptsMouseClicks(true, false);
}

inline void HardwarePad::configure(int index, const juce::String& padName, juce::Colour colour)
{
    idx = index;
    name = padName;
    catColour = colour;
    repaint();
}

inline void HardwarePad::setSelected(bool s)
{
    if (selected != s)
    {
        selected = s;
        repaint();
    }
}

inline void HardwarePad::setActivityLevel(float linear)
{
    if (std::abs(activity - linear) > 0.005f)
    {
        activity = linear;
        repaint();
    }
}

inline void HardwarePad::flash()
{
    flashAlpha = 1.0f;
    repaint();
}

inline void HardwarePad::tickFlash()
{
    if (flashAlpha > 0.0f)
    {
        flashAlpha *= 0.82f;
        if (flashAlpha < 0.01f)
            flashAlpha = 0.0f;
        repaint();
    }
}

inline void HardwarePad::resized()
{
    // Layout handled by parent in resized()
}

inline void HardwarePad::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float radius = UIThemeHW::panelRadius();

    // === Background fill ===
    g.setColour(UIThemeHW::panelGrey().withBrightness(selected ? 0.22f : 0.18f));
    g.fillRoundedRectangle(b, radius);

    // === Bevel highlight (top-left light source) ===
    g.setColour(juce::Colours::white.withAlpha(selected ? 0.12f : 0.08f));
    g.fillRoundedRectangle(b.withTrimmedBottom(b.getHeight() * 0.6f).withTrimmedRight(b.getWidth() * 0.6f), radius);

    // === Bevel shadow (bottom-right) ===
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(b.withTrimmedTop(b.getHeight() * 0.6f).withTrimmedLeft(b.getWidth() * 0.6f), radius);

    // === Border ===
    g.setColour(selected ? catColour.withAlpha(0.8f) : juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(b, radius, selected ? 1.5f : 1.0f);

    // === Pad number (top-left corner) ===
    g.setColour(selected ? catColour : UIThemeHW::textDim());
    g.setFont(juce::Font("Arial", 10.0f, juce::Font::bold));
    g.drawText(juce::String(idx + 1), b.reduced(6.0f, 4.0f), juce::Justification::topLeft);

    // === Pad name (centered) ===
    g.setColour(selected ? UIThemeHW::textWhite() : UIThemeHW::textDim());
    g.setFont(juce::Font("Arial", 11.5f, juce::Font::bold));
    g.drawText(name, b.reduced(4.0f, 0.0f).removeFromTop(b.getHeight() * 0.5f + 4.0f), juce::Justification::centred);

    // === Category LED dot (top-right area) ===
    auto ledCenter = juce::Point<float>(b.getRight() - 10.0f, b.getY() + 10.0f);
    UIThemeHW::drawLED(g, ledCenter, 4.0f, selected, catColour, UIThemeHW::ledOff());

    // === Activity LED strip (bottom of pad) ===
    auto track = b.reduced(8.0f, 4.0f);
    track = track.withY(b.getBottom() - 12.0f).withHeight(6.0f);

    // Track background
    g.setColour(UIThemeHW::panelInset());
    g.fillRoundedRectangle(track, 1.5f);

    if (activity > 0.01f)
    {
        auto activeWidth = track.getWidth() * activity;
        auto activeTrack = track.withWidth(activeWidth);

        // Gradient fill based on activity level
        juce::Colour activityCol;
        if (activity < 0.6f)
            activityCol = catColour;
        else if (activity < 0.85f)
            activityCol = UIThemeHW::ledAmber();
        else
            activityCol = UIThemeHW::ledRed();

        g.setColour(activityCol.withAlpha(0.85f));
        g.fillRoundedRectangle(activeTrack, 1.5f);

        // Highlight on active portion
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.fillRoundedRectangle(activeTrack.withHeight(2.5f).withY(activeTrack.getY()), 1.5f);
    }

    if (flashAlpha > 0.0f)
    {
        g.setColour(catColour.withAlpha(0.22f * flashAlpha));
        g.fillRoundedRectangle(b.reduced(2.0f), radius);
    }

    // === Hover/press overlay ===
    if (hovered && !pressed)
    {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRoundedRectangle(b, radius);
    }
    if (pressed)
    {
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillRoundedRectangle(b, radius);
    }
}

inline void HardwarePad::mouseDown(const juce::MouseEvent&)
{
    pressed = true;
    repaint();
}

inline void HardwarePad::mouseUp(const juce::MouseEvent&)
{
    pressed = false;
    if (hovered && onClicked)
        onClicked(idx);
    repaint();
}

inline void HardwarePad::mouseEnter(const juce::MouseEvent&)
{
    hovered = true;
    repaint();
}

inline void HardwarePad::mouseExit(const juce::MouseEvent&)
{
    hovered = false;
    pressed = false;
    repaint();
}