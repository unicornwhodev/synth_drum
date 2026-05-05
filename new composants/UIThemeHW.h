#pragma once
#include <JuceHeader.h>

// =============================================================================
// UIThemeHW — Hardware synth aesthetic design tokens and drawing helpers
// Inspired by classic hardware: Moog, Oberheim, Roland, TAL, NI
// =============================================================================
namespace UIThemeHW
{
    // ---- Colour palette — hardware-inspired ----

    inline juce::Colour rackBlack()        { return juce::Colour::fromRGB(26, 26, 30); }
    inline juce::Colour brushedAlum()      { return juce::Colour::fromRGB(58, 61, 66); }
    inline juce::Colour panelGrey()        { return juce::Colour::fromRGB(45, 47, 51); }
    inline juce::Colour panelDark()        { return juce::Colour::fromRGB(35, 36, 40); }
    inline juce::Colour panelInset()       { return juce::Colour::fromRGB(20, 21, 24); }
    inline juce::Colour creamPanel()       { return juce::Colour::fromRGB(232, 224, 212); }
    inline juce::Colour creamMid()         { return juce::Colour::fromRGB(200, 191, 180); }
    inline juce::Colour creamDark()        { return juce::Colour::fromRGB(160, 150, 135); }
    inline juce::Colour knobMetal()        { return juce::Colour::fromRGB(74, 77, 82); }
    inline juce::Colour knobDark()         { return juce::Colour::fromRGB(45, 48, 53); }
    inline juce::Colour knobPointer()      { return juce::Colour::fromRGB(240, 240, 240); }
    inline juce::Colour knobCap()          { return juce::Colour::fromRGB(35, 38, 43); }

    inline juce::Colour ledGreen()         { return juce::Colour::fromRGB(61, 255, 127); }
    inline juce::Colour ledAmber()         { return juce::Colour::fromRGB(255, 184, 77); }
    inline juce::Colour ledRed()           { return juce::Colour::fromRGB(255, 61, 61); }
    inline juce::Colour ledOff()           { return juce::Colour::fromRGB(55, 57, 62); }

    inline juce::Colour textWhite()        { return juce::Colour::fromRGB(245, 245, 245); }
    inline juce::Colour textDim()          { return juce::Colour::fromRGB(136, 136, 136); }
    inline juce::Colour textMuted()        { return juce::Colour::fromRGB(85, 85, 85); }
    inline juce::Colour textMain()         { return textWhite(); }

    inline juce::Colour accentOrange()     { return juce::Colour::fromRGB(255, 107, 53); }   // Moog-style
    inline juce::Colour accentBlue()       { return juce::Colour::fromRGB(74, 158, 255); }  // LED blue

    // Section accent colours (pad categories)
    inline juce::Colour catKick()          { return juce::Colour::fromRGB(255, 107, 53); }
    inline juce::Colour catSnare()         { return juce::Colour::fromRGB(255, 184, 77); }
    inline juce::Colour catHiHat()         { return juce::Colour::fromRGB(61, 255, 127); }
    inline juce::Colour catTom()           { return juce::Colour::fromRGB(74, 158, 255); }
    inline juce::Colour catClap()          { return juce::Colour::fromRGB(200, 100, 255); }
    inline juce::Colour catPerc()          { return juce::Colour::fromRGB(255, 61, 61); }
    inline juce::Colour catFX()            { return juce::Colour::fromRGB(61, 220, 180); }
    inline juce::Colour catCym()           { return juce::Colour::fromRGB(255, 230, 77); }

    // ---- Typography ----

    inline juce::Font headerFont()   { return juce::Font("Arial", 13.0f, juce::Font::bold); }
    inline juce::Font labelFont()    { return juce::Font("Arial", 11.0f, juce::Font::bold); }
    inline juce::Font valueFont()    { return juce::Font("Arial", 10.0f, juce::Font::plain); }
    inline juce::Font smallFont()    { return juce::Font("Arial", 9.0f, juce::Font::plain); }
    inline juce::Font sectionFont()  { return juce::Font("Arial", 12.0f, juce::Font::bold); }

    // ---- Geometry constants ----

    inline constexpr float panelRadius()   { return 4.0f; }
    inline constexpr float insetRadius()   { return 2.0f; }
    inline constexpr float screwRadius()    { return 3.5f; }
    inline constexpr float ledRadius()     { return 2.8f; }

