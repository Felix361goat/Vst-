#include "UI/NogLookAndFeel.h"

namespace nog::ui
{
    namespace
    {
        constexpr float knobThickness = 3.5f;
        constexpr float modRingInset  = 4.0f;

        /** Knobs advertise their modulation depth through a slider property, so
            the look and feel stays decoupled from the modulation system. */
        constexpr const char* modDepthProperty = "modDepth";

        juce::Font uiFont (float height, bool bold = false)
        {
            return juce::Font (juce::FontOptions (height,
                                                  bold ? juce::Font::bold : juce::Font::plain));
        }
    }

    NogLookAndFeel::NogLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, colours::background);
        setColour (juce::Label::textColourId,                 colours::text);
        setColour (juce::Slider::textBoxTextColourId,         colours::text);
        setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
        setColour (juce::ComboBox::backgroundColourId,        colours::panelHeader);
        setColour (juce::ComboBox::textColourId,              colours::text);
        setColour (juce::ComboBox::outlineColourId,           colours::border);
        setColour (juce::ComboBox::arrowColourId,             colours::dimText);
        setColour (juce::PopupMenu::backgroundColourId,       colours::panelHeader);
        setColour (juce::PopupMenu::textColourId,             colours::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha (0.25f));
        setColour (juce::PopupMenu::highlightedTextColourId,  juce::Colours::white);
        setColour (juce::TextButton::buttonColourId,          colours::panelHeader);
        setColour (juce::TextButton::textColourOffId,         colours::text);
        setColour (juce::TextButton::textColourOnId,          juce::Colours::white);
        setColour (juce::TextEditor::backgroundColourId,      colours::panelHeader);
        setColour (juce::TextEditor::textColourId,            colours::text);
        setColour (juce::TextEditor::outlineColourId,         colours::border);
        setColour (juce::TabbedComponent::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::TabbedComponent::outlineColourId,    colours::border);
    }

    juce::Font NogLookAndFeel::getLabelFont (juce::Label&)            { return uiFont (12.0f); }
    juce::Font NogLookAndFeel::getComboBoxFont (juce::ComboBox&)      { return uiFont (12.0f); }
    juce::Font NogLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return uiFont (12.0f); }

    void NogLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        label.setBounds (6, 0, box.getWidth() - 22, box.getHeight());
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centredLeft);
    }

    void NogLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosProportional, float rotaryStartAngle,
                                           float rotaryEndAngle, juce::Slider& slider)
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (modRingInset);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto angle  = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        const auto arcRadius = radius - knobThickness;

        // -- track ----------------------------------------------------------
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);

        g.setColour (colours::knobTrack);
        g.strokePath (track, juce::PathStrokeType (knobThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // -- value ----------------------------------------------------------
        // Bipolar controls fill outwards from the centre, so a pan or a detune
        // reads as an offset rather than an amount.
        const auto isBipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        const auto originProportion = isBipolar
            ? static_cast<float> ((0.0 - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()))
            : 0.0f;
        const auto originAngle = rotaryStartAngle + originProportion * (rotaryEndAngle - rotaryStartAngle);

        if (std::abs (angle - originAngle) > 1.0e-3f)
        {
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                 juce::jmin (originAngle, angle), juce::jmax (originAngle, angle), true);

            g.setColour (slider.isEnabled() ? colours::accent : colours::dimText);
            g.strokePath (value, juce::PathStrokeType (knobThickness, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }

        // -- modulation range ------------------------------------------------
        const auto modDepth = static_cast<float> (slider.getProperties().getWithDefault (modDepthProperty, 0.0));

        if (std::abs (modDepth) > 1.0e-4f)
        {
            const auto sweep    = rotaryEndAngle - rotaryStartAngle;
            const auto modStart = juce::jlimit (rotaryStartAngle, rotaryEndAngle, angle);
            const auto modEnd   = juce::jlimit (rotaryStartAngle, rotaryEndAngle, angle + modDepth * sweep);

            juce::Path modulation;
            modulation.addCentredArc (centre.x, centre.y, arcRadius + knobThickness, arcRadius + knobThickness,
                                      0.0f, juce::jmin (modStart, modEnd), juce::jmax (modStart, modEnd), true);

            g.setColour (colours::modulation.withAlpha (0.85f));
            g.strokePath (modulation, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                            juce::PathStrokeType::rounded));
        }

        // -- body and pointer ------------------------------------------------
        const auto bodyRadius = arcRadius - knobThickness * 1.4f;

        g.setColour (colours::panelHeader);
        g.fillEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre));

        g.setColour (colours::border);
        g.drawEllipse (juce::Rectangle<float> (bodyRadius * 2.0f, bodyRadius * 2.0f).withCentre (centre), 1.0f);

        juce::Path pointer;
        const auto pointerLength = bodyRadius * 0.75f;
        pointer.addRoundedRectangle (-1.0f, -bodyRadius + 1.0f, 2.0f, pointerLength, 1.0f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));

        g.setColour (slider.isEnabled() ? colours::text : colours::dimText);
        g.fillPath (pointer);
    }

    void NogLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float, float,
                                           juce::Slider::SliderStyle style, juce::Slider& slider)
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();

        if (style == juce::Slider::LinearHorizontal)
        {
            const auto track = bounds.withSizeKeepingCentre (bounds.getWidth(), 4.0f);

            g.setColour (colours::knobTrack);
            g.fillRoundedRectangle (track, 2.0f);

            // Bipolar sliders fill outwards from zero, matching the rotaries.
            // A matrix amount of 0 should read as empty, not half full.
            const auto isBipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
            const auto originX = isBipolar
                ? track.getX() + track.getWidth()
                      * static_cast<float> ((0.0 - slider.getMinimum())
                                            / (slider.getMaximum() - slider.getMinimum()))
                : track.getX();

            g.setColour (colours::accent);
            g.fillRoundedRectangle (track.withLeft (juce::jmin (originX, sliderPos))
                                         .withRight (juce::jmax (originX, sliderPos)), 2.0f);

            g.setColour (colours::text);
            g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ sliderPos, bounds.getCentreY() }));
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                              0.0f, 0.0f, style, slider);
        }
    }

    void NogLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                       int, int, int, int, juce::ComboBox& box)
    {
        const auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                                    static_cast<float> (width),
                                                    static_cast<float> (height));

        g.setColour (colours::panelHeader);
        g.fillRoundedRectangle (bounds, 3.0f);

        g.setColour (box.hasKeyboardFocus (false) ? colours::accent : colours::border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);

        juce::Path arrow;
        const auto arrowX = bounds.getRight() - 13.0f;
        const auto arrowY = bounds.getCentreY() - 1.5f;

        arrow.startNewSubPath (arrowX, arrowY);
        arrow.lineTo (arrowX + 4.0f, arrowY + 4.0f);
        arrow.lineTo (arrowX + 8.0f, arrowY);

        g.setColour (colours::dimText);
        g.strokePath (arrow, juce::PathStrokeType (1.4f));
    }

    void NogLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour&, bool shouldDrawButtonAsHighlighted,
                                               bool shouldDrawButtonAsDown)
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const auto on     = button.getToggleState() || shouldDrawButtonAsDown;

        auto fill = on ? colours::accent.withAlpha (0.22f) : colours::panelHeader;

        if (shouldDrawButtonAsHighlighted)
            fill = fill.brighter (0.15f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 3.0f);

        g.setColour (on ? colours::accent : colours::border);
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    }

    void NogLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                           bool shouldDrawButtonAsHighlighted, bool)
    {
        // Toggles are drawn as small indicator pills rather than tick boxes,
        // which suits a synth panel and stays legible at this size.
        const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const auto on     = button.getToggleState();

        auto fill = on ? colours::accent.withAlpha (0.28f) : colours::panelHeader;

        if (shouldDrawButtonAsHighlighted)
            fill = fill.brighter (0.15f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 3.0f);

        g.setColour (on ? colours::accent : colours::border);
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

        g.setColour (on ? juce::Colours::white : colours::dimText);
        g.setFont (uiFont (11.0f, on));
        g.drawText (button.getButtonText(), bounds, juce::Justification::centred, false);
    }
}
