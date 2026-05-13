#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumSelector — Hardware dark dropdown / combo box
// =============================================================================
class DrumSelector : public juce::ComboBox
{
public:
    DrumSelector();
    ~DrumSelector() override = default;

    void paint(juce::Graphics& g) override;

private:
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        juce::Font getComboBoxFont(juce::ComboBox&) override
        {
            return UITheme::fontLabel();
        }

        juce::Label* createComboBoxTextBox(juce::ComboBox& box) override
        {
            auto* label = juce::LookAndFeel_V4::createComboBoxTextBox(box);
            label->setFont(UITheme::fontLabel());
            label->setColour(juce::Label::textColourId, UITheme::textMain());
            label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            return label;
        }

        void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
        {
            label.setBounds(10, 0, box.getWidth() - 30, box.getHeight());
        }

        void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&) override
        {
            auto area = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height).reduced(0.5f);
            UITheme::fillPanel(g, area, UITheme::panelBase(), UITheme::cornerRadiusSmall());

            // Arrow
            juce::Path arrow;
            const float cx = area.getRight() - 14.0f;
            const float cy = area.getCentreY() + 0.5f;
            arrow.startNewSubPath(cx - 3.5f, cy - 2.0f);
            arrow.lineTo(cx, cy + 2.5f);
            arrow.lineTo(cx + 3.5f, cy - 2.0f);
            g.setColour(UITheme::accentOrange());
            g.strokePath(arrow, juce::PathStrokeType(1.5f));
        }

        juce::Font getPopupMenuFont() override
        {
            return UITheme::fontLabel();
        }

        void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
        {
            UITheme::fillPanel(g, juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height),
                               UITheme::panelBase(), UITheme::cornerRadiusSmall());
        }

        void drawPopupMenuItem(juce::Graphics& g,
                               const juce::Rectangle<int>& area,
                               bool isSeparator,
                               bool isActive,
                               bool isHighlighted,
                               bool, bool,
                               const juce::String& text,
                               const juce::String&, const juce::Drawable*,
                               const juce::Colour* textColourToUse) override
        {
            if (isSeparator)
            {
                g.setColour(juce::Colours::white.withAlpha(0.06f));
                g.drawHorizontalLine(area.getCentreY(), (float)area.getX() + 8.0f, (float)area.getRight() - 8.0f);
                return;
            }

            auto row = area.toFloat().reduced(2.0f, 1.0f);
            if (isHighlighted)
            {
                g.setColour(UITheme::accentOrange().withAlpha(0.15f));
                g.fillRoundedRectangle(row, UITheme::cornerRadiusSmall());
            }

            g.setColour(textColourToUse != nullptr ? *textColourToUse
                        : (isActive ? UITheme::textMain() : UITheme::textDim()));
            g.setFont(UITheme::fontLabel());
            g.drawText(text, area.reduced(10, 0), juce::Justification::centredLeft, false);
        }
    };

    LookAndFeel lnf;
};

// =============================================================================
// Implementation
// =============================================================================
inline DrumSelector::DrumSelector()
{
    setLookAndFeel(&lnf);
}

inline void DrumSelector::paint(juce::Graphics& g)
{
    juce::ComboBox::paint(g);
}