    // =============================================================================
    // Drawing helpers
    // =============================================================================

    // Fill a panel with brushed aluminum look, screw heads in corners
    inline void fillPanel(juce::Graphics& g, const juce::Rectangle<float>& area,
                         juce::Colour panelCol = panelGrey(),
                         juce::Colour accent = accentOrange(),
                         float cornerRadius = panelRadius())
    {
        // Main panel fill
        juce::ColourGradient grad(panelCol.withBrightness(0.22f), area.getX(), area.getY(),
                                  panelCol.withBrightness(0.17f), area.getX(), area.getBottom(), false);
        grad.addColour(0.4f, panelCol);
        grad.addColour(1.0f, panelCol.withBrightness(0.14f));
        g.setGradientFill(grad);
        g.fillRoundedRectangle(area, cornerRadius);

        // Top sheen line
        g.setColour(juce::Colours::white.withAlpha(0.07f));
        g.fillRoundedRectangle(area.withHeight(1.5f), cornerRadius);

        // Border
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRoundedRectangle(area, cornerRadius, 1.0f);

        // Screw heads in corners
        auto screwPositions = juce::Array<juce::Point<float>>{
            area.getTopLeft().translated(cornerRadius * 1.5f, cornerRadius * 1.5f),
            area.getTopRight().translated(-cornerRadius * 1.5f, cornerRadius * 1.5f),
            area.getBottomLeft().translated(cornerRadius * 1.5f, -cornerRadius * 1.5f),
            area.getBottomRight().translated(-cornerRadius * 1.5f, -cornerRadius * 1.5f)
        };
        for (auto pos : screwPositions)
        {
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillEllipse(juce::Rectangle<float>(screwRadius() * 2.0f, screwRadius() * 2.0f).withCentre(pos));
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            g.fillEllipse(juce::Rectangle<float>(screwRadius() * 1.5f, screwRadius() * 1.5f).withCentre(pos));
        }
    }

    inline void fillPanel(juce::Graphics& g, const juce::Rectangle<float>& area,
                          float cornerRadius)
    {
        fillPanel(g, area, panelGrey(), accentOrange(), cornerRadius);
    }

    // Inset/recess area (like slot for knob, meter track)
    inline void fillInset(juce::Graphics& g, const juce::Rectangle<float>& area,
                          float cornerRadius = insetRadius())
    {
        g.setColour(panelInset());
        g.fillRoundedRectangle(area, cornerRadius);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(area, cornerRadius, 1.0f);
    }

    inline void fillRecess(juce::Graphics& g, const juce::Rectangle<float>& area,
                           float cornerRadius = insetRadius())
    {
        fillInset(g, area, cornerRadius);
    }

    // Section label strip — cream text on dark band, like hardware label tape
    inline void drawSectionLabel(juce::Graphics& g, const juce::Rectangle<float>& area,
                                 const juce::String& text, juce::Colour textCol = creamPanel(),
                                 juce::Colour bgCol = panelDark())
    {
        g.setColour(bgCol);
        g.fillRect(area);

        g.setColour(textCol);
        g.setFont(labelFont());
        g.drawText(text, area.reduced(8, 0), juce::Justification::centredLeft);
    }

    // LED indicator dot
    inline void drawLED(juce::Graphics& g, juce::Point<float> center,
                        float radius = ledRadius(),
                        bool active = true,
                        juce::Colour activeCol = ledGreen(),
                        juce::Colour inactiveCol = ledOff())
    {
        auto col = active ? activeCol : inactiveCol;

        // Glow
        if (active)
        {
            g.setColour(col.withAlpha(0.3f));
            g.fillEllipse(juce::Rectangle<float>(radius * 3.0f, radius * 3.0f).withCentre(center));
        }

        // LED body
        g.setColour(col);
        g.fillEllipse(juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(center));

        // Highlight
        g.setColour(juce::Colours::white.withAlpha(active ? 0.5f : 0.1f));
        g.fillEllipse(juce::Rectangle<float>(radius, radius).withCentre(
            center.translated(-radius * 0.3f, -radius * 0.3f)));
    }

