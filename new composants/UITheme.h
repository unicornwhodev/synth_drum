#pragma once
#include <JuceHeader.h>

// =============================================================================
// UITheme — Unified Hardware Dark Theme for UWdeVST Drum Synth
// =============================================================================
// Single source of truth for all visual design tokens.
// Inspired by modern hardware: Moog Subsequent, Roland TR-8S, TAL-Drum.
// Flat with subtle depth, LED accents, clean typography.
// =============================================================================

namespace UITheme
{
    // =========================================================================
    // Colour Palette
    // =========================================================================

    // Background hierarchy
    inline juce::Colour bgDeep()      { return juce::Colour::fromRGB(14, 14, 18); }   // Deepest layer
    inline juce::Colour bgBase()      { return juce::Colour::fromRGB(20, 20, 26); }   // Main background
    inline juce::Colour bgElevated()  { return juce::Colour::fromRGB(28, 28, 36); }   // Slightly raised

    // Panel surfaces
    inline juce::Colour panelBase()   { return juce::Colour::fromRGB(35, 36, 44); }   // Standard panel
    inline juce::Colour panelHover()  { return juce::Colour::fromRGB(42, 43, 52); }   // Hover state
    inline juce::Colour panelActive() { return juce::Colour::fromRGB(48, 49, 60); }   // Active/selected
    inline juce::Colour panelInset()  { return juce::Colour::fromRGB(18, 18, 24); }   // Recessed areas

    // Accent colours — section-based
    inline juce::Colour accentOrange()  { return juce::Colour::fromRGB(255, 107, 53); }   // Voice / Synth
    inline juce::Colour accentCyan()    { return juce::Colour::fromRGB(74, 158, 255); }   // Filter / FX
    inline juce::Colour accentGreen()   { return juce::Colour::fromRGB(61, 255, 127); }   // Macros / Perform
    inline juce::Colour accentAmber()   { return juce::Colour::fromRGB(255, 184, 77); }   // Warning / FX alt
    inline juce::Colour accentRed()     { return juce::Colour::fromRGB(255, 61, 61); }    // Clip / Danger
    inline juce::Colour accentPurple()  { return juce::Colour::fromRGB(200, 100, 255); }  // Special

    // Pad category colours
    inline juce::Colour catKick()   { return accentOrange(); }
    inline juce::Colour catSnare()  { return accentAmber(); }
    inline juce::Colour catHiHat()  { return accentGreen(); }
    inline juce::Colour catTom()    { return accentCyan(); }
    inline juce::Colour catClap()   { return accentPurple(); }
    inline juce::Colour catPerc()   { return juce::Colour::fromRGB(255, 100, 100); }
    inline juce::Colour catFX()     { return juce::Colour::fromRGB(61, 220, 180); }
    inline juce::Colour catCym()    { return juce::Colour::fromRGB(255, 230, 77); }

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
    inline juce::Colour textMain()    { return juce::Colour::fromRGB(245, 245, 245); }   // Primary text
    inline juce::Colour textDim()     { return juce::Colour::fromRGB(160, 160, 170); }   // Secondary
    inline juce::Colour textMuted()   { return juce::Colour::fromRGB(100, 100, 115); }   // Tertiary / disabled
    inline juce::Colour textDark()    { return juce::Colour::fromRGB(60, 60, 70); }      // Very subtle

    // LED colours
    inline juce::Colour ledOn()       { return accentOrange(); }
    inline juce::Colour ledOff()      { return juce::Colour::fromRGB(45, 45, 55); }
    inline juce::Colour ledGlow()     { return accentOrange().withAlpha(0.4f); }

    // Border / outline
    inline juce::Colour borderSubtle() { return juce::Colour::fromRGBA(255, 255, 255, 18); }
    inline juce::Colour borderStrong() { return juce::Colour::fromRGBA(255, 255, 255, 40); }
    inline juce::Colour shadowSoft()   { return juce::Colour::fromRGBA(0, 0, 0, 80); }

    // =========================================================================
    // Typography — System fonts, clear hierarchy
    // =========================================================================

