
/*======================================================================================================================================================
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
 
 PluginProcessor.cpp: This file contains the guts of Imogen's AudioProcessor code.
 
======================================================================================================================================================*/


#include "GUI/Holders/Plugin_Editor/PluginEditor.h"
#include "PluginProcessor.h"

#include "PluginProcessorParameters.cpp"
#include "PluginProcessorState.cpp"
#include "PluginProcessorNetworking.cpp"


ImogenAudioProcessor::ImogenAudioProcessor():
    AudioProcessor(makeBusProperties()),
    tree(*this, nullptr, "IMOGEN_PARAMETERS", createParameters()), 
    oscMapper(false),
    abletonLink(120.0) // constructed with the initial BPM
{
#if BV_USE_NE10
    ne10_init();  // if you use the Ne10 library, you must initialize it in your constructor like this!
#endif
    
    jassert (AudioProcessor::getParameters().size() == numParams);
    initializeParameterPointers();
    initializeParameterListeners();
    updateParameterDefaults();
           
    initializeParameterOscMappings();
    
    if (isUsingDoublePrecision())
        initialize (doubleEngine);
    else
        initialize (floatEngine);
    
    rescanPresetsFolder();
    
    mts_wasConnected.store (isConnectedToMtsEsp());
    mts_lastScaleName = getScaleName();
    
    lastPresetName = getActivePresetName();
    
    abletonLink_wasEnabled.store (isAbletonLinkEnabled());
    
    constexpr int timerFramerate = 30;
    Timer::startTimerHz (timerFramerate);
}

ImogenAudioProcessor::~ImogenAudioProcessor()
{
    Timer::stopTimer();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ImogenAudioProcessor::timerCallback()
{
    auto* const activeEditor = dynamic_cast<ImogenGuiHolder*> (getActiveEditor());
    
    if (parameterDefaultsAreDirty.load())
    {
        if (activeEditor != nullptr)
            activeEditor->parameterDefaultsUpdated();
        
        parameterDefaultsAreDirty.store (false);
    }
    
    const auto mts_isConnected = isConnectedToMtsEsp();
    if (mts_isConnected != mts_wasConnected.load())
    {
        if (activeEditor != nullptr)
            activeEditor->mts_connectionChange (mts_isConnected);
        
        mts_wasConnected.store (mts_isConnected);
    }
    
    const auto scaleName = getScaleName();
    if (scaleName != mts_lastScaleName)
    {
        if (activeEditor != nullptr)
            activeEditor->mts_scaleChange (scaleName);
        
        mts_lastScaleName = scaleName;
    }
    
    const auto presetName = getActivePresetName();
    if (presetName != lastPresetName)
    {
        if (activeEditor != nullptr)
            activeEditor->presetNameChange (presetName);
        
        lastPresetName = presetName;
    }
    
    const auto abletonLink_isEnabled = isAbletonLinkEnabled();
    if (abletonLink_isEnabled != abletonLink_wasEnabled.load())
    {
        if (activeEditor != nullptr)
            activeEditor->abletonLinkChange (abletonLink_isEnabled);
        
        abletonLink_wasEnabled.store (abletonLink_isEnabled);
    }
    
    /* send editor state data...
     - latest intonation (pitch as string, + num cents sharp/flat)
     - levels for all level / gain reduction meters
    */
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SampleType>
inline void ImogenAudioProcessor::initialize (bav::ImogenEngine<SampleType>& activeEngine)
{
    auto initSamplerate = getSampleRate();
    if (initSamplerate <= 0.0) initSamplerate = 44100.0;
    
    auto initBlockSize = getBlockSize();
    if (initBlockSize <= 0) initBlockSize = 512;
    
    activeEngine.initialize (initSamplerate, initBlockSize);
    
    updateAllParameters (activeEngine);
    
    setLatencySamples (activeEngine.reportLatency());
}


void ImogenAudioProcessor::prepareToPlay (const double sampleRate, const int samplesPerBlock)
{
    if (isUsingDoublePrecision())
        prepareToPlayWrapped (sampleRate, doubleEngine, floatEngine);
    else
        prepareToPlayWrapped (sampleRate, floatEngine,  doubleEngine);
    
    currentMessages.ensureStorageAllocated (samplesPerBlock);
}


template <typename SampleType1, typename SampleType2>
inline void ImogenAudioProcessor::prepareToPlayWrapped (const double sampleRate,
                                                        bav::ImogenEngine<SampleType1>& activeEngine,
                                                        bav::ImogenEngine<SampleType2>& idleEngine)
{
    if (! idleEngine.hasBeenReleased())
        idleEngine.releaseResources();
    
    updateAllParameters (activeEngine);
    
    jassert (activeEngine.getLatency() > 0);
    
    activeEngine.prepare (sampleRate);
    
    setLatencySamples (activeEngine.reportLatency());
}


void ImogenAudioProcessor::releaseResources()
{
    if (! doubleEngine.hasBeenReleased())
        doubleEngine.releaseResources();
    
    if (! floatEngine.hasBeenReleased())
        floatEngine.releaseResources();
}


void ImogenAudioProcessor::reset()
{
    if (isUsingDoublePrecision())
        doubleEngine.reset();
    else
        floatEngine.reset();
}


void ImogenAudioProcessor::editorPitchbend (int wheelValue)
{
    nonParamEvents.pushMessage (pitchBendFromEditor, pitchbendNormalizedRange.convertTo0to1(float(wheelValue)));
}


/*
 These four functions represent the top-level callbacks made by the host during audio processing. Audio samples may be sent to us as float or double values; both of these functions redirect to the templated processBlockWrapped() function below.
 The buffers sent to this function by the host may be variable in size, so I have coded defensively around several edge cases & possible buggy host behavior and created several layers of checks that each callback passes through before individual chunks of audio are actually rendered.
*/

void ImogenAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockWrapped (buffer, midiMessages, floatEngine, mainBypass->get());
}


void ImogenAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockWrapped (buffer, midiMessages, doubleEngine, mainBypass->get());
}


void ImogenAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (! mainBypass->get())
        mainBypass->setValueNotifyingHost (1.0f);
    
    processBlockWrapped (buffer, midiMessages, floatEngine, true);
}


void ImogenAudioProcessor::processBlockBypassed (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    if (! mainBypass->get())
        mainBypass->setValueNotifyingHost (1.0f);
    
    processBlockWrapped (buffer, midiMessages, doubleEngine, true);
}


template <typename SampleType>
inline void ImogenAudioProcessor::processBlockWrapped (juce::AudioBuffer<SampleType>& buffer,
                                                       juce::MidiBuffer& midiMessages,
                                                       bav::ImogenEngine<SampleType>& engine,
                                                       const bool isBypassedThisCallback)
{
    jassert (! engine.hasBeenReleased() && engine.hasBeenInitialized());
    
    juce::ScopedNoDenormals nodenorms;
    
    updateAllParameters (engine);
    //processQueuedParameterChanges (engine);
    //processQueuedNonParamEvents (engine);

    if (buffer.getNumSamples() == 0 || buffer.getNumChannels() == 0)
        return;
           
    // program change messages need to be handled at this top level...
    std::for_each (midiMessages.cbegin(), midiMessages.cend(),
                   [&] (const juce::MidiMessageMetadata& meta)
                   {
                       const auto msg = meta.getMessage();
        
                       if (msg.isProgramChange())
                           setCurrentProgram (msg.getProgramChangeNumber());
                   });
    
    juce::AudioBuffer<SampleType> inBus  = getBusBuffer (buffer, true, getBusesLayout().getMainInputChannelSet() == juce::AudioChannelSet::disabled());
    juce::AudioBuffer<SampleType> outBus = getBusBuffer (buffer, false, 0);
    
    engine.process (inBus, outBus, midiMessages, isBypassedThisCallback);
}


/*===========================================================================================================================
 ============================================================================================================================*/


// standard and general-purpose functions -----------------------------------------------------------------------------------------------------------

double ImogenAudioProcessor::getTailLengthSeconds() const
{
    return double(adsrRelease->get());
}


inline juce::AudioProcessor::BusesProperties ImogenAudioProcessor::makeBusProperties() const
{
    auto stereo = juce::AudioChannelSet::stereo();
    auto mono   = juce::AudioChannelSet::mono();

    return BusesProperties().withInput ("Input",  stereo, true)
                            .withInput ("Sidechain", mono, false)
                            .withOutput("Output", stereo, true);
}


bool ImogenAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto disabled = juce::AudioChannelSet::disabled();
    
    if (layouts.getMainInputChannelSet() == disabled && layouts.getChannelSet(true, 1) == disabled)
        return false;
    
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}


juce::AudioProcessorEditor* ImogenAudioProcessor::createEditor()
{
    return new ImogenAudioProcessorEditor(*this);
}


// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ImogenAudioProcessor();
}
