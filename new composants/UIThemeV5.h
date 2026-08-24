#pragma once
#include <JuceHeader.h>

namespace UIThemeV5
{
    static constexpr float panelRadius = 20.0f;
    static constexpr float recessRadius = 13.0f;

    inline juce::Colour bgTop()        { return juce::Colour::fromRGB (12, 16, 22); }
    inline juce::Colour bgMid()        { return juce::Colour::fromRGB (8, 12, 18); }
    inline juce::Colour bgBottom()     { return juce::Colour::fromRGB (5, 8, 13); }
    inline juce::Colour panelTop()     { return juce::Colour::fromRGB (30, 36, 46); }
    inline juce::Colour panelMid()     { return juce::Colour::fromRGB (20, 25, 33); }
    inline juce::Colour panelBottom()  { return juce::Colour::fromRGB (13, 17, 24); }
    inline juce::Colour outlineHi()    { return juce::Colour::fromRGBA (255, 245, 224, 22); }
    inline juce::Colour outlineLo()    { return juce::Colour::fromRGBA (0, 0, 0, 135); }
    inline juce::Colour panelShadow()  { return juce::Colour::fromRGBA (0, 0, 0, 90); }

    inline juce::Colour recessTop()    { return juce::Colour::fromRGB (14, 19, 27); }
    inline juce::Colour recessMid()    { return juce::Colour::fromRGB (9, 13, 20); }
    inline juce::Colour recessBottom() { return juce::Colour::fromRGB (6, 8, 14); }

    inline juce::Colour textMain()     { return juce::Colour::fromRGB (245, 239, 227); }
    inline juce::Colour textDim()      { return juce::Colour::fromRGB (137, 145, 156); }
    inline juce::Colour textMuted()    { return juce::Colour::fromRGB (98, 107, 119); }

    inline juce::Colour accent()       { return juce::Colour::fromRGB (255, 187, 92); }
    inline juce::Colour accentStrong() { return juce::Colour::fromRGB (255, 149, 48); }
    inline juce::Colour accentGlow()   { return juce::Colour::fromRGBA (255, 187, 92, 120); }
    inline juce::Colour accentAlt()    { return juce::Colour::fromRGB (104, 215, 232); }
    inline juce::Colour success()      { return juce::Colour::fromRGB (114, 220, 156); }

    inline juce::Colour metalHi()      { return juce::Colour::fromRGB (238, 223, 198); }
    inline juce::Colour metalUpper()   { return juce::Colour::fromRGB (202, 175, 138); }
    inline juce::Colour metalMid()     { return juce::Colour::fromRGB (127, 109, 88); }
    inline juce::Colour metalLo()      { return juce::Colour::fromRGB (57, 47, 39); }

    inline juce::Font heroFont()       { return juce::FontOptions ("Trebuchet MS", 30.0f, juce::Font::bold); }
    inline juce::Font titleFont()      { return juce::FontOptions ("Trebuchet MS", 16.0f, juce::Font::bold); }
    inline juce::Font labelFont()      { return juce::FontOptions ("Trebuchet MS", 13.0f, juce::Font::bold); }
    inline juce::Font smallFont()      { return juce::FontOptions ("Trebuchet MS", 11.0f, juce::Font::plain); }
    inline juce::Font microFont()      { return juce::FontOptions ("Trebuchet MS", 9.5f, juce::Font::plain); }

    inline void drawBackdropGrid (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setColour (juce::Colours::white.withAlpha (0.018f));
        for (float x = area.getX() - 24.0f; x < area.getRight() + 24.0f; x += 28.0f)
            g.drawVerticalLine (juce::roundToInt (x), area.getY(), area.getBottom());
        for (float y = area.getY() - 24.0f; y < area.getBottom() + 24.0f; y += 28.0f)
            g.drawHorizontalLine (juce::roundToInt (y), area.getX(), area.getRight());
    }

    inline void fillPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = panelRadius)
    {
        g.setColour (panelShadow());
        g.fillRoundedRectangle (r.translated (0.0f, 7.0f), radius + 1.0f);

        juce::ColourGradient grad (panelTop(), r.getCentreX(), r.getY(),
                                   panelBottom(), r.getCentreX(), r.getBottom(), false);
        grad.addColour (0.36, panelMid());
        grad.addColour (0.72, panelBottom().brighter (0.04f));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, radius);

        auto sheen = r.reduced (1.0f).withHeight (r.getHeight() * 0.22f);
        juce::ColourGradient topGlow (juce::Colours::white.withAlpha (0.05f), sheen.getX(), sheen.getY(),
                                      juce::Colours::transparentWhite, sheen.getX(), sheen.getBottom(), false);
        g.setGradientFill (topGlow);
        g.fillRoundedRectangle (sheen, radius - 1.0f);

        g.setColour (outlineLo());
        g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.1f);
        g.setColour (outlineHi());
        g.drawRoundedRectangle (r.reduced (1.3f), radius - 0.8f, 0.8f);
    }

    inline void fillRecess (juce::Graphics& g, juce::Rectangle<float> r, float radius = recessRadius)
    {
        juce::ColourGradient grad (recessTop(), r.getCentreX(), r.getY(),
                                   recessBottom(), r.getCentreX(), r.getBottom(), false);
        grad.addColour (0.40, recessMid());
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, radius);

        auto innerGlow = r.reduced (1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.58f));
        g.drawRoundedRectangle (r, radius, 1.25f);
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        g.drawRoundedRectangle (innerGlow, radius - 0.9f, 0.8f);
    }

    inline void drawGlowStrip (juce::Graphics& g, juce::Rectangle<float> r, float radius = 2.5f, float alpha = 0.65f)
    {
        juce::ColourGradient glow (accentGlow().withAlpha (alpha), r.getCentreX(), r.getY(),
                                   accentStrong().withAlpha (alpha * 0.35f), r.getCentreX(), r.getBottom(), false);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (r, radius);
    }

    inline void drawPanelHeader (juce::Graphics& g,
                                 juce::Rectangle<float> area,
                                 const juce::String& title,
                                 const juce::String& caption,
                                 juce::Colour accentColour)
    {
        g.setColour (accentColour.withAlpha (0.95f));
        g.setFont (titleFont());
        g.drawText (title, area.removeFromTop (20.0f), juce::Justification::centredLeft, false);

        if (caption.isNotEmpty())
        {
            g.setColour (textDim());
            g.setFont (smallFont());
            g.drawText (caption, area.removeFromTop (14.0f), juce::Justification::centredLeft, false);
        }
    }

    inline void drawValuePill (juce::Graphics& g,
                               juce::Rectangle<float> area,
                               const juce::String& text,
                               juce::Colour colour,
                               float alpha = 0.18f)
    {
        g.setColour (colour.withAlpha (alpha));
        g.fillRoundedRectangle (area, area.getHeight() * 0.5f);
        g.setColour (colour.withAlpha (0.72f));
        g.drawRoundedRectangle (area, area.getHeight() * 0.5f, 0.9f);
        g.setColour (textMain());
        g.setFont (microFont());
        g.drawText (text, area, juce::Justification::centred, false);
    }
}
