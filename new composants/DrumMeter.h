#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumMeter — Horizontal segmented LED ladder meter
// =============================================================================
class DrumMeter : public juce::Component, private juce::Timer
{
public:
    DrumMeter();

    void setLevels(float left, float right);  // 0..1 linear
    void setSingleChannel(bool single) { singleChannel = single; repaint(); }
    void reset();

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    static constexpr int numSegments = 20;

    float targetL = 0.0f, targetR = 0.0f;
    float displayL = 0.0f, displayR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;
    bool singleChannel = false;
};

// =============================================================================
// Implementation
// =============================================================================
inline DrumMeter::DrumMeter()
{
    setInterceptsMouseClicks(false, false);
    startTimerHz(60);
}

inline void DrumMeter::resized()
{
    // Layout handled by parent
}

inline void DrumMeter::setLevels(float left, float right)
{
    targetL = juce::jlimit(0.0f, 1.0f, left);
    targetR = juce::jlimit(0.0f, 1.0f, right);
    if (targetL > peakL) peakL = targetL;
    if (targetR > peakR) peakR = targetR;
}

inline void DrumMeter::reset()
{
    targetL = targetR = displayL = displayR = peakL = peakR = 0.0f;
    repaint();
}

inline void DrumMeter::timerCallback()
{
    bool changed = false;

    auto newL = UITheme::smoothDecay(displayL, targetL, 0.88f);
    auto newR = UITheme::smoothDecay(displayR, targetR, 0.88f);

    if (std::abs(newL - displayL) > 0.001f || std::abs(newR - displayR) > 0.001f)
    {
        displayL = newL;
        displayR = newR;
        changed = true;
    }

    // Peak hold decay
    peakL *= 0.992f;
    peakR *= 0.992f;
    if (peakL < 0.001f) peakL = 0.0f;
    if (peakR < 0.001f) peakR = 0.0f;

    if (changed)
        repaint();
}

inline void DrumMeter::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    // Background track
    UITheme::fillInset(g, area, UITheme::panelInset(), UITheme::cornerRadiusSmall());

    if (singleChannel)
    {
        UITheme::drawLEDStrip(g, area.reduced(3.0f), displayL, numSegments, true);

        // Peak indicator
        if (peakL > 0.01f)
        {
            auto peakX = area.getX() + 3.0f + (area.getWidth() - 6.0f) * peakL;
            g.setColour(UITheme::accentRed().withAlpha(0.7f));
            g.fillRect(peakX - 1.0f, area.getY() + 2.0f, 2.0f, area.getHeight() - 4.0f);
        }
    }
    else
    {
        // Stereo: two rows
        auto top = area.removeFromTop(area.getHeight() * 0.48f);
        auto bottom = area.removeFromBottom(area.getHeight() * 0.9f);

        UITheme::drawLEDStrip(g, top.reduced(3.0f, 1.0f), displayL, numSegments / 2, true);
        UITheme::drawLEDStrip(g, bottom.reduced(3.0f, 1.0f), displayR, numSegments / 2, true);
    }
}