    inline juce::Font fontHeader()  { return juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::bold)); }
    inline juce::Font fontLabel()   { return juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 11.0f, juce::Font::bold)); }
    inline juce::Font fontValue()   { return juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::plain)); }
    inline juce::Font fontSmall()   { return juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 9.0f, juce::Font::plain)); }
    inline juce::Font fontMicro()   { return juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 8.0f, juce::Font::plain)); }

    // =========================================================================
    // Geometry Constants
    // =========================================================================

    inline constexpr float cornerRadius()    { return 4.0f; }
    inline constexpr float cornerRadiusSmall() { return 2.0f; }
    inline constexpr float padGap()          { return 6.0f; }
    inline constexpr float panelGap()        { return 8.0f; }
    inline constexpr float margin()          { return 12.0f; }

    // =========================================================================
    // Drawing Helpers
    // =========================================================================

    // Fill a panel surface with subtle depth
    inline void fillPanel(juce::Graphics& g, juce::Rectangle<float> area,
                          juce::Colour base = panelBase(), float radius = cornerRadius())
    {
        // Subtle vertical gradient for depth
        juce::ColourGradient grad(base.withBrightness(1.04f), area.getX(), area.getY(),
                                  base.withBrightness(0.96f), area.getX(), area.getBottom(), false);
        grad.addColour(0.5f, base);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(area, radius);

        // Top highlight line
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.fillRoundedRectangle(area.withHeight(1.0f), radius);

        // Border
        g.setColour(borderSubtle());
        g.drawRoundedRectangle(area.reduced(0.5f), radius, 1.0f);
    }

    // Fill an inset / recessed area
    inline void fillInset(juce::Graphics& g, juce::Rectangle<float> area,
                          juce::Colour base = panelInset(), float radius = cornerRadiusSmall())
    {
        g.setColour(base);
        g.fillRoundedRectangle(area, radius);

        // Inner shadow effect
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawRoundedRectangle(area, radius, 1.0f);

        g.setColour(juce::Colours::white.withAlpha(0.02f));
        g.drawRoundedRectangle(area.reduced(1.0f), radius - 0.5f, 0.5f);
    }

    // Draw an LED indicator (circular or rectangular)
    inline void drawLED(juce::Graphics& g, juce::Point<float> centre, float radius,
                        bool on, juce::Colour onCol = ledOn(), juce::Colour offCol = ledOff())
    {
        auto col = on ? onCol : offCol;
        auto bounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);

        // Glow when on
        if (on)
        {
            g.setColour(onCol.withAlpha(0.25f));
            g.fillEllipse(bounds.expanded(3.0f));
        }

        // LED body
        g.setColour(col);
        g.fillEllipse(bounds);

        // Highlight
        g.setColour(juce::Colours::white.withAlpha(on ? 0.4f : 0.08f));
        g.fillEllipse(bounds.reduced(bounds.getWidth() * 0.25f, bounds.getHeight() * 0.35f)
                          .translated(0.0f, -bounds.getHeight() * 0.1f));
    }

    inline void drawLED(juce::Graphics& g, juce::Rectangle<float> area,
                        bool on, juce::Colour onCol = ledOn(), juce::Colour offCol = ledOff())
    {
        auto col = on ? onCol : offCol;

        if (on)
        {
            g.setColour(onCol.withAlpha(0.2f));
            g.fillRoundedRectangle(area.expanded(2.0f), 2.0f);
        }

        g.setColour(col);
        g.fillRoundedRectangle(area, 2.0f);

        g.setColour(juce::Colours::white.withAlpha(on ? 0.3f : 0.06f));
        g.fillRoundedRectangle(area.withHeight(area.getHeight() * 0.4f), 2.0f);
    }

    // Draw a horizontal LED strip / bar
    inline void drawLEDStrip(juce::Graphics& g, juce::Rectangle<float> area,
                             float level, // 0..1
                             int numSegments = 16,
                             bool horizontal = true)
    {
        const float gap = 1.5f;
        const float segSize = horizontal
            ? (area.getWidth() - gap * (numSegments - 1)) / numSegments
            : (area.getHeight() - gap * (numSegments - 1)) / numSegments;

        for (int i = 0; i < numSegments; ++i)
        {
            float t = (float)i / (float)(numSegments - 1);
            bool active = level > t;

            juce::Rectangle<float> seg;
            if (horizontal)
            {
                seg = juce::Rectangle<float>(area.getX() + i * (segSize + gap),
                                             area.getY(), segSize, area.getHeight());
            }
            else
            {
                seg = juce::Rectangle<float>(area.getX(),
                                             area.getBottom() - (i + 1) * (segSize + gap) + gap,
                                             area.getWidth(), segSize);
            }

            // Colour: green < 0.6, amber < 0.85, red >= 0.85
            juce::Colour segCol;
            if (t < 0.6f)       segCol = accentGreen();
            else if (t < 0.85f) segCol = accentAmber();
            else                segCol = accentRed();

            if (active)
            {
                g.setColour(segCol.withAlpha(0.85f));
                g.fillRoundedRectangle(seg, 1.0f);
                g.setColour(juce::Colours::white.withAlpha(0.2f));
                g.fillRoundedRectangle(seg.withHeight(seg.getHeight() * 0.4f), 1.0f);
            }
            else
            {
                g.setColour(segCol.withAlpha(0.12f));
                g.fillRoundedRectangle(seg, 1.0f);
            }
        }
    }

    // Draw a section header with accent line
    inline void drawSectionHeader(juce::Graphics& g, juce::Rectangle<float> area,
                                  const juce::String& title, juce::Colour accent = accentOrange())
    {
        // Background strip
        g.setColour(panelBase().withBrightness(0.9f));
        g.fillRect(area);

        // LED dot
        auto ledCentre = juce::Point<float>(area.getX() + 10.0f, area.getCentreY());
        drawLED(g, ledCentre, 3.0f, true, accent);

        // Title
        g.setColour(textDim());
        g.setFont(fontLabel());
        g.drawText(title, area.reduced(20.0f, 0.0f), juce::Justification::centredLeft, false);

        // Accent line at bottom
        g.setColour(accent.withAlpha(0.4f));
        g.drawHorizontalLine(area.getBottom() - 1.0f, area.getX() + 4.0f, area.getRight() - 4.0f);
    }

    // Draw a subtle grid background
    inline void drawGrid(juce::Graphics& g, juce::Rectangle<float> area, float spacing = 20.0f)
    {
        g.setColour(juce::Colours::white.withAlpha(0.015f));
        for (float x = area.getX(); x < area.getRight(); x += spacing)
            g.drawVerticalLine(juce::roundToInt(x), area.getY(), area.getBottom());
        for (float y = area.getY(); y < area.getBottom(); y += spacing)
            g.drawHorizontalLine(juce::roundToInt(y), area.getX(), area.getRight());
    }

    // =========================================================================
    // Animation Helpers
    // =========================================================================

    // Smooth decay for meter / activity values
    inline float smoothDecay(float current, float target, float factor = 0.85f)
    {
        return current * factor + target * (1.0f - factor);
    }

    // Flash decay (for pad triggers)
    inline float flashDecay(float current, float decayRate = 0.82f)
    {
        float next = current * decayRate;
        return next < 0.01f ? 0.0f : next;
    }

} // namespace UITheme
