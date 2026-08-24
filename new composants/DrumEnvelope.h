#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumEnvelope — ADSR envelope display with control points
// =============================================================================
class DrumEnvelope : public juce::Component
{
public:
    DrumEnvelope();

    void updateFromADSR(float attack, float decay, float sustain, float release);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    float a = 0.18f, d = 0.28f, s = 0.48f, r = 0.22f;
};

// =============================================================================
// Implementation
// =============================================================================
inline DrumEnvelope::DrumEnvelope()
{
    setInterceptsMouseClicks(false, false);
}

inline void DrumEnvelope::resized()
{
    // Layout handled by parent
}

inline void DrumEnvelope::updateFromADSR(float attack, float decay, float sustain, float release)
{
    a = juce::jlimit(0.01f, 1.0f, attack);
    d = juce::jlimit(0.01f, 1.0f, decay);
    s = juce::jlimit(0.0f, 1.0f, sustain);
    r = juce::jlimit(0.01f, 1.0f, release);
    repaint();
}

inline void DrumEnvelope::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Background inset
    UITheme::fillInset(g, b, UITheme::panelInset(), UITheme::cornerRadiusSmall());

    auto plot = b.reduced(8.0f);

    // Grid
    g.setColour(UITheme::textMuted().withAlpha(0.15f));
    for (int i = 0; i <= 8; ++i)
    {
        float x = plot.getX() + (float)i / 8.0f * plot.getWidth();
        g.drawVerticalLine(juce::roundToInt(x), plot.getY(), plot.getBottom());
    }
    for (int i = 0; i <= 4; ++i)
    {
        float y = plot.getY() + (float)i / 4.0f * plot.getHeight();
        g.drawHorizontalLine(juce::roundToInt(y), plot.getX(), plot.getRight());
    }

    // ADSR points
    const float totalTime = a + d + 0.35f + r;
    if (totalTime < 0.01f) return;

    const float attackX  = plot.getX() + (a / totalTime) * plot.getWidth() * 0.88f;
    const float decayX   = plot.getX() + ((a + d) / totalTime) * plot.getWidth() * 0.88f;
    const float sustainX = plot.getX() + plot.getWidth() * 0.88f;
    const float releaseX = plot.getRight() - 4.0f;

    const float yTop = plot.getY() + 4.0f;
    const float ySus = plot.getY() + (1.0f - s) * (plot.getHeight() - 8.0f) + 4.0f;
    const float yBot = plot.getBottom() - 4.0f;

    auto p0 = juce::Point<float>(plot.getX(), yBot);
    auto p1 = juce::Point<float>(attackX, yTop);
    auto p2 = juce::Point<float>(decayX, ySus);
    auto p3 = juce::Point<float>(sustainX, ySus);
    auto p4 = juce::Point<float>(releaseX, yBot);

    // Fill under curve
    juce::Path fillPath;
    fillPath.startNewSubPath(p0);
    fillPath.cubicTo(p0.translated((p1.x - p0.x) * 0.5f, 0.0f), p1.translated(0.0f, (p0.y - p1.y) * 0.3f), p1);
    fillPath.lineTo(p2);
    fillPath.cubicTo(p2.translated((p3.x - p2.x) * 0.5f, 0.0f), p3.translated(0.0f, 0.0f), p3);
    fillPath.lineTo(p4);
    fillPath.lineTo(p0);

    g.setColour(UITheme::accentOrange().withAlpha(0.12f));
    g.fillPath(fillPath);

    // Curve line
    juce::Path curve;
    curve.startNewSubPath(p0);
    curve.cubicTo(p0.translated((p1.x - p0.x) * 0.5f, 0.0f), p1.translated(0.0f, (p0.y - p1.y) * 0.3f), p1);
    curve.lineTo(p2);
    curve.cubicTo(p2.translated((p3.x - p2.x) * 0.5f, 0.0f), p3.translated(0.0f, 0.0f), p3);
    curve.lineTo(p4);

    g.setColour(UITheme::accentOrange().withAlpha(0.85f));
    g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Control points
    auto drawPoint = [&](juce::Point<float> p, const juce::String& label)
    {
        g.setColour(UITheme::accentOrange());
        g.fillEllipse(p.x - 3.0f, p.y - 3.0f, 6.0f, 6.0f);
        g.setColour(UITheme::textMain());
        g.setFont(UITheme::fontMicro());
        constexpr float labelW = 20.0f;
        constexpr float labelH = 10.0f;
        const float labelX = juce::jlimit(plot.getX(), plot.getRight() - labelW, p.x - labelW * 0.5f);
        const float labelY = juce::jlimit(plot.getY(), plot.getBottom() - labelH, p.y - 14.0f);
        g.drawText(label, juce::Rectangle<float>(labelX, labelY, labelW, labelH),
                   juce::Justification::centred, false);
    };

    drawPoint(p1, "A");
    drawPoint(p2, "D");
    drawPoint(p3, "S");
    drawPoint(p4, "R");
}
