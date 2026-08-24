#pragma once
#include <JuceHeader.h>

// =============================================================================
// LayoutGrid — Lightweight responsive layout system for JUCE components
// =============================================================================
// No external dependencies. Pure JUCE. Based on ratios and flex distribution.
// Designed for the UWdeVST Drum Synth UI refont.
// =============================================================================

namespace LayoutGrid
{
    // =========================================================================
    // LayoutRegion — Defines a rectangular area with margins and gaps
    // =========================================================================
    struct Region
    {
        juce::Rectangle<int> bounds;
        int margin = 0;
        int gap = 0;

        Region() = default;
        explicit Region(juce::Rectangle<int> b, int m = 0, int g = 0)
            : bounds(b), margin(m), gap(g) {}

        // Get the content area (bounds minus margin)
        juce::Rectangle<int> content() const
        {
            return bounds.reduced(margin);
        }

        // Split horizontally: left portion by ratio (0..1)
        std::pair<Region, Region> splitH(float ratio) const
        {
            auto c = content();
            int splitX = juce::roundToInt(c.getWidth() * ratio);
            auto left = c.withWidth(splitX);
            auto right = c.withX(c.getX() + splitX).withWidth(c.getWidth() - splitX);
            return { Region(left, 0, gap), Region(right, 0, gap) };
        }

        // Split vertically: top portion by ratio (0..1)
        std::pair<Region, Region> splitV(float ratio) const
        {
            auto c = content();
            int splitY = juce::roundToInt(c.getHeight() * ratio);
            auto top = c.withHeight(splitY);
            auto bottom = c.withY(c.getY() + splitY).withHeight(c.getHeight() - splitY);
            return { Region(top, 0, gap), Region(bottom, 0, gap) };
        }

        // Remove a header strip from top
        std::pair<Region, Region> header(int height) const
        {
            auto c = content();
            auto h = c.removeFromTop(height);
            return { Region(h, 0, 0), Region(c, 0, gap) };
        }

        // Remove a footer strip from bottom
        std::pair<Region, Region> footer(int height) const
        {
            auto c = content();
            auto f = c.removeFromBottom(height);
            return { Region(c, 0, gap), Region(f, 0, 0) };
        }
    };

    // =========================================================================
    // Grid cell calculator
    // =========================================================================
    inline juce::Rectangle<int> cell(const Region& region,
                                      int col, int row,
                                      int numCols, int numRows)
    {
        auto c = region.content();
        int cellW = (c.getWidth() - region.gap * (numCols - 1)) / numCols;
        int cellH = (c.getHeight() - region.gap * (numRows - 1)) / numRows;

        return juce::Rectangle<int>(
            c.getX() + col * (cellW + region.gap),
            c.getY() + row * (cellH + region.gap),
            cellW, cellH
        );
    }

    // =========================================================================
    // Flex distribution — Distribute components evenly in a row/column
    // =========================================================================
    inline void flexRow(const Region& region,
                        juce::Component** components, int count,
                        int fixedHeight = -1)
    {
        auto c = region.content();
        int itemW = (c.getWidth() - region.gap * (count - 1)) / count;
        int itemH = fixedHeight > 0 ? fixedHeight : c.getHeight();
        int y = fixedHeight > 0 ? c.getCentreY() - itemH / 2 : c.getY();

        for (int i = 0; i < count; ++i)
        {
            if (components[i] != nullptr)
            {
                components[i]->setBounds(
                    c.getX() + i * (itemW + region.gap),
                    y, itemW, itemH
                );
            }
        }
    }

    inline void flexCol(const Region& region,
                        juce::Component** components, int count,
                        int fixedWidth = -1)
    {
        auto c = region.content();
        int itemH = (c.getHeight() - region.gap * (count - 1)) / count;
        int itemW = fixedWidth > 0 ? fixedWidth : c.getWidth();
        int x = fixedWidth > 0 ? c.getCentreX() - itemW / 2 : c.getX();

        for (int i = 0; i < count; ++i)
        {
            if (components[i] != nullptr)
            {
                components[i]->setBounds(
                    x,
                    c.getY() + i * (itemH + region.gap),
                    itemW, itemH
                );
            }
        }
    }

    // =========================================================================
    // Convenience: place a single component filling the region
    // =========================================================================
    inline void fill(juce::Component& comp, const Region& region)
    {
        comp.setBounds(region.content());
    }

    // =========================================================================
    // Convenience: place a component with aspect ratio preservation
    // =========================================================================
    inline void fit(juce::Component& comp, const Region& region, float aspectRatio)
    {
        auto c = region.content();
        int w = c.getWidth();
        int h = juce::roundToInt(w / aspectRatio);

        if (h > c.getHeight())
        {
            h = c.getHeight();
            w = juce::roundToInt(h * aspectRatio);
        }

        int x = c.getX() + (c.getWidth() - w) / 2;
        int y = c.getY() + (c.getHeight() - h) / 2;

        comp.setBounds(x, y, w, h);
    }

} // namespace LayoutGrid
