/*==========================================================================================================================
           _             _   _                _                _                 _               _
          /\ \          /\_\/\_\ _           /\ \             /\ \              /\ \            /\ \     _
          \ \ \        / / / / //\_\        /  \ \           /  \ \            /  \ \          /  \ \   /\_\
          /\ \_\      /\ \/ \ \/ / /       / /\ \ \         / /\ \_\          / /\ \ \        / /\ \ \_/ / /
         / /\/_/     /  \____\__/ /       / / /\ \ \       / / /\/_/         / / /\ \_\      / / /\ \___/ /
        / / /       / /\/________/       / / /  \ \_\     / / / ______      / /_/_ \/_/     / / /  \/____/
       / / /       / / /\/_// / /       / / /   / / /    / / / /\_____\    / /____/\       / / /    / / /
      / / /       / / /    / / /       / / /   / / /    / / /  \/____ /   / /\____\/      / / /    / / /
  ___/ / /__     / / /    / / /       / / /___/ / /    / / /_____/ / /   / / /______     / / /    / / /
 /\__\/_/___\    \/_/    / / /       / / /____\/ /    / / /______\/ /   / / /_______\   / / /    / / /
 \/_________/            \/_/        \/_________/     \/___________/    \/__________/   \/_/     \/_/
 
 
 This file is part of the Imogen codebase.
 
 @2021 by Ben Vining. All rights reserved.
 
 ImogenCommon.h: This file defines some common constants that the various modules of Imogen all refer to.
 
==========================================================================================================================*/


#pragma once

#ifndef IMOGEN_REMOTE_APP
  #define IMOGEN_REMOTE_APP 0
#endif


namespace Imogen
{


enum parameterID
{
    inputSourceID,
    mainBypassID,
    leadBypassID,
    harmonyBypassID,
    dryPanID,
    dryWetID,
    adsrAttackID,
    adsrDecayID,
    adsrSustainID,
    adsrReleaseID,
    stereoWidthID,
    lowestPannedID,
    velocitySensID,
    pitchBendRangeID,
    pedalPitchIsOnID,
    pedalPitchThreshID,
    pedalPitchIntervalID,
    descantIsOnID,
    descantThreshID,
    descantIntervalID,
    voiceStealingID,
    inputGainID,
    outputGainID,
    limiterToggleID,
    noiseGateToggleID,
    noiseGateThresholdID,
    compressorToggleID,
    compressorAmountID,
    aftertouchGainToggleID,
    deEsserToggleID,
    deEsserThreshID,
    deEsserAmountID,
    reverbToggleID,
    reverbDryWetID,
    reverbDecayID,
    reverbDuckID,
    reverbLoCutID,
    reverbHiCutID
};
static constexpr int numParams = reverbHiCutID + 1;


enum eventID
{
    killAllMidi,
    midiLatch,
    pitchBendFromEditor
};
static constexpr int num_nonParamEventTypes = pitchBendFromEditor + 1;


}  // namespace
