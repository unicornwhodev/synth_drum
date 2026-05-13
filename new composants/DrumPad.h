#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumPad — Modern hardware drum pad with category colour, velocity bar, flash
// =============================================================================
class DrumPad : public juce::Component
{
public:
    DrumPad();

    void configure(int index, const juce::String& padName, juce::Colour catColour);
    void setSelected(bool s);
    void setActivityLevel(float linear);
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
    bool selected  = false;
    bool hovered   = false;
    bool pressed   = false;
    float activity   = 0.0f;
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
    idx = index;  name = padName;  catColour = colour;  repaint();
}

inline void DrumPad::setSelected(bool s)
{
    if (selected != s) { selected = s; repaint(); }
}

inline void DrumPad::setActivityLevel(float linear)
{
    activity = juce::jlimit(0.0f, 1.0f, linear);  repaint();
}

inline void DrumPad::flash()     { flashAlpha = 1.0f; repaint(); }

inline void DrumPad::tickFlash()
{
    if (flashAlpha > 0.0f) { flashAlpha = UITheme::flashDecay(flashAlpha, 0.80f); repaint(); }
}

inline void DrumPad::resized() {}

inline void DrumPad::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float R = 10.0f;

    // ── Drop shadow ────────────────────────────────────────────────────────────
    g.setColour(juce::Colours::black.withAlpha(0.50f));
    g.fillRoundedRectangle(b.translated(0.0f, 3.0f).reduced(2.0f), R + 1.0f);

    // ── Body fill — category colour bleeds in when selected / hovered ─────────
    const float bleed = selected ? 0.18f : (hovered ? 0.07f : 0.04f);
    juce::ColourGradient bodyGrad(
        catColour.withAlpha(bleed).overlaidWith(UITheme::bgElevated()),
        b.getCentreX(), b.getY(),
        UITheme::bgBase(),
        b.getCentreX(), b.getBottom(), false);
    g.setGradientFill(bodyGrad);
    g.fillRoundedRectangle(b, R);

    // ── Border ────────────────────────────────────────────────────────────────
    if (selected)
    {
        g.setColour(catColour.withAlpha(0.20f));                          // outer halo
        g.drawRoundedRectangle(b.expanded(1.5f), R + 1.5f, 3.0f);
        g.setColour(catColour.withAlpha(0.95f));                          // main border
        g.drawRoundedRectangle(b.reduced(0.75f), R, 1.5f);
        g.setColour(catColour.withAlpha(0.18f));                          // inner highlight
        g.drawRoundedRectangle(b.reduced(2.5f), R - 1.5f, 1.0f);
    }
    else if (hovered)
    {
        g.setColour(catColour.withAlpha(0.38f));
        g.drawRoundedRectangle(b.reduced(0.75f), R, 1.5f);
    }
    else
    {
        g.setColour(UITheme::borderSubtle());
        g.drawRoundedRectangle(b.reduced(0.5f), R, 1.0f);
    }

    // ── Left accent strip ─────────────────────────────────────────────────────
    auto strip = b.reduced(1.5f).removeFromLeft(4.0f)
                  .withTrimmedTop(10.0f).withTrimmedBottom(10.0f);
    g.setColour(catColour.withAlpha(selected ? 1.0f : 0.35f));
    g.fillRoundedRectangle(strip, 2.0f);

    // ── Pad number (top-left) ─────────────────────────────────────────────────
    g.setColour(selected ? catColour : UITheme::textMuted());
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 9.5f, juce::Font::bold));
    g.drawText(juce::String::formatted("%02d", idx + 1),
               b.reduced(12.0f, 6.0f).removeFromTop(14.0f),
               juce::Justification::topLeft);

    // ── Category LED (top-right) ──────────────────────────────────────────────
    UITheme::drawLED(g,
                     juce::Point<float>(b.getRight() - 13.0f, b.getY() + 13.0f),
                     4.5f,
                     selected || flashAlpha > 0.4f,
                     catColour, UITheme::panelInset());

    // ── Pad name (centred) ────────────────────────────────────────────────────
    const float nameSize = juce::jlimit(10.0f, 15.0f, b.getHeight() * 0.125f);
    g.setColour(selected ? UITheme::textMain() : UITheme::textDim());
    g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), nameSize, juce::Font::bold));
    g.drawText(name,
               b.withTrimmedTop(b.getHeight() * 0.26f)
                .withTrimmedBottom(b.getHeight() * 0.28f),
               juce::Justification::centred);

    // ── Velocity / activity bar (bottom) ─────────────────────────────────────
    const float barH = juce::jmax(4.0f, b.getHeight() * 0.055f);
    auto track = b.reduced(12.0f, 0.0f)
                  .withY(b.getBottom() - barH - 9.0f)
                  .withHeight(barH);

    g.setColour(UITheme::panelInset());
    g.fillRoundedRectangle(track, barH * 0.5f);

    if (activity > 0.01f)
    {
        auto fill   = track.withWidth(track.getWidth() * activity);
        auto barCol = activity < 0.65f ? catColour
                    : activity < 0.85f ? UITheme::accentAmber()
                                       : UITheme::accentRed();
        g.setColour(barCol.withAlpha(0.92f));
        g.fillRoundedRectangle(fill, barH * 0.5f);
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.fillRoundedRectangle(fill.withHeight(barH * 0.45f), barH * 0.5f);
    }

    // ── Trigger flash ─────────────────────────────────────────────────────────
    if (flashAlpha > 0.01f)
    {
        g.setColour(catColour.withAlpha(0.40f * flashAlpha));
        g.fillRoundedRectangle(b.reduced(2.0f), R - 1.0f);
        if (flashAlpha > 0.35f)                                           // outer halo
        {
            g.setColour(catColour.withAlpha(0.12f * flashAlpha));
            g.fillRoundedRectangle(b.expanded(5.0f * flashAlpha), R + 4.0f);
        }
    }

    // ── Press darkening ───────────────────────────────────────────────────────
    if (pressed)
    {
        g.setColour(juce::Colours::black.withAlpha(0.20f));
        g.fillRoundedRectangle(b, R);
    }
}

inline void DrumPad::mouseDown (const juce::MouseEvent&) { pressed = true;  repaint(); }
inline void DrumPad::mouseUp   (const juce::MouseEvent&)
{
    pressed = false;
    if (hovered && onClicked) onClicked(idx);
    repaint();
}
inline void DrumPad::mouseEnter(const juce::MouseEvent&) { hovered = true;  repaint(); }
inline void DrumPad::mouseExit (const juce::MouseEvent&) { hovered = pressed = false; repaint(); }
