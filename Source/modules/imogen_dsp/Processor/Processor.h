
#pragma once

#include <imogen_dsp/Engine/Engine.h>

namespace Imogen
{
class Processor : public plugin::Processor< State, Engine >
{
public:
    Processor();

private:
    bool canAddBus (bool isInput) const override final { return isInput; }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const final;

    double getTailLengthSeconds() const final;

    bool acceptsMidi() const final { return true; }
    bool producesMidi() const final { return true; }
    bool supportsMPE() const final { return false; }
    bool isMidiEffect() const final { return false; }

    const String      getName() const final { return "Imogen"; }
    juce::StringArray getAlternateDisplayNames() const final { return {"Imgn"}; }

    Parameters& parameters {state.parameters};

    // network::OscDataSynchronizer dataSync {state};
};

}  // namespace Imogen
