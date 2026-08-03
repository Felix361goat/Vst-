#include "UI/NogLookAndFeel.h"

namespace nog::ui
{
    namespace
    {
        constexpr float knobThickness = 3.0f;
        constexpr float modRingInset  = 3.0f;

        /** Gap between the value arc and the edge of the cap. Kept small so the
            cap is as large as it can be - the moulded shading needs the room to
            be legible at the size these knobs are drawn. */
        constexpr float capGap = 2.0f;

        /** Knobs advertise their modulation depth through a slider property, so
            the look and feel stays decoupled from the modulation system. */
        constexpr const char* modDepthProperty = "modDepth";

        juce::Font uiFont (float height, bool bold = false)
        {
            return juce::Font (juce::FontOptions (height,
                                                  bold ? juce::Font::bold : juce::Font::plain));
        }
    }

    void paintGlassPanel (juce::Graphics& g, juce::Rectangle<float> bounds,
                          float cornerSize, bool highlightTop)
    {
        // The tint is graded rather than flat: heavier at the top where text
        // sits, lighter towards the bottom so more of the artwork comes through.
        juce::ColourGradient tint (colours::panel.withAlpha (0.90f),
                                   bounds.getCentreX(), bounds.getY(),
                                   colours::panel.withAlpha (0.72f),
                                   bounds.getCentreX(), bounds.getBottom(), false);

        g.setGradientFill (tint);
        g.fillRoundedRectangle (bounds, cornerSize);

        // A lit top edge is most of what makes a surface read as glass.
        if (highlightTop && bounds.getHeight() > 4.0f)
        {
            juce::Graphics::ScopedSaveState saved (g);

            juce::Path clip;
            clip.addRoundedRectangle (bounds, cornerSize);
            g.reduceClipRegion (clip);

            juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.16f),
                                        bounds.getCentreX(), bounds.getY(),
                                        juce::Colours::white.withAlpha (0.0f),
                                        bounds.getCentreX(), bounds.getY() + bounds.getHeight() * 0.35f,
                                        false);

            g.setGradientFill (sheen);
            g.fillRect (bounds.withHeight (bounds.getHeight() * 0.35f));
        }

        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), cornerSize, 1.0f);
    }

    NogLookAndFeel::NogLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, colours::background);
        setColour (juce::Label::textColourId,                 colours::text);
        setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white);
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

        // The cap colour comes from the knob itself; anything that has not set
        // one falls back to the interface accent.
        const auto capColour = [&slider]
        {
            const auto stored = slider.getProperties().getWithDefault (knobColourProperty, 0);
            const auto argb   = static_cast<juce::uint32> (static_cast<juce::int64> (stored));

            return argb != 0 ? juce::Colour (argb) : colours::accent;
        }();

        const auto enabled = slider.isEnabled();
        const auto cap     = enabled ? capColour : capColour.withSaturation (0.15f).withBrightness (0.35f);

        // -- value track and arc --------------------------------------------
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);

        g.setColour (colours::knobTrack);
        g.strokePath (track, juce::PathStrokeType (knobThickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

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

            g.setColour (cap.brighter (0.35f));
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

            g.setColour (colours::modulation.withAlpha (0.9f));
            g.strokePath (modulation, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                            juce::PathStrokeType::rounded));
        }

        // -- the cap ----------------------------------------------------------
        // Built up in layers the way a moulded plastic button reads: a lit top,
        // a dark rim where it curves away, a hard specular highlight, and a
        // softer bounce light near the bottom edge.
        const auto capRadius = arcRadius - knobThickness * 0.5f - capGap;

        if (capRadius <= 1.0f)
            return;

        const auto capBounds = juce::Rectangle<float> (capRadius * 2.0f, capRadius * 2.0f).withCentre (centre);

        {
            juce::ColourGradient body (cap.brighter (0.55f), capBounds.getCentreX(), capBounds.getY(),
                                       cap.darker (0.55f),   capBounds.getCentreX(), capBounds.getBottom(),
                                       false);
            body.addColour (0.55, cap);

            g.setGradientFill (body);
            g.fillEllipse (capBounds);
        }

        // Darkening around the rim is what turns a flat disc into a dome.
        {
            juce::ColourGradient rim (juce::Colours::transparentBlack, centre.x, centre.y,
                                      juce::Colours::black.withAlpha (0.45f),
                                      centre.x, capBounds.getBottom(), true);
            rim.addColour (0.72, juce::Colours::transparentBlack);

            g.setGradientFill (rim);
            g.fillEllipse (capBounds);
        }

        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawEllipse (capBounds.reduced (0.5f), juce::jmax (1.0f, capRadius * 0.09f));

        // Specular highlight across the upper half.
        {
            const auto highlight = juce::Rectangle<float> (capRadius * 1.35f, capRadius * 0.95f)
                                       .withCentre ({ centre.x, capBounds.getY() + capRadius * 0.52f });

            juce::ColourGradient gloss (juce::Colours::white.withAlpha (0.85f),
                                        highlight.getCentreX(), highlight.getY(),
                                        juce::Colours::white.withAlpha (0.0f),
                                        highlight.getCentreX(), highlight.getBottom(), false);

            g.setGradientFill (gloss);
            g.fillEllipse (highlight);
        }

        // Bounce light along the lower edge.
        {
            const auto bounce = juce::Rectangle<float> (capRadius * 1.15f, capRadius * 0.5f)
                                    .withCentre ({ centre.x, capBounds.getBottom() - capRadius * 0.34f });

            juce::ColourGradient reflection (cap.brighter (0.9f).withAlpha (0.0f),
                                             bounce.getCentreX(), bounce.getY(),
                                             cap.brighter (0.9f).withAlpha (0.55f),
                                             bounce.getCentreX(), bounce.getBottom(), false);

            g.setGradientFill (reflection);
            g.fillEllipse (bounce);
        }

        // -- pointer ----------------------------------------------------------
        // A notch cut into the rim rather than a line across the gloss, which
        // would break the moulded look.
        juce::Path pointer;
        const auto pointerWidth  = juce::jmax (2.0f, capRadius * 0.16f);
        const auto pointerLength = capRadius * 0.52f;

        pointer.addRoundedRectangle (-pointerWidth * 0.5f, -capRadius + capRadius * 0.1f,
                                     pointerWidth, pointerLength, pointerWidth * 0.5f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));

        g.setColour (juce::Colours::black.withAlpha (0.72f));
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

        paintGlassPanel (g, bounds, 3.0f, true);

        if (box.hasKeyboardFocus (false))
        {
            g.setColour (colours::accent);
            g.drawRoundedRectangle (bounds.reduced (0.5f), 3.0f, 1.0f);
        }

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

        paintGlassPanel (g, bounds, 3.0f, true);

        if (on || shouldDrawButtonAsHighlighted)
        {
            g.setColour (colours::accent.withAlpha (on ? 0.30f : 0.12f));
            g.fillRoundedRectangle (bounds, 3.0f);
        }

        g.setColour (on ? colours::accent : juce::Colours::white.withAlpha (0.16f));
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
    }

    void NogLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                           bool shouldDrawButtonAsHighlighted, bool)
    {
        // Toggles are drawn as small indicator pills rather than tick boxes,
        // which suits a synth panel and stays legible at this size.
        const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const auto on     = button.getToggleState();

        paintGlassPanel (g, bounds, 3.0f, true);

        if (on || shouldDrawButtonAsHighlighted)
        {
            g.setColour (colours::accent.withAlpha (on ? 0.34f : 0.12f));
            g.fillRoundedRectangle (bounds, 3.0f);
        }

        g.setColour (on ? colours::accent : juce::Colours::white.withAlpha (0.16f));
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

        g.setColour (on ? juce::Colours::white : colours::dimText);
        g.setFont (uiFont (11.0f, on));
        g.drawText (button.getButtonText(), bounds, juce::Justification::centred, false);
    }
}
