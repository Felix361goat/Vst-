#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace nog::ui
{
    /** The plugin's colour palette, in one place so panels never hard-code a
        colour and a re-skin stays a single-file change. */
    namespace colours
    {
        inline const juce::Colour background   { 0xff101216 };
        inline const juce::Colour panel        { 0xff1a1e24 };
        inline const juce::Colour panelHeader  { 0xff232830 };
        inline const juce::Colour border       { 0xff2a303a };
        inline const juce::Colour text         { 0xffc7cdd6 };
        inline const juce::Colour dimText      { 0xff7a8496 };
        inline const juce::Colour accent       { 0xff5cc8ff };
        inline const juce::Colour modulation   { 0xffff9f45 };
        inline const juce::Colour knobTrack    { 0xff2f3641 };
        inline const juce::Colour meter        { 0xff6ee7a8 };
        inline const juce::Colour meterClip    { 0xffff5f56 };

        /** Control colours. Each module gets one, so a knob's colour says which
            section it belongs to as well as looking the way it does. */
        inline const juce::Colour candyRed     { 0xffd81f1f };
        inline const juce::Colour candyBlue    { 0xff2340e0 };
        inline const juce::Colour candyGreen   { 0xff2bbd2b };
        inline const juce::Colour candyYellow  { 0xfff2c40e };
        inline const juce::Colour candyOrange  { 0xffe8890f };
        inline const juce::Colour candyPurple  { 0xff8f2bc9 };
    }

    /** Slider property carrying a control's colour through to the look and
        feel, so drawing stays decoupled from which module owns the knob. */
    inline constexpr const char* knobColourProperty = "knobColour";

    /** Paints a translucent panel over whatever is behind it.

        Panels are deliberately see-through so the artwork behind the interface
        stays visible. Readability comes from a graded tint that is darkest at
        the top, where the labels are, plus a bright top edge and a soft inner
        shadow that together read as a pane of glass rather than a flat
        rectangle.

        @param highlightTop  a brighter leading edge, for panel headers.
    */
    void paintGlassPanel (juce::Graphics& g, juce::Rectangle<float> bounds,
                          float cornerSize = 6.0f, bool highlightTop = true);

    /**
        Dark theme for the whole plugin.

        The rotary drawing is the interesting part: as well as the usual value
        arc it draws a second, wider arc showing how far modulation can push the
        control. Seeing the modulated range on the knob itself is most of what
        makes a matrix-driven synth navigable.
    */
    class NogLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        NogLookAndFeel();

        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPosProportional, float rotaryStartAngle,
                               float rotaryEndAngle, juce::Slider& slider) override;

        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle style, juce::Slider& slider) override;

        void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                           int buttonX, int buttonY, int buttonW, int buttonH,
                           juce::ComboBox& box) override;

        void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override;

        void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

        void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;
    };
}
