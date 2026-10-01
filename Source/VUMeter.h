#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Shared screen bezel + orange backlit glow, used by both the VU meter and
// the EQ curve display so the two cadrans match exactly.
struct VintageScreen
{
    static juce::Rectangle<float> drawBezelAndGlow (juce::Graphics& g, juce::Rectangle<float> bounds,
                                                      juce::Colour backlightColour)
    {
        juce::ColourGradient bezelGradient (juce::Colour (0xff5a5a5a), bounds.getX(), bounds.getY(),
                                             juce::Colour (0xff383838), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bezelGradient);
        g.fillRoundedRectangle (bounds, 4.0f);

        auto screen = bounds.reduced (4.0f);

        juce::ColourGradient glow (backlightColour, screen.getCentreX(), screen.getBottom() * 1.05f,
                                    backlightColour.darker (2.2f), screen.getX(), screen.getY(), true);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (screen, 2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillRoundedRectangle (screen, 2.0f);

        // subtle bevel on the screen edges: light catching the top-left,
        // shadow on the bottom-right, without changing the bezel thickness
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawLine (screen.getX(), screen.getY(), screen.getRight(), screen.getY(), 1.0f);
        g.drawLine (screen.getX(), screen.getY(), screen.getX(), screen.getBottom(), 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (screen.getX(), screen.getBottom(), screen.getRight(), screen.getBottom(), 1.0f);
        g.drawLine (screen.getRight(), screen.getY(), screen.getRight(), screen.getBottom(), 1.0f);

        return screen;
    }
};

class VUMeter : public juce::Component, private juce::Timer
{
public:
    explicit VUMeter (juce::String labelText) : label (std::move (labelText))
    {
        startTimerHz (30);
    }

    // linear peak level, 0 = silence, 1 = 0dBFS
    void pushLevel (float newLevel)
    {
        targetLevel = juce::jlimit (0.0f, 2.0f, newLevel);
    }

    void setBacklightColour (juce::Colour glowColour) { backlightColour = glowColour; }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        auto screen = VintageScreen::drawBezelAndGlow (g, bounds, backlightColour);

        // arc scale
        const float cx = screen.getCentreX();
        const float cy = screen.getBottom() + screen.getHeight() * 0.55f;
        const float radius = screen.getHeight() * 1.55f;
        const float startAngle = juce::degreesToRadians (-135.0f);
        const float endAngle   = juce::degreesToRadians (-45.0f);

        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.setFont (juce::Font (9.0f));

        const int numTicks = 10;
        for (int i = 0; i <= numTicks; ++i)
        {
            float t = (float) i / (float) numTicks;
            float angle = startAngle + t * (endAngle - startAngle);
            juce::Point<float> inner (cx + std::cos (angle) * (radius - 8.0f), cy + std::sin (angle) * (radius - 8.0f));
            juce::Point<float> outer (cx + std::cos (angle) * radius, cy + std::sin (angle) * radius);
            g.drawLine ({ inner, outer }, i == numTicks - 3 ? 1.6f : 0.9f); // emphasise near 0dB
        }

        // a few dB reference numbers along the scale
        g.setFont (juce::Font (6.5f));
        auto drawDbLabel = [&] (float db, const juce::String& text)
        {
            float norm = juce::jlimit (0.0f, 1.0f, (db + 20.0f) / 23.0f);
            float a = startAngle + norm * (endAngle - startAngle);
            float labelRadius = radius + 11.0f;
            juce::Point<float> p (cx + std::cos (a) * labelRadius, cy + std::sin (a) * labelRadius);
            g.drawText (text, juce::Rectangle<float> (p.x - 9.0f, p.y - 5.0f, 18.0f, 10.0f), juce::Justification::centred);
        };
        drawDbLabel (-20.0f, "-20");
        drawDbLabel (-10.0f, "-10");
        drawDbLabel (-5.0f, "-5");
        drawDbLabel (0.0f, "0");
        drawDbLabel (3.0f, "+3");

        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawText ("VU", screen, juce::Justification::centred);

        // needle
        float levelDb = juce::Decibels::gainToDecibels (juce::jmax (0.001f, smoothedLevel), -30.0f);
        float norm = juce::jlimit (0.0f, 1.0f, (levelDb + 20.0f) / 23.0f); // -20dB..+3dB range
        float needleAngle = startAngle + norm * (endAngle - startAngle);

        juce::Path needle;
        needle.startNewSubPath (cx, cy);
        needle.lineTo (cx + std::cos (needleAngle) * (radius - 5.0f), cy + std::sin (needleAngle) * (radius - 5.0f));
        g.setColour (juce::Colour (0xff1a1a1a));
        g.strokePath (needle, juce::PathStrokeType (1.6f));

        g.setColour (juce::Colour (0xff1a1a1a));
        g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);

        // label, raised ~15px from the very bottom
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.setFont (juce::Font (10.0f));
        g.drawText (label, getLocalBounds().removeFromBottom (29).removeFromTop (14), juce::Justification::centred);
    }

private:
    void timerCallback() override
    {
        // simple VU ballistics: fast-ish attack, slower release
        const float coeff = targetLevel > smoothedLevel ? 0.5f : 0.12f;
        smoothedLevel += coeff * (targetLevel - smoothedLevel);
        repaint();
    }

    juce::String label;
    juce::Colour backlightColour { 0xffffa542 }; // warm vintage-lamp orange by default
    float targetLevel = 0.0f;
    float smoothedLevel = 0.0f;
};