    inline void drawLED(juce::Graphics& g, const juce::Rectangle<float>& area,
                        float fillRatio,
                        float alpha,
                        juce::Colour activeCol = ledGreen(),
                        juce::Colour inactiveCol = ledOff())
    {
        auto normalized = area;
        if (normalized.getWidth() <= 0.0f || normalized.getHeight() <= 0.0f)
            normalized = juce::Rectangle<float>(area.getX(), area.getY(), 2.0f, juce::jmax(area.getHeight(), 8.0f));

        if (normalized.getWidth() <= normalized.getHeight() * 1.5f)
        {
            const auto radius = juce::jmax(1.5f, normalized.getHeight() * 0.3f);
            drawLED(g, normalized.getCentre(), radius, fillRatio > 0.0f,
                    activeCol.withAlpha(alpha), inactiveCol.withAlpha(juce::jmax(0.12f, alpha * 0.35f)));
            return;
        }

        const auto activeWidth = normalized.getWidth() * juce::jlimit(0.0f, 1.0f, fillRatio);
        auto activeArea = normalized.withWidth(activeWidth);

        g.setColour(inactiveCol.withAlpha(juce::jmax(0.08f, alpha * 0.18f)));
        g.fillRoundedRectangle(normalized, normalized.getHeight() * 0.5f);

        if (activeWidth > 0.0f)
        {
            g.setColour(activeCol.withAlpha(alpha * 0.3f));
            g.fillRoundedRectangle(normalized.expanded(1.5f, 1.5f), normalized.getHeight() * 0.5f);

            g.setColour(activeCol.withAlpha(alpha));
            g.fillRoundedRectangle(activeArea, normalized.getHeight() * 0.5f);

            g.setColour(juce::Colours::white.withAlpha(alpha * 0.25f));
            g.fillRoundedRectangle(activeArea.withHeight(normalized.getHeight() * 0.4f), normalized.getHeight() * 0.5f);
        }
    }

    // Draw an LED strip (horizontal bar of LEDs)
    inline void drawLEDStrip(juce::Graphics& g, const juce::Rectangle<float>& track,
                             int numLEDs, float fillRatio,
                             juce::Colour greenCol = ledGreen(),
                             juce::Colour amberCol = ledAmber(),
                             juce::Colour redCol = ledRed())
    {
        const float ledH = track.getHeight();
        const float gap = 2.0f;
        const float ledW = (track.getWidth() - gap * (numLEDs - 1)) / (float)numLEDs;

        for (int i = 0; i < numLEDs; ++i)
        {
            auto ledRect = juce::Rectangle<float>(
                track.getX() + i * (ledW + gap),
                track.getY(),
                ledW,
                ledH
            ).reduced(0.5f);

            float threshold = (float)i / (float)numLEDs;
            juce::Colour col;
            if (threshold < 0.6f) col = greenCol;
            else if (threshold < 0.85f) col = amberCol;
            else col = redCol;

            bool active = fillRatio > threshold;
            if (!active)
                col = col.withAlpha(0.15f);

            g.setColour(col);
            g.fillRoundedRectangle(ledRect, 1.0f);

            if (active)
            {
                g.setColour(juce::Colours::white.withAlpha(0.3f));
                g.fillRoundedRectangle(ledRect.withHeight(ledH * 0.4f).withY(ledRect.getY()), 1.0f);
            }
        }
    }

    // Value display pill (monospace readout under knob)
    inline void drawValuePill(juce::Graphics& g, const juce::Rectangle<float>& area,
                              const juce::String& value, juce::Colour textCol = textWhite(),
                              float alpha = 1.0f)
    {
        auto pill = area.expanded(4.0f, 2.0f);
        g.setColour(juce::Colour::fromRGB(15, 15, 18).withAlpha(alpha * 0.9f));
        g.fillRoundedRectangle(pill, 2.0f);
        g.setColour(textCol.withAlpha(alpha));
        g.setFont(valueFont());
        g.drawText(value, area, juce::Justification::centred);
    }

    // Pointer/needle line for a knob
    inline void drawKnobPointer(juce::Graphics& g, juce::Point<float> center,
                                float radius, float angle,
                                juce::Colour col = knobPointer())
    {
        auto p1 = center.getPointOnCircumference(radius * 0.18f, angle);
        auto p2 = center.getPointOnCircumference(radius * 0.72f, angle);
        g.setColour(col);
        g.drawLine(p1.x, p1.y, p2.x, p2.y, 2.5f);
    }

