#pragma once
#include <JuceHeader.h>
#include "UIThemeHW.h"

// =============================================================================
// HardwareMeter — segmented LED ladder meter, like hardware VU meters
// Green -> Amber -> Red gradient, dB labels, stereo with center gap
// =============================================================================
class HardwareMeter : public juce::Component
{
public:
    HardwareMeter();

    void setLevels(float left, float right);  // 0..1 linear
    void setRefLevel(float ref) { refLevel = ref; } // 0..1 default 0.7
    void reset();

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int numLEDs = 16;

    float leftLevel = 0.0f;
    float rightLevel = 0.0f;
    float refLevel = 0.7f;  // 0 dB reference

    void drawLEDLadder(juce::Graphics& g, const juce::Rectangle<float>& area,
                       float level, bool showRef = true);
};

// =============================================================================
// HardwareMeter implementation
// =============================================================================
HardwareMeter::HardwareMeter()
{
    setInterceptsMouseClicks(false, false);
}

void HardwareMeter::resized()
{
    // Layout handled by parent
}

void HardwareMeter::setLevels(float left, float right)
{
    if (std::abs(leftLevel - left) > 0.003f || std::abs(rightLevel - right) > 0.003f)
    {
        leftLevel = left;
        rightLevel = right;
        repaint();
    }
}

void HardwareMeter::reset()
{
    leftLevel = rightLevel = 0.0f;
    repaint();
}

void HardwareMeter::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    const float gap = 4.0f;
    auto width = (area.getWidth() - gap) * 0.5f;

    // Left channel
    auto leftRect = area.withWidth(width).withX(area.getX());
    drawLEDLadder(g, leftRect, leftLevel);

    // Right channel
    auto rightRect = area.withWidth(width).withX(area.getRight() - width);
    drawLEDLadder(g, rightRect, rightLevel);
}

void HardwareMeter::drawLEDLadder(juce::Graphics& g, const juce::Rectangle<float>& area, float level)
{
    const float ledH = 4.0f;
    const float gap = 1.5f;
    const float totalH = numLEDs * ledH + (numLEDs - 1) * gap;
    const float startY = area.getCentreY() - totalH * 0.5f;

    // Channel label background strip
    g.setColour(UIThemeHW::panelDark());
    g.fillRect(area);

    // dB scale labels on left side
    g.setColour(UIThemeHW::textMuted());
    g.setFont(UIThemeHW::smallFont());
    const juce::String dbLabels[] = { "+6", "0", "-6", "-12", "-24", "-48" };
    for (int i = 0; i < 6; ++i)
    {
        float t = (float)i / 5.0f;
        float y = startY + t * totalH;
        g.drawText(dbLabels[i], area.getX(), y - 4.0f, 14.0f, 10.0f, juce::Justification::topRight);
    }

    // LED track area (right portion of channel)
    auto ledArea = area.reduced(18.0f, 2.0f);

    for (int i = 0; i < numLEDs; ++i)
    {
        // LEDs from bottom (0) to top (numLEDs-1), but display top to bottom visually
        // We invert so green is at bottom (low level), red at top (high level)
        int ledIdx = numLEDs - 1 - i;
        float threshold = (float)ledIdx / (float)(numLEDs - 1);
        bool active = level > threshold;

        // Colour: green for <60%, amber for 60-85%, red for >85%
        juce::Colour col;
        if (threshold < 0.6f)       col = UIThemeHW::ledGreen();
        else if (threshold < 0.85f) col = UIThemeHW::ledAmber();
        else                       col = UIThemeHW::ledRed();

        auto ledRect = juce::Rectangle<float>(
            ledArea.getX(),
            startY + i * (ledH + gap),
            ledArea.getWidth(),
            ledH
        ).reduced(0.5f, 0.0f);

        if (active)
        {
            // Glow
            g.setColour(col.withAlpha(0.3f));
            g.fillRect(ledRect.expanded(2.0f, 1.0f));

            // LED body
            g.setColour(col);
            g.fillRoundedRectangle(ledRect, 1.0f);

            // Highlight
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.fillRoundedRectangle(ledRect.withHeight(ledH * 0.4f).withY(ledRect.getY()), 1.0f);
        }
        else
        {
            // Inactive LED
            g.setColour(col.withAlpha(0.12f));
            g.fillRoundedRectangle(ledRect, 1.0f);
        }
    }

    // Reference level marker line (0 dB)
    float refY = startY + this->refLevel * totalH;
    g.setColour(UIThemeHW::ledAmber().withAlpha(0.6f));
    g.fillRect(ledArea.withTop(refY - 1.0f).withBottom(refY + 1.0f));
}