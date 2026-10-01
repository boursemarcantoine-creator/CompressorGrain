#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::black);
        setColour (juce::Label::textColourId, juce::Colour (0xff1a1a1a));
    }

    static void drawBrushedPanel (juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour (juce::Colour (0xff0c0c0c));
        g.fillRect (bounds);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider&) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // drop shadow, detaches the knob from the black background
        juce::ColourGradient shadowGrad (juce::Colours::black.withAlpha (0.55f), centre.x, centre.y + radius * 0.25f,
                                          juce::Colours::transparentBlack, centre.x, centre.y + radius * 1.3f, true);
        g.setGradientFill (shadowGrad);
        g.fillEllipse (centre.x - radius * 1.15f, centre.y - radius * 0.85f, radius * 2.3f, radius * 2.3f);

        // knob body: subtle radial gradient for a slightly domed, realistic look
        juce::ColourGradient bodyGradient (juce::Colour (0xff3a3a3a), centre.x - radius * 0.35f, centre.y - radius * 0.4f,
                                            juce::Colour (0xff050505), centre.x, centre.y, true);
        bodyGradient.addColour (0.7, juce::Colour (0xff161616));
        g.setGradientFill (bodyGradient);
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        // thin stainless-steel bevel ring, with a faint inner shadow for depth
        const float ringThickness = 5.5f;
        juce::ColourGradient ringGradient (juce::Colour (0xffe6e6e6), centre.x - radius, centre.y - radius,
                                            juce::Colour (0xff5c5c5c), centre.x + radius, centre.y + radius, true);
        g.setGradientFill (ringGradient);
        g.drawEllipse (centre.x - radius + ringThickness * 0.5f, centre.y - radius + ringThickness * 0.5f,
                        radius * 2.0f - ringThickness, radius * 2.0f - ringThickness, ringThickness);

        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawEllipse (centre.x - radius + ringThickness, centre.y - radius + ringThickness,
                        radius * 2.0f - ringThickness * 2.0f, radius * 2.0f - ringThickness * 2.0f, 1.0f);

        // specular highlight, upper-left, like light catching a domed cap
        juce::ColourGradient highlight (juce::Colours::white.withAlpha (0.18f),
                                         centre.x - radius * 0.35f, centre.y - radius * 0.45f,
                                         juce::Colours::transparentWhite,
                                         centre.x, centre.y, true);
        g.setGradientFill (highlight);
        g.fillEllipse (centre.x - radius * 0.75f, centre.y - radius * 0.75f, radius * 1.1f, radius * 0.9f);

        // pointer, silver-grey
        juce::Path pointer;
        float pointerLength = radius * 0.62f;
        pointer.addRectangle (-1.6f, -pointerLength, 3.2f, pointerLength * 0.9f);
        g.setColour (juce::Colour (0xffb9b9b9));
        g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                bool shouldDrawButtonAsHighlighted, bool) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        const bool isOn = button.getToggleState();

        // dark square housing
        g.setColour (juce::Colour (0xffd2d2d2));
        g.fillRect (bounds);

        auto inner = bounds.reduced (2.5f);
        juce::Colour litColour (0xffffa542);
        juce::Colour offColour (0xff1c1c1c);
        g.setColour (isOn ? litColour : offColour);
        g.fillRect (inner);

        if (isOn)
        {
            juce::ColourGradient glow (litColour.withAlpha (0.5f), inner.getCentreX(), inner.getCentreY(),
                                        juce::Colours::transparentBlack, inner.getX(), inner.getY(), true);
            g.setGradientFill (glow);
            g.fillRect (inner);
        }

        g.setColour (juce::Colours::white.withAlpha (shouldDrawButtonAsHighlighted ? 0.25f : 0.1f));
        g.drawRect (bounds, 1.0f);
    }
};
