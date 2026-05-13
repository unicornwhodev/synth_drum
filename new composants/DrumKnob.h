#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumKnob — Flat-body rotary with thick LED arc ring and always-on value
// =============================================================================
class DrumKnob : public juce::Component
{
public:
    DrumKnob(juce::String labelText,
             double min = 0.0, double max = 1.0, double value = 0.5,
             juce::Colour accentCol = UITheme::accentOrange());

    juce::Slider& getSlider() noexcept { return slider; }
    void setLabelText(const juce::String& t);
    void setAccentColour(juce::Colour c) { accentColour = c; lnf.accentCol = c; repaint(); }
    void resized() override;

private:
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        explicit LookAndFeel(juce::Colour ac) : accentCol(ac) {}

        void drawRotarySlider(juce::Graphics&,
                              int x, int y, int width, int height,
                              float sliderPosProportional,
                              float rotaryStartAngle, float rotaryEndAngle,
                              juce::Slider&) override;

        juce::Colour accentCol;

    private:
        static juce::String fmtValue(const juce::Slider&);
    };

    LookAndFeel lnf;
    juce::Slider slider;
    juce::Label  label;
    juce::Colour accentColour;
};

// =============================================================================
// Implementation
// =============================================================================
inline DrumKnob::DrumKnob(juce::String labelText, double min, double max,
                           double value, juce::Colour ac)
    : lnf(ac), accentColour(ac)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(min, max);
    slider.setValue(value);
    slider.setLookAndFeel(&lnf);
    slider.setScrollWheelEnabled(false);
    slider.setMouseDragSensitivity(300);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(UITheme::fontSmall());
    label.setColour(juce::Label::textColourId, UITheme::textMuted());
    label.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(slider);
    addAndMakeVisible(label);
}

inline void DrumKnob::resized()
{
    auto area = getLocalBounds();
    label.setBounds(area.removeFromBottom(13));
    slider.setBounds(area);
}

inline void DrumKnob::setLabelText(const juce::String& t)
{
    label.setText(t, juce::dontSendNotification);
}

inline juce::String DrumKnob::LookAndFeel::fmtValue(const juce::Slider& s)
{
    const double v   = s.getValue();
    const double mn  = s.getMinimum();
    const double mx  = s.getMaximum();
    const double rng = mx - mn;

    if (rng <= 2.01 && mn >= -1.01)          return juce::String(juce::roundToInt(v * 100)) + "%";
    if (mx >= 1000.0)
        return v >= 1000.0 ? juce::String(v / 1000.0, 1) + "k"
                           : juce::String(juce::roundToInt(v));
    if (rng <= 10.0)                          return juce::String(v, 2);
    if (rng <= 100.0)                         return juce::String(v, 1);
    return juce::String(juce::roundToInt(v));
}

inline void DrumKnob::LookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x, int y, int w, int h,
    float pos, float startA, float endA,
    juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(3.0f);
    const auto  ctr    = bounds.getCentre();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

    // ── Arc track geometry ────────────────────────────────────────────────────
    const float arcThick  = juce::jmax(3.5f, radius * 0.17f);
    const float arcRadius = radius - arcThick * 0.5f - 1.0f;
    const float angle     = juce::jmap(pos, startA, endA);

    // Background arc (dim track)
    juce::Path bgArc;
    bgArc.addCentredArc(ctr.x, ctr.y, arcRadius, arcRadius, 0.0f, startA, endA, true);
    g.setColour(UITheme::panelInset().withBrightness(1.3f));
    g.strokePath(bgArc, juce::PathStrokeType(arcThick, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Active arc
    if (pos > 0.002f)
    {
        juce::Path activeArc;
        activeArc.addCentredArc(ctr.x, ctr.y, arcRadius, arcRadius, 0.0f, startA, angle, true);
        g.setColour(accentCol);
        g.strokePath(activeArc, juce::PathStrokeType(arcThick, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        // Glowing tip dot
        auto tip = ctr.getPointOnCircumference(arcRadius, angle);
        g.setColour(accentCol.withAlpha(0.55f));
        g.fillEllipse(tip.x - arcThick * 0.8f, tip.y - arcThick * 0.8f,
                      arcThick * 1.6f, arcThick * 1.6f);
        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.fillEllipse(tip.x - arcThick * 0.35f, tip.y - arcThick * 0.35f,
                      arcThick * 0.7f, arcThick * 0.7f);
    }

    // ── Knob body ─────────────────────────────────────────────────────────────
    const float bodyR = arcRadius - arcThick * 0.5f - 2.0f;

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.40f));
    g.fillEllipse(ctr.x - bodyR + 1.0f, ctr.y - bodyR + 3.0f, bodyR * 2.0f, bodyR * 2.0f);

    // Body gradient (subtle, top-lit)
    juce::ColourGradient bodyGrad(UITheme::bgElevated().withBrightness(1.15f),
                                   ctr.x, ctr.y - bodyR * 0.6f,
                                   UITheme::bgBase(),
                                   ctr.x, ctr.y + bodyR * 0.6f, false);
    g.setGradientFill(bodyGrad);
    g.fillEllipse(ctr.x - bodyR, ctr.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    // Body border
    g.setColour(UITheme::borderSubtle().withAlpha(0.6f));
    g.drawEllipse(ctr.x - bodyR, ctr.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // ── Pointer line ──────────────────────────────────────────────────────────
    auto p1 = ctr.getPointOnCircumference(bodyR * 0.28f, angle);
    auto p2 = ctr.getPointOnCircumference(bodyR * 0.78f, angle);
    g.setColour(UITheme::textMain().withAlpha(0.85f));
    g.drawLine({p1, p2}, 2.2f);

    // Centre dot
    g.setColour(accentCol.withAlpha(0.60f));
    g.fillEllipse(ctr.x - 2.5f, ctr.y - 2.5f, 5.0f, 5.0f);

    // ── Value text (always visible, inside body) ──────────────────────────────
    const juce::String valStr = fmtValue(slider);
    g.setColour(UITheme::textDim().withAlpha(slider.isMouseOverOrDragging() ? 1.0f : 0.65f));
    g.setFont(UITheme::fontMicro().withHeight(8.5f));
    g.drawText(valStr,
               juce::Rectangle<float>(ctr.x - bodyR, ctr.y + bodyR * 0.30f,
                                      bodyR * 2.0f, 12.0f),
               juce::Justification::centred, false);
}
