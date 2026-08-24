#pragma once
#include <JuceHeader.h>
#include "UIThemeHW.h"

// =============================================================================
// HardwareEnvelope — oscilloscope-style ADSR envelope display
// Grid overlay, simple curve, filled area, no point markers
// =============================================================================
class HardwareEnvelope : public juce::Component
{
public:
    HardwareEnvelope();

    void updateFromADSR(float attack, float decay, float sustain, float release);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    float a = 0.18f, d = 0.28f, s = 0.48f, r = 0.22f;
};

// =============================================================================
// HardwareEnvelope implementation
// =============================================================================
HardwareEnvelope::HardwareEnvelope()
{
    setInterceptsMouseClicks(false, false);
}

void HardwareEnvelope::resized()
{
    // Layout handled by parent
}

void HardwareEnvelope::updateFromADSR(float attack, float decay, float sustain, float release)
{
    a = juce::jlimit(0.01f, 1.0f, attack);
    d = juce::jlimit(0.01f, 1.0f, decay);
    s = juce::jlimit(0.0f, 1.0f, sustain);
    r = juce::jlimit(0.01f, 1.0f, release);
    repaint();
}

void HardwareEnvelope::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // --- Background with inset look ---
    UIThemeHW::fillInset(g, b, UIThemeHW::insetRadius());

    // --- Oscilloscope grid ---
    g.setColour(UIThemeHW::textMuted().withAlpha(0.12f));

    // Vertical lines (10 divisions)
    for (int i = 0; i <= 10; ++i)
    {
        float x = b.getX() + (float)i / 10.0f * b.getWidth();
        g.drawLine(x, b.getY(), x, b.getBottom());
    }
    // Horizontal lines (6 divisions)
    for (int i = 0; i <= 6; ++i)
    {
        float y = b.getY() + (float)i / 6.0f * b.getHeight();
        g.drawLine(b.getX(), y, b.getRight(), y);
    }

    // --- Envelope curve ---
    const float padL = 6.0f;
    const float padT = 6.0f;
    const float padR = 6.0f;
    const float padB = 6.0f;

    const float x = b.getX() + padL;
    const float y = b.getY() + padT;
    const float w = b.getWidth() - padL - padR;
    const float h = b.getHeight() - padT - padB;

    // ADSR time proportions
    const float totalTime = a + d + 0.35f + r;
    if (totalTime < 0.01f) return;

    const float attackX  = x + (a / totalTime) * w * 0.88f;
    const float decayX   = x + ((a + d) / totalTime) * w * 0.88f;
    const float sustainX = x + w * 0.88f;
    const float releaseX = b.getRight() - padR;

    const float yTop = y;
    const float ySus = y + (1.0f - s) * h;
    const float yBot  = b.getBottom() - padB;

    // Control points for smooth bezier curves
    auto p0 = juce::Point<float>(x, yBot);
    auto p1 = juce::Point<float>(attackX, yTop);
    auto p2 = juce::Point<float>(decayX, ySus);
    auto p3 = juce::Point<float>(sustainX, ySus);
    auto p4 = juce::Point<float>(releaseX, yBot);

    // --- Fill below curve ---
    juce::Path fillPath;
    fillPath.startNewSubPath(p0);
    fillPath.lineTo(p1);
    fillPath.cubicTo(
        attackX + w * 0.06f, yTop - h * 0.12f,
        decayX - w * 0.05f, ySus + h * 0.08f,
        p2.x, p2.y);
    fillPath.lineTo(p3);
    fillPath.cubicTo(
        sustainX + w * 0.04f, ySus - h * 0.06f,
        releaseX - w * 0.04f, yBot - h * 0.08f,
        p4.x, p4.y);
    fillPath.lineTo(p0);
    fillPath.closeSubPath();

    // Gradient fill (fade from accent to transparent)
    g.setGradientFill(juce::ColourGradient(
        UIThemeHW::accentOrange().withAlpha(0.35f), b.getCentreX(), b.getY(),
        UIThemeHW::accentOrange().withAlpha(0.0f), b.getCentreX(), b.getBottom(), false));
    g.fillPath(fillPath);

    // --- Curve stroke with glow ---
    juce::Path curvePath;
    curvePath.startNewSubPath(p0);
    curvePath.lineTo(p1);
    curvePath.cubicTo(
        attackX + w * 0.06f, yTop - h * 0.12f,
        decayX - w * 0.05f, ySus + h * 0.08f,
        p2.x, p2.y);
    curvePath.lineTo(p3);
    curvePath.cubicTo(
        sustainX + w * 0.04f, ySus - h * 0.06f,
        releaseX - w * 0.04f, yBot - h * 0.08f,
        p4.x, p4.y);

    // Glow pass
    g.setColour(UIThemeHW::accentOrange().withAlpha(0.25f));
    g.strokePath(curvePath, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Main stroke
    g.setColour(UIThemeHW::accentOrange().withAlpha(0.9f));
    g.strokePath(curvePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // --- ADSR labels ---
    g.setColour(UIThemeHW::textMuted());
    g.setFont(UIThemeHW::smallFont());
    g.drawText("A", attackX - 2.0f, yBot + 2.0f, 12.0f, 10.0f, juce::Justification::topLeft);
    g.drawText("D", (attackX + decayX) * 0.5f, yBot + 2.0f, 12.0f, 10.0f, juce::Justification::topLeft);
    g.drawText("S", (decayX + sustainX) * 0.5f, yBot + 2.0f, 12.0f, 10.0f, juce::Justification::topLeft);
    g.drawText("R", (sustainX + releaseX) * 0.5f, yBot + 2.0f, 12.0f, 10.0f, juce::Justification::topLeft);
}