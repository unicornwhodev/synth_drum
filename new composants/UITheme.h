#pragma once
#include <JuceHeader.h>

// =============================================================================
// UITheme — Unified Hardware Dark Theme for UWdeVST Drum Synth
// =============================================================================
namespace UITheme
{
    // =========================================================================
    // Colour Palette
    // =========================================================================

    // Background hierarchy
    inline juce::Colour bgDeep()      { return juce::Colour::fromRGB(10, 10, 14); }
    inline juce::Colour bgBase()      { return juce::Colour::fromRGB(16, 16, 22); }
    inline juce::Colour bgElevated()  { return juce::Colour::fromRGB(24, 24, 32); }

    // Panel surfaces
    inline juce::Colour panelBase()   { return juce::Colour::fromRGB(30, 31, 40); }
    inline juce::Colour panelHover()  { return juce::Colour::fromRGB(38, 39, 50); }
    inline juce::Colour panelActive() { return juce::Colour::fromRGB(44, 46, 58); }
    inline juce::Colour panelInset()  { return juce::Colour::fromRGB(14, 14, 20); }

    // Accent colours — section-based
    inline juce::Colour accentOrange()  { return juce::Colour::fromRGB(255, 107, 53); }
    inline juce::Colour accentCyan()    { return juce::Colour::fromRGB(74, 190, 255); }
    inline juce::Colour accentGreen()   { return juce::Colour::fromRGB(50, 255, 120); }
    inline juce::Colour accentAmber()   { return juce::Colour::fromRGB(255, 190, 60); }
    inline juce::Colour accentRed()     { return juce::Colour::fromRGB(255, 60, 60); }
    inline juce::Colour accentPurple()  { return juce::Colour::fromRGB(200, 100, 255); }
    inline juce::Colour accentTeal()    { return juce::Colour::fromRGB(0, 220, 190); }

    // Pad category colours
    inline juce::Colour catKick()   { return juce::Colour::fromRGB(255, 90, 40); }
    inline juce::Colour catSnare()  { return juce::Colour::fromRGB(255, 190, 50); }
    inline juce::Colour catHiHat()  { return juce::Colour::fromRGB(50, 220, 255); }
    inline juce::Colour catTom()    { return juce::Colour::fromRGB(100, 180, 255); }
    inline juce::Colour catClap()   { return juce::Colour::fromRGB(210, 90, 255); }
    inline juce::Colour catFX()     { return juce::Colour::fromRGB(50, 255, 160); }

    inline juce::Colour padCategoryColour(int padIndex)
    {
        switch (padIndex / 3)
        {
            case 0: return catKick();
            case 1: return catSnare();
            case 2: return catHiHat();
            case 3: return catTom();
            default: return catFX();
        }
    }

    // Text colours
    inline juce::Colour textMain()    { return juce::Colour::fromRGB(240, 240, 248); }
    inline juce::Colour textDim()     { return juce::Colour::fromRGB(155, 158, 175); }
    inline juce::Colour textMuted()   { return juce::Colour::fromRGB(90, 92, 110); }
    inline juce::Colour textDark()    { return juce::Colour::fromRGB(55, 57, 70); }

    // LED colours
    inline juce::Colour ledOn()       { return accentOrange(); }
    inline juce::Colour ledOff()      { return juce::Colour::fromRGB(38, 38, 50); }
    inline juce::Colour ledGlow()     { return accentOrange().withAlpha(0.4f); }

    // Border / outline
    inline juce::Colour borderSubtle() { return juce::Colour::fromRGBA(255, 255, 255, 16); }
    inline juce::Colour borderStrong() { return juce::Colour::fromRGBA(255, 255, 255, 38); }
    inline juce::Colour shadowSoft()   { return juce::Colour::fromRGBA(0, 0, 0, 100); }

    // =========================================================================
    // Typography
    // =========================================================================
    inline juce::Font fontHeader() { return juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold);  }
    inline juce::Font fontLabel()  { return juce::Font(juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::bold);  }
    inline juce::Font fontValue()  { return juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::plain); }
    inline juce::Font fontSmall()  { return juce::Font(juce::Font::getDefaultSansSerifFontName(),  9.0f, juce::Font::plain); }
    inline juce::Font fontMicro()  { return juce::Font(juce::Font::getDefaultSansSerifFontName(),  8.0f, juce::Font::plain); }

    // =========================================================================
    // Geometry Constants
    // =========================================================================
    inline constexpr float cornerRadius()      { return 6.0f; }
    inline constexpr float cornerRadiusSmall() { return 3.0f; }
    inline constexpr float padGap()            { return 6.0f; }
    inline constexpr float panelGap()          { return 8.0f; }
    inline constexpr float margin()            { return 8.0f; }

    // =========================================================================
    // Drawing Helpers
    // =========================================================================

    // Standard panel fill
    inline void fillPanel(juce::Graphics& g, juce::Rectangle<float> area,
                          juce::Colour base = panelBase(), float radius = cornerRadius())
    {
        juce::ColourGradient grad(base.withBrightness(1.04f), area.getX(), area.getY(),
                                  base.withBrightness(0.94f), area.getX(), area.getBottom(), false);
        grad.addColour(0.5f, base);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(area, radius);
        g.setColour(juce::Colours::white.withAlpha(0.03f));
        g.fillRoundedRectangle(area.withHeight(1.0f), radius);
        g.setColour(borderSubtle());
        g.drawRoundedRectangle(area.reduced(0.5f), radius, 1.0f);
    }

    // Section card: panel + accent top line + section label
    inline void drawSectionCard(juce::Graphics& g, juce::Rectangle<float> area,
                                const juce::String& title, juce::Colour accent,
                                float radius = cornerRadius())
    {
        // Panel body
        fillPanel(g, area, panelBase(), radius);

        // Accent top bar (gradient fade to transparent)
        auto topBar = area.withHeight(3.0f).reduced(radius * 0.5f, 0.0f);
        juce::ColourGradient accentGrad(accent.withAlpha(0.0f), topBar.getX(), topBar.getY(),
                                         accent,                  topBar.getCentreX(), topBar.getY(), false);
        accentGrad.addColour(1.0f, accent.withAlpha(0.0f));
        g.setGradientFill(accentGrad);
        g.fillRoundedRectangle(topBar, 1.5f);

        // Section title — small, upper-left
        g.setColour(accent.withAlpha(0.75f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 8.5f, juce::Font::bold));
        g.drawText(title, area.reduced(10.0f, 6.0f).removeFromTop(14.0f),
                   juce::Justification::topLeft);
    }

    // Inset / recessed area
    inline void fillInset(juce::Graphics& g, juce::Rectangle<float> area,
                          juce::Colour base = panelInset(), float radius = cornerRadiusSmall())
    {
        g.setColour(base);
        g.fillRoundedRectangle(area, radius);
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.drawRoundedRectangle(area, radius, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.02f));
        g.drawRoundedRectangle(area.reduced(1.0f), radius - 0.5f, 0.5f);
    }

    // Circular LED
    inline void drawLED(juce::Graphics& g, juce::Point<float> centre, float radius,
                        bool on, juce::Colour onCol = ledOn(), juce::Colour offCol = ledOff())
    {
        auto bounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
        if (on)
        {
            g.setColour(onCol.withAlpha(0.30f));
            g.fillEllipse(bounds.expanded(3.5f));
        }
        g.setColour(on ? onCol : offCol);
        g.fillEllipse(bounds);
        g.setColour(juce::Colours::white.withAlpha(on ? 0.45f : 0.08f));
        g.fillEllipse(bounds.reduced(bounds.getWidth() * 0.25f, bounds.getHeight() * 0.35f)
                           .translated(0.0f, -bounds.getHeight() * 0.1f));
    }

    // Rectangular LED strip
    inline void drawLED(juce::Graphics& g, juce::Rectangle<float> area,
                        bool on, juce::Colour onCol = ledOn(), juce::Colour offCol = ledOff())
    {
        if (on)
        {
            g.setColour(onCol.withAlpha(0.22f));
            g.fillRoundedRectangle(area.expanded(2.0f), 2.0f);
        }
        g.setColour(on ? onCol : offCol);
        g.fillRoundedRectangle(area, 2.0f);
        g.setColour(juce::Colours::white.withAlpha(on ? 0.32f : 0.05f));
        g.fillRoundedRectangle(area.withHeight(area.getHeight() * 0.4f), 2.0f);
    }

    // Segmented LED strip (meter)
    inline void drawLEDStrip(juce::Graphics& g, juce::Rectangle<float> area,
                             float level, int numSegments = 16, bool horizontal = true)
    {
        const float gap = 1.5f;
        const float segSize = horizontal
            ? (area.getWidth()  - gap * (numSegments - 1)) / numSegments
            : (area.getHeight() - gap * (numSegments - 1)) / numSegments;

        for (int i = 0; i < numSegments; ++i)
        {
            float t = (float)i / (float)(numSegments - 1);
            bool  active = level > t;

            juce::Rectangle<float> seg;
            if (horizontal)
                seg = { area.getX() + i * (segSize + gap), area.getY(), segSize, area.getHeight() };
            else
                seg = { area.getX(), area.getBottom() - (i + 1) * (segSize + gap) + gap,
                        area.getWidth(), segSize };

            juce::Colour col = t < 0.60f ? accentGreen()
                             : t < 0.85f ? accentAmber()
                                         : accentRed();

            if (active)
            {
                g.setColour(col.withAlpha(0.88f));
                g.fillRoundedRectangle(seg, 1.0f);
                g.setColour(juce::Colours::white.withAlpha(0.22f));
                g.fillRoundedRectangle(seg.withHeight(seg.getHeight() * 0.38f), 1.0f);
            }
            else
            {
                g.setColour(col.withAlpha(0.10f));
                g.fillRoundedRectangle(seg, 1.0f);
            }
        }
    }

    // Section label (floating, above a panel)
    inline void drawSectionHeader(juce::Graphics& g, juce::Rectangle<float> area,
                                  const juce::String& title, juce::Colour accent = accentOrange())
    {
        g.setColour(panelBase().darker(0.1f));
        g.fillRect(area);
        drawLED(g, { area.getX() + 10.0f, area.getCentreY() }, 3.0f, true, accent);
        g.setColour(textDim());
        g.setFont(fontLabel());
        g.drawText(title, area.reduced(22.0f, 0.0f), juce::Justification::centredLeft, false);
        g.setColour(accent.withAlpha(0.4f));
        g.drawHorizontalLine((int)area.getBottom() - 1, area.getX() + 4.0f, area.getRight() - 4.0f);
    }

    // Subtle dot grid
    inline void drawGrid(juce::Graphics& g, juce::Rectangle<float> area, float spacing = 20.0f)
    {
        g.setColour(juce::Colours::white.withAlpha(0.013f));
        for (float x = area.getX(); x < area.getRight(); x += spacing)
            g.drawVerticalLine(juce::roundToInt(x), area.getY(), area.getBottom());
        for (float y = area.getY(); y < area.getBottom(); y += spacing)
            g.drawHorizontalLine(juce::roundToInt(y), area.getX(), area.getRight());
    }

    // =========================================================================
    // Animation Helpers
    // =========================================================================
    inline float smoothDecay(float current, float target, float factor = 0.85f)
    {
        return current * factor + target * (1.0f - factor);
    }

    inline float flashDecay(float current, float decayRate = 0.82f)
    {
        float next = current * decayRate;
        return next < 0.01f ? 0.0f : next;
    }

} // namespace UITheme
