#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VUMeter.h"

// Same bezel/screen/backlight as the VU meter, but instead of a needle it
// draws the Grain EQ response: a flat line at 0%, morphing into the actual
// low/mid bump + high sizzle shape as the Grain knob is turned up.
class EQCurveDisplay : public juce::Component
{
public:
    void setBacklightColour (juce::Colour glowColour) { backlightColour = glowColour; }

    // 0 = flat line, 1 = full EQ curve (matches the Grain knob, 0-100%)
    void setGrainAmount (float amount)
    {
        grainAmount = juce::jlimit (0.0f, 1.0f, amount);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto screen = VintageScreen::drawBezelAndGlow (g, bounds, backlightColour);

        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawText ("EQ", screen, juce::Justification::centred);

        // --- Build the curve: 20Hz .. 20kHz across the width, on a log axis
        const float lowGainDb   = grainAmount * 7.0f;  // matches updateGrainFilters()
        const float midGainDb   = grainAmount * 5.0f;
        const float shelfGainDb = grainAmount * 6.0f;  // visual stand-in for the 6kHz sizzle mix

        auto bumpDb = [] (float freq, float centreFreq, float widthOctaves, float peakGainDb)
        {
            float x = std::log2 (freq / centreFreq) / widthOctaves;
            return peakGainDb * std::exp (-(x * x) * 0.5f);
        };

        auto shelfDb = [] (float freq, float centreFreq, float gainDb)
        {
            float ratio = centreFreq / freq;
            return gainDb / (1.0f + ratio * ratio);
        };

        juce::Path curve;
        const int numPoints = 48;
        const float minFreq = 20.0f, maxFreq = 20000.0f;
        const float centreY = screen.getCentreY();
        const float dbToPixels = (screen.getHeight() * 0.42f) / 10.0f; // +-10dB fills most of the screen height

        for (int i = 0; i < numPoints; ++i)
        {
            float t = (float) i / (float) (numPoints - 1);
            float freq = minFreq * std::pow (maxFreq / minFreq, t);

            float db = bumpDb (freq, 72.0f, 0.6f, lowGainDb)
                      + bumpDb (freq, 621.0f, 0.5f, midGainDb)
                      + shelfDb (freq, 6000.0f, shelfGainDb);

            float x = screen.getX() + t * screen.getWidth();
            float y = centreY - db * dbToPixels;

            if (i == 0) curve.startNewSubPath (x, y);
            else        curve.lineTo (x, y);
        }

        g.setColour (juce::Colour (0xff1a1a1a));
        g.strokePath (curve, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));

        // label, raised ~15px from the very bottom
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.setFont (juce::Font (10.0f));
        g.drawText ("grain eq", getLocalBounds().removeFromBottom (29).removeFromTop (14), juce::Justification::centred);
    }

private:
    juce::Colour backlightColour { 0xffffa542 };
    float grainAmount = 0.0f;
};