    // LED ring around knob (shows value as lit segment)
    inline void drawKnobLEDRing(juce::Graphics& g, juce::Point<float> center,
                                float radius, float startAngle, float endAngle,
                                float fillRatio, int numLEDs,
                                juce::Colour activeCol = ledGreen(),
                                juce::Colour inactiveCol = ledOff())
    {
        for (int i = 0; i < numLEDs; ++i)
        {
            float t = (float)i / (float)(numLEDs - 1);
            float a = juce::jmap(t, startAngle, endAngle);
            auto p = center.getPointOnCircumference(radius * 1.02f, a);
            bool active = t <= fillRatio;
            drawLED(g, p, ledRadius() * 0.8f, active, activeCol, inactiveCol);
        }
    }

    // Section header with LED indicator
    inline void drawSectionHeader(juce::Graphics& g, const juce::Rectangle<float>& headerRect,
                                  const juce::String& title, bool ledActive = true,
                                  juce::Colour ledCol = ledGreen())
    {
        // Dark label strip
        g.setColour(panelDark());
        g.fillRect(headerRect);

        // LED dot on left
        auto ledCenter = headerRect.getCentre().translated(-headerRect.getWidth() * 0.45f, 0.0f);
        drawLED(g, ledCenter, ledRadius() * 0.7f, ledActive, ledCol);

        // Title text
        g.setColour(creamPanel());
        g.setFont(labelFont());
        g.drawText(title, headerRect.reduced(6, 0), juce::Justification::centredLeft);

        // Bottom accent line
        g.setColour(ledCol.withAlpha(0.4f));
        g.fillRect(headerRect.withBottom(headerRect.getBottom() - 1));
    }

    // Envelope curve draw helper
    inline void drawADSREnvelope(juce::Graphics& g, const juce::Rectangle<float> area,
                                  float a, float d, float s, float r,
                                  juce::Colour curveCol, juce::Colour fillCol)
    {
        const float padL = 6.0f;
        const float padT = 10.0f;
        const float padB = 10.0f;
        const float x = area.getX() + padL;
        const float y = area.getY() + padT;
        const float w = area.getWidth() - padL * 2;
        const float h = area.getHeight() - padT - padB;

        // Normalize times (a,d,s,r are 0..1 relative values, s is level)
        const float total = a + d + 0.4f + r;
        const float ax = x + (a / total) * w * 0.85f;
        const float dx = x + ((a + d) / total) * w * 0.85f;
        const float sx = x + w * 0.85f;
        const float rx = area.getRight() - padL;

        const float yTop = y;
        const float ySus = y + (1.0f - s) * h;
        const float yBot = area.getBottom() - padB;

        // Points
        auto p0 = juce::Point<float>(x, yBot);
        auto p1 = juce::Point<float>(ax, yTop);
        auto p2 = juce::Point<float>(dx, ySus);
        auto p3 = juce::Point<float>(sx, ySus);
        auto p4 = juce::Point<float>(rx, yBot);

        // Fill area
        juce::Path fillPath;
        fillPath.startNewSubPath(p0);
        fillPath.lineTo(p1);
        fillPath.cubicTo(p1.x + w * 0.08f, p1.y - h * 0.15f,
                 dx - w * 0.06f, ySus + h * 0.05f, p2.x, p2.y);
        fillPath.lineTo(p3);
        fillPath.cubicTo(sx + w * 0.05f, ySus - h * 0.08f,
                         rx - w * 0.04f, yBot - h * 0.1f, p4.x, p4.y);
        fillPath.lineTo(p0);
        fillPath.closeSubPath();

        g.setColour(fillCol.withAlpha(0.25f));
        g.fillPath(fillPath);

        // Curve stroke
        juce::Path curvePath;
        curvePath.startNewSubPath(p0);
        curvePath.lineTo(p1);
        curvePath.cubicTo(p1.x + w * 0.08f, p1.y - h * 0.15f,
                  dx - w * 0.06f, ySus + h * 0.05f, p2.x, p2.y);
        curvePath.lineTo(p3);
        curvePath.cubicTo(sx + w * 0.05f, ySus - h * 0.08f,
                          rx - w * 0.04f, yBot - h * 0.1f, p4.x, p4.y);

        g.setColour(curveCol.withAlpha(0.6f));
        g.strokePath(curvePath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Glow stroke
        g.setColour(curveCol.withAlpha(0.2f));
        g.strokePath(curvePath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}