#pragma once

#include <bv_plugin_gui/bv_plugin_gui.h>
#include <imogen_state/imogen_state.h>

#include <imogen_gui/Header/Header.h>
#include <imogen_gui/CenterDial/CenterDial.h>
#include <imogen_gui/MidiKeyboard/MidiKeyboard.h>
#include <imogen_gui/DryWet/DryWet.h>

namespace Imogen
{
class GUI : public juce::Component
{
public:
    GUI (State& stateToUse);

    virtual ~GUI() override;

private:
    void paint (juce::Graphics& g) final;
    void resized() final;
    bool keyPressed (const juce::KeyPress& key) final;

    State&      state;
    Parameters& parameters {state.parameters};
    Internals&  internals {state.internals};

    Header       header {state};
    CenterDial   dial {state};
    DryWet       dryWet {state};
    MidiKeyboard keyboard;

    juce::TooltipWindow tooltipWindow {this, 700};
};

}  // namespace Imogen
