
#pragma once

#include "ImogenCommon.h"

#include <juce_audio_processors/juce_audio_processors.h>



namespace Imogen
{


static inline auto createParameterTree()
{
    using Group = juce::AudioProcessorParameterGroup;
    
    using FloatParameter = bav::FloatParameter;
    using IntParameter   = bav::IntParameter;
    using BoolParameter  = bav::BoolParameter;
    
    namespace l = bav::ParameterValueConversionLambdas;
    
    std::vector<std::unique_ptr<Group>> groups;
    
    const juce::NormalisableRange<float> gainRange      (-60.0f, 0.0f, 0.01f);
    const juce::NormalisableRange<float> zeroToOneRange (0.0f, 1.0f, 0.01f);
    const juce::NormalisableRange<float> msRange        (0.001f, 1.0f, 0.001f);
    const juce::NormalisableRange<float> hzRange        (40.0f, 10000.0f, 1.0f);
    
    constexpr auto generic = juce::AudioProcessorParameter::genericParameter;
    
    const auto dB = TRANS ("dB");
    const auto st = TRANS ("st");
    const auto sec = TRANS ("sec");
    
    // the dividing character that will be used for each AudioParameterGroup
    const auto div = "|";
    
    {   /* MIXING */
        auto inputMode = std::make_unique<IntParameter> (inputSourceID,
                                                         TRANS("Input source"), TRANS("Input source"),
                                                         1, 3, 1, juce::String(),
            [](int value, int maxLength)
            {
                switch (value)
                {
                    case (2): return TRANS("Right").substring(0, maxLength);
                    case (3): return TRANS("Mix to mono").substring(0, maxLength);
                    default:  return TRANS("Left").substring(0, maxLength);
                }
            },
            [](const juce::String& text)
            {
                if (text.containsIgnoreCase (TRANS("Right"))) return 2;
                if (text.containsIgnoreCase (TRANS("mono")) || text.containsIgnoreCase (TRANS("mix"))) return 3;
                return 1;
            });
        
        auto dryWet = std::make_unique<IntParameter> (dryWetID,
                                                      TRANS ("Dry/wet"), TRANS ("Main dry/wet"),
                                                      0, 100, 100, "%", l::pcnt_stringFromInt, l::pcnt_intFromString);
        
        auto inGain = std::make_unique<FloatParameter> (inputGainID,
                                                        TRANS ("In"), TRANS ("Input gain"),
                                                        gainRange, 0.0f, dB, juce::AudioProcessorParameter::inputGain,
                                                        l::gain_stringFromFloat, l::gain_floatFromString);
        
        auto outGain = std::make_unique<FloatParameter> (outputGainID,
                                                         TRANS ("Out"), TRANS ("Output gain"),
                                                         gainRange, -4.0f, dB, juce::AudioProcessorParameter::outputGain,
                                                         l::gain_stringFromFloat, l::gain_floatFromString);
        //  subgroup: bypasses
        auto mainBypass    = std::make_unique<BoolParameter> (mainBypassID,
                                                              TRANS (""), TRANS (""),
                                                              false, juce::String(),
                                                              l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto leadBypass    = std::make_unique<BoolParameter> (leadBypassID,
                                                              TRANS (""), TRANS (""),
                                                              false, juce::String(),
                                                              l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto harmonyBypass = std::make_unique<BoolParameter> (harmonyBypassID,
                                                              TRANS (""), TRANS (""),
                                                              false, juce::String(),
                                                              l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto bypasses = std::make_unique<Group> ("Bypasses", TRANS ("Bypasses"), div,
                                                 std::move (mainBypass), std::move (leadBypass), std::move (harmonyBypass));
        //  subgroup: stereo image
        auto stereo_width   = std::make_unique<IntParameter> (stereoWidthID,
                                                              TRANS (""), TRANS (""),
                                                              0, 100, 100, "%", l::pcnt_stringFromInt, l::pcnt_intFromString);
        
        auto stereo_lowest  = std::make_unique<IntParameter> (lowestPannedID, TRANS (""), TRANS (""),
                                                              0, 127, 0, juce::String(), l::pitch_stringFromInt, l::pitch_intFromString);
        
        auto stereo_leadPan = std::make_unique<IntParameter> (dryPanID, TRANS (""), TRANS (""),
                                                              0, 127, 64, juce::String(), l::midiPan_stringFromInt, l::midiPan_intFromString);
        
        auto stereo = std::make_unique<Group> ("Stereo image", TRANS ("Stereo image"), div,
                                               std::move (stereo_width), std::move (stereo_lowest), std::move (stereo_leadPan));
        
        groups.emplace_back (std::make_unique<Group> ("Mixing", TRANS ("Mixing"), div,
                                                      std::move (inputMode), std::move (dryWet), std::move (inGain), std::move (outGain),
                                                      std::move (bypasses), std::move (stereo)));
    }
    {   /* MIDI */
        auto pitchbendRange   = std::make_unique<IntParameter>  (pitchBendRangeID,
                                                                 TRANS (""), TRANS (""),
                                                                 0, 12, 2, st, l::st_stringFromInt, l::st_intFromString);
        
        auto velocitySens     = std::make_unique<IntParameter>  (velocitySensID,
                                                                 TRANS (""), TRANS (""),
                                                                 0, 100, 100, "%", l::pcnt_stringFromInt, l::pcnt_intFromString);
        
        auto aftertouchToggle = std::make_unique<BoolParameter> (aftertouchGainToggleID,
                                                                 TRANS (""), TRANS (""),
                                                                 true, juce::String(), l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto voiceStealing    = std::make_unique<BoolParameter> (voiceStealingID,
                                                                 TRANS (""), TRANS (""),
                                                                 false, juce::String(), l::toggle_stringFromBool, l::toggle_boolFromString);
        //  subgroup: pedal pitch
        auto pedal_toggle   = std::make_unique<BoolParameter> (pedalPitchIsOnID,
                                                               TRANS (""), TRANS (""),
                                                               false, juce::String(), l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto pedal_thresh   = std::make_unique<IntParameter>  (pedalPitchThreshID,
                                                               TRANS (""), TRANS (""),
                                                               0, 127, 0, juce::String(), l::pitch_stringFromInt, l::pitch_intFromString);
        
        auto pedal_interval = std::make_unique<IntParameter>  (pedalPitchIntervalID,
                                                               TRANS (""), TRANS (""),
                                                               1, 12, 12, st, l::st_stringFromInt, l::st_intFromString);
        
        auto pedal = std::make_unique<Group> ("Pedal pitch", TRANS ("Pedal pitch"), div,
                                              std::move (pedal_toggle), std::move (pedal_thresh), std::move (pedal_interval));
        
        //  subgroup: descant
        auto descant_toggle   = std::make_unique<BoolParameter> (descantIsOnID,
                                                                 TRANS (""), TRANS (""),
                                                                 false, juce::String(), l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto descant_thresh   = std::make_unique<IntParameter>  (descantThreshID,
                                                                 TRANS (""), TRANS (""),
                                                                 0, 127, 127, juce::String(), l::pcnt_stringFromInt, l::pitch_intFromString);
        
        auto descant_interval = std::make_unique<IntParameter>  (descantIntervalID,
                                                                 TRANS (""), TRANS (""),
                                                                 1, 12, 12, st, l::st_stringFromInt, l::st_intFromString);
        
        auto descant = std::make_unique<Group> ("Descant", TRANS ("Descant"), div,
                                                std::move (descant_toggle), std::move (descant_thresh), std::move (descant_interval));
        
        groups.emplace_back (std::make_unique<Group> ("MIDI", TRANS ("MIDI"), div,
                                                      std::move (pitchbendRange), std::move (velocitySens), std::move (aftertouchToggle),
                                                      std::move (voiceStealing), std::move (pedal), std::move (descant)));
    }
    {   /* ADSR */
        auto attack  = std::make_unique<FloatParameter> (adsrAttackID,
                                                         TRANS (""), TRANS (""),
                                                         msRange, 0.35f, sec, generic, l::sec_stringFromFloat, l::sec_floatFromString);
        
        auto decay   = std::make_unique<FloatParameter> (adsrDecayID,
                                                         TRANS (""), TRANS (""),
                                                         msRange, 0.06f, sec, generic, l::sec_stringFromFloat, l::sec_floatFromString);
        
        auto sustain = std::make_unique<FloatParameter> (adsrSustainID,
                                                         TRANS (""), TRANS (""),
                                                         zeroToOneRange, 0.8f, "%", generic, l::normPcnt_stringFromInt, l::normPcnt_intFromString);
        
        auto release = std::make_unique<FloatParameter> (adsrReleaseID,
                                                         TRANS (""), TRANS (""),
                                                         msRange, 0.1f, sec, generic, l::sec_stringFromFloat, l::sec_floatFromString);
        
        groups.emplace_back (std::make_unique<Group> ("ADSR", TRANS ("ADSR"), div,
                                                      std::move (attack), std::move (decay), std::move (sustain), std::move (release)));
    }
    {    /* EFFECTS */
        //  subgroup: noise gate
        auto gate_toggle = std::make_unique<BoolParameter>  (noiseGateToggleID,
                                                             TRANS (""), TRANS (""),
                                                             true, juce::String(), l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto gate_thresh = std::make_unique<FloatParameter> (noiseGateThresholdID,
                                                             TRANS (""), TRANS (""),
                                                             gainRange, -20.0f, dB, generic, l::gain_stringFromFloat, l::gain_floatFromString);
        
        auto gate = std::make_unique<Group> ("Noise gate", TRANS ("Noise gate"), div, std::move (gate_toggle), std::move (gate_thresh));
        
        //  subgroup: de-esser
        auto ess_toggle = std::make_unique<BoolParameter>  (deEsserToggleID,
                                                            TRANS (""), TRANS (""),
                                                            true, juce::String(), l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto ess_thresh = std::make_unique<FloatParameter> (deEsserThreshID,
                                                            TRANS (""), TRANS (""),
                                                            gainRange, -6.0f, dB, generic, l::gain_stringFromFloat, l::gain_floatFromString);
        
        auto ess_amount = std::make_unique<FloatParameter> (deEsserAmountID,
                                                            TRANS (""), TRANS (""),
                                                            zeroToOneRange, 0.5f, dB, generic, l::normPcnt_stringFromInt, l::normPcnt_intFromString);
        
        auto deEss = std::make_unique<Group> ("De-esser", TRANS ("De-esser"), div,
                                              std::move (ess_toggle), std::move (ess_thresh), std::move (ess_amount));
        //  subgroup: compressor
        auto comp_toggle = std::make_unique<BoolParameter>  (compressorToggleID,
                                                             TRANS (""), TRANS (""),
                                                             false, juce::String(), l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto comp_amount = std::make_unique<FloatParameter> (compressorAmountID,
                                                             TRANS (""), TRANS (""),
                                                             zeroToOneRange, 0.35f, dB, generic,
                                                             l::normPcnt_stringFromInt, l::normPcnt_intFromString);
        
        auto compressor = std::make_unique<Group> ("Compressor", TRANS ("Compressor"), div, std::move (comp_toggle), std::move (comp_amount));
        
        //  subgroup: delay
        auto delay_toggle = std::make_unique<BoolParameter> (delayToggleID,
                                                             TRANS (""), TRANS (""),
                                                             false, juce::String(), l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto delay_mix    = std::make_unique<IntParameter>  (delayDryWetID,
                                                             TRANS (""), TRANS (""),
                                                             0, 100, 35, "%", l::pcnt_stringFromInt, l::pcnt_intFromString);
        
        auto delay = std::make_unique<Group> ("Delay", TRANS ("Delay"), div, std::move (delay_toggle), std::move (delay_mix));
        
        //  subgroup: reverb
        auto verb_toggle = std::make_unique<BoolParameter>  (reverbToggleID,
                                                             TRANS (""), TRANS (""),
                                                             false, juce::String(), l::toggle_stringFromBool, l::toggle_boolFromString);
        
        auto verb_dryWet = std::make_unique<IntParameter>   (reverbDryWetID,
                                                             TRANS (""), TRANS (""),
                                                             0, 100, 35, "%", l::pcnt_stringFromInt, l::pcnt_intFromString);
        
        auto verb_decay  = std::make_unique<FloatParameter> (reverbDecayID,
                                                             TRANS (""), TRANS (""),
                                                             zeroToOneRange, 0.6f, "%", generic,
                                                             l::normPcnt_stringFromInt, l::normPcnt_intFromString);
        
        auto verb_duck   = std::make_unique<FloatParameter> (reverbDuckID,
                                                             TRANS (""), TRANS (""),
                                                             zeroToOneRange, 0.3f, "%", generic,
                                                             l::normPcnt_stringFromInt, l::normPcnt_intFromString);
        
        auto verb_loCut  = std::make_unique<FloatParameter> (reverbLoCutID,
                                                             TRANS (""), TRANS (""),
                                                             hzRange, 80.0f, TRANS ("Hz"), generic, l::hz_stringFromFloat, l::hz_floatFromString);
        
        auto verb_hiCut  = std::make_unique<FloatParameter> (reverbHiCutID,
                                                             TRANS (""), TRANS (""),
                                                             hzRange, 5500.0f, TRANS ("Hz"), generic, l::hz_stringFromFloat, l::hz_floatFromString);
        
        auto reverb = std::make_unique<Group> ("Reverb", TRANS ("Reverb"), div,
                                               std::move (verb_toggle), std::move (verb_dryWet), std::move (verb_decay),
                                               std::move (verb_duck), std::move (verb_loCut), std::move (verb_hiCut));
        //  limiter
        auto limiter = std::make_unique<Group> ("Limiter", TRANS ("Limiter"), div,
                                                std::make_unique<BoolParameter>  (limiterToggleID,
                                                                                  TRANS (""), TRANS (""),
                                                                                  true, juce::String(),
                                                                                  l::toggle_stringFromBool, l::toggle_boolFromString));
        
        groups.emplace_back (std::make_unique<Group> ("Effects", TRANS ("Effects"), div,
                                                      std::move (gate),  std::move (deEss), std::move (compressor),
                                                      std::move (delay), std::move (reverb), std::move (limiter)));
    }
    
    auto mainGroup = std::make_unique<Group> ("ImogenParameters", TRANS ("Imogen Parameters"), div);
    
    for (auto& group : groups)
        mainGroup->addChild (std::move (group));
    
    return mainGroup;
}


}  // namespace