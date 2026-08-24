#pragma once
#include <JuceHeader.h>
#include "UITheme.h"

// =============================================================================
// DrumWaveform — Mini waveform display for sample preview
// =============================================================================
class DrumWaveform : public juce::Component
{
public:
    DrumWaveform();

    void setAudioData(const float* data, int numSamples);
    void clear();

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::Array<float> samples;
    bool hasData = false;
};

// =============================================================================
// Implementation
// =============================================================================
inline DrumWaveform::DrumWaveform()
{
    setInterceptsMouseClicks(false, false);
}

inline void DrumWaveform::resized()
{
    // Layout handled by parent
}

inline void DrumWaveform::setAudioData(const float* data, int numSamples)
{
    samples.clear();
    if (data == nullptr || numSamples <= 0)
    {
        hasData = false;
        repaint();
        return;
    }

    // Downsample for display
    const int maxDisplaySamples = 512;
    const int step = juce::jmax(1, numSamples / maxDisplaySamples);

    for (int i = 0; i < numSamples; i += step)
    {
        float maxVal = 0.0f;
        int end = juce::jmin(i + step, numSamples);
        for (int j = i; j < end; ++j)
            maxVal = juce::jmax(maxVal, std::abs(data[j]));
        samples.add(maxVal);
    }

    hasData = true;
    repaint();
}

inline void DrumWaveform::clear()
{
    samples.clear();
    hasData = false;
    repaint();
}

inline void DrumWaveform::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Background
    UITheme::fillInset(g, b, UITheme::panelInset(), UITheme::cornerRadiusSmall());

    if (!hasData || samples.isEmpty())
    {
        // Placeholder text
        g.setColour(UITheme::textMuted());
        g.setFont(UITheme::fontSmall());
        g.drawText("NO SAMPLE", b, juce::Justification::centred);
        return;
    }

    // Waveform
    auto plot = b.reduced(4.0f);
    float midY = plot.getCentreY();
    float height = plot.getHeight() * 0.5f;
    float stepX = plot.getWidth() / (float)samples.size();

    juce::Path wave;
    wave.startNewSubPath(plot.getX(), midY);

    for (int i = 0; i < samples.size(); ++i)
    {
        float x = plot.getX() + i * stepX;
        float y = midY - samples[i] * height;
        wave.lineTo(x, y);
    }

    for (int i = samples.size() - 1; i >= 0; --i)
    {
        float x = plot.getX() + i * stepX;
        float y = midY + samples[i] * height;
        wave.lineTo(x, y);
    }

    wave.closeSubPath();

    g.setColour(UITheme::accentCyan().withAlpha(0.25f));
    g.fillPath(wave);

    g.setColour(UITheme::accentCyan().withAlpha(0.7f));
    g.strokePath(wave, juce::PathStrokeType(1.0f));

    // Center line
    g.setColour(UITheme::textMuted().withAlpha(0.2f));
    g.drawHorizontalLine(juce::roundToInt(midY), plot.getX(), plot.getRight());
}
