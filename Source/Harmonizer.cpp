/*
  ==============================================================================

    Harmonizer.cpp
    Created: 13 Dec 2020 7:53:39pm
    Author:  Ben Vining

  ==============================================================================
*/

#include "Harmonizer.h"


HarmonizerVoice::HarmonizerVoice(): adsrIsOn(true), currentlyPlayingNote(-1), currentOutputFreq(-1.0f), currentVelocityMultiplier(0.0f), pitchbendRangeUp(2), pitchbendRangeDown(2), lastRecievedPitchbend(64), lastRecievedVelocity(0), currentSampleRate(44100.0), noteOnTime(0), keyIsDown(false), sustainPedalDown(false), sostenutoPedalDown(false), midiVelocitySensitivity(100)
{ };


HarmonizerVoice::~HarmonizerVoice()
{ };


void HarmonizerVoice::renderNextBlock(AudioBuffer<float> outputBuffer, const int startSample, const int numSamples)
{
	if(! (sustainPedalDown || sostenutoPedalDown))
	{
		if(! keyIsDown)
			stopNote(1.0f, false);
	}
	
	if(adsr.isActive())
	{
		AudioBuffer<float> subBuffer(outputBuffer.getArrayOfWritePointers(), outputBuffer.getNumChannels(), startSample, numSamples);
		
		tempBuffer.makeCopyOf(subBuffer, true);
		esola(tempBuffer, 0, numSamples);
		subBuffer.makeCopyOf(tempBuffer, true);
		
		if(adsrIsOn)
			adsr.applyEnvelopeToBuffer(subBuffer, startSample, numSamples);
	}
	else
	{
		clearCurrentNote();
	}
};


void HarmonizerVoice::esola(AudioBuffer<float>& outputBuffer, const int startSample, const int numSamples)
{
	
};


// MIDI -----------------------------------------------------------------------------------------------------------

float HarmonizerVoice::getOutputFreqFromMidinoteAndPitchbend(const int lastRecievedNote, const int pitchBend)
{
	jassert(lastRecievedNote >= 0);
	
	if(pitchBend == 64)
	{
		return benutils::mtof(lastRecievedNote);
	}
	else if(pitchBend > 64)
	{
		return benutils::mtof(((pitchbendRangeUp * (pitchBend - 65)) / 62) + lastRecievedNote);
	}
	else
	{
		return benutils::mtof((((1 - pitchbendRangeDown) * pitchBend) / 63) + lastRecievedNote - pitchbendRangeDown);
	}
	
};

void HarmonizerVoice::setMidiVelocitySensitivity(const int newsensitity)
{
	midiVelocitySensitivity = newsensitity;
	if(currentlyPlayingNote >= 0)
		currentVelocityMultiplier = calcVelocityMultiplier(lastRecievedVelocity);
};

float HarmonizerVoice::calcVelocityMultiplier(const int inputVelocity)
{
	const float initialMutiplier = inputVelocity / 127.0f;
	return ((1.0f - initialMutiplier) * (1.0f - midiVelocitySensitivity) + initialMutiplier);
};

void HarmonizerVoice::startNote(const int midiPitch, const float velocity, const int currentPitchWheelPosition)
{
	adsr.noteOn();
	currentlyPlayingNote = midiPitch;
	lastRecievedPitchbend = currentPitchWheelPosition;
	currentOutputFreq = getOutputFreqFromMidinoteAndPitchbend(midiPitch, currentPitchWheelPosition);
	currentVelocityMultiplier = calcVelocityMultiplier(velocity);
	lastRecievedVelocity = velocity;
};

void HarmonizerVoice::stopNote(const float velocity, const bool allowTailOff)
{
	if (allowTailOff)
	{
		adsr.noteOff();
	}
	else
	{
		clearCurrentNote();
		adsr.reset();
	}
	lastRecievedVelocity = 0.0f;
};

void HarmonizerVoice::pitchWheelMoved(const int newPitchWheelValue)
{
	lastRecievedPitchbend = newPitchWheelValue;
	if(currentlyPlayingNote >= 0)
		currentOutputFreq = getOutputFreqFromMidinoteAndPitchbend(currentlyPlayingNote, newPitchWheelValue);
};

void HarmonizerVoice::aftertouchChanged(const int) { };

void HarmonizerVoice::channelPressureChanged(const int) { };

void HarmonizerVoice::controllerMoved(const int controllerNumber, const int newControllerValue) { };


// ADSR settings -------------------------------------------------------------------------------------------------------

void HarmonizerVoice::updateAdsrSettings(const float attack, const float decay, const float sustain, const float release)
{
	adsrParams.attack = attack;
	adsrParams.decay = decay;
	adsrParams.sustain = sustain;
	adsrParams.release = release;
	adsr.setParameters(adsrParams);
};


void HarmonizerVoice::updatePitchbendSettings(const int rangeUp, const int rangeDown)
{
	pitchbendRangeUp = rangeUp;
	pitchbendRangeDown = rangeDown;
	if(currentlyPlayingNote >= 0)
		currentOutputFreq = getOutputFreqFromMidinoteAndPitchbend(currentlyPlayingNote, lastRecievedPitchbend);
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


Harmonizer::Harmonizer(): lastPitchWheelValue(0), sampleRate(44100.0), shouldStealNotes(true), lastNoteOnCounter(0), minimumSubBlockSize(32), subBlockSubdivisionIsStrict(false)
{
	currentlyActiveNotes.ensureStorageAllocated(NUMBER_OF_VOICES);
	currentlyActiveNotes.clearQuick();
	currentlyActiveNotes.add(-1);
};


Harmonizer::~Harmonizer()
{
	voices.clear();
};


// audio rendering-----------------------------------------------------------------------------------------------------------------------------------

void Harmonizer::renderNextBlock(AudioBuffer<float>& outputAudio, const MidiBuffer& inputMidi, int startSample, int numSamples)
{
	jassert (sampleRate != 0);
	
	auto midiIterator = inputMidi.findNextSamplePosition(startSample);
	
	bool firstEvent = true;
	
	const ScopedLock sl (lock);
	
	for (; numSamples > 0; ++midiIterator)
	{
		if (midiIterator == inputMidi.cend())
		{
			renderVoices(outputAudio, startSample, numSamples);
			return;
		}
		
		const auto metadata = *midiIterator;
		const int samplesToNextMidiMessage = metadata.samplePosition - startSample;
		
		if (samplesToNextMidiMessage >= numSamples)
		{
			renderVoices(outputAudio, startSample, numSamples);
			handleMidiEvent(metadata.getMessage());
			break;
		}
		
		if (samplesToNextMidiMessage < ((firstEvent && ! subBlockSubdivisionIsStrict) ? 1 : minimumSubBlockSize))
		{
			handleMidiEvent(metadata.getMessage());
			continue;
		}
		
		firstEvent = false;
		
		renderVoices(outputAudio, startSample, samplesToNextMidiMessage);
		
		handleMidiEvent(metadata.getMessage());
		startSample += samplesToNextMidiMessage;
		numSamples  -= samplesToNextMidiMessage;
	}
	
	std::for_each (midiIterator,
				   inputMidi.cend(),
				   [&] (const MidiMessageMetadata& meta) { handleMidiEvent (meta.getMessage()); });
	
};

void Harmonizer::renderVoices (AudioBuffer<float>& outputAudio, const int startSample, const int numSamples)
{
	for (auto* voice : voices)
		voice->renderNextBlock (outputAudio, startSample, numSamples);
};

void Harmonizer::setCurrentPlaybackSampleRate(const double newRate)
{
	if (sampleRate != newRate)
	{
		const ScopedLock sl (lock);
		allNotesOff (false);
		sampleRate = newRate;
		
		for (auto* voice : voices)
			voice->setCurrentPlaybackSamplerate (newRate);
	}
};

void Harmonizer::setMinimumRenderingSubdivisionSize (const int numSamples, const bool shouldBeStrict) noexcept
{
	jassert (numSamples > 0); // it wouldn't make much sense for this to be less than 1
	minimumSubBlockSize = numSamples;
	subBlockSubdivisionIsStrict = shouldBeStrict;
};


// MIDI events---------------------------------------------------------------------------------------------------------------------------------------

void Harmonizer::handleMidiEvent(const MidiMessage& m)
{
	if (m.isNoteOn())
	{
		noteOn (m.getNoteNumber(), m.getFloatVelocity());
	}
	else if (m.isNoteOff())
	{
		noteOff (m.getNoteNumber(), m.getFloatVelocity(), true);
	}
	else if (m.isAllNotesOff() || m.isAllSoundOff())
	{
		allNotesOff (true);
	}
	else if (m.isPitchWheel())
	{
		const int wheelPos = m.getPitchWheelValue();
		lastPitchWheelValue = wheelPos;
		handlePitchWheel (wheelPos);
	}
	else if (m.isAftertouch())
	{
		handleAftertouch (m.getNoteNumber(), m.getAfterTouchValue());
	}
	else if (m.isChannelPressure())
	{
		handleChannelPressure (m.getChannelPressureValue());
	}
	else if (m.isController())
	{
		handleController (m.getControllerNumber(), m.getControllerValue());
	}
};

void Harmonizer::updateMidiVelocitySensitivity(const int newSensitivity)
{
	const ScopedLock sl (lock);
	for(auto* voice : voices)
		voice->setMidiVelocitySensitivity(newSensitivity);
}

Array<int> Harmonizer::reportActiveNotes() const
{
	const ScopedLock sl (lock);
	
	currentlyActiveNotes.clearQuick();
	
	for (auto* voice : voices)
	{
		if (voice->isVoiceActive())
			currentlyActiveNotes.add(voice->getCurrentlyPlayingNote());
	}
	
	if(! currentlyActiveNotes.isEmpty()) { currentlyActiveNotes.sort(); }
	else { currentlyActiveNotes.add(-1); }

	return currentlyActiveNotes;
};

void Harmonizer::noteOn(const int midiPitch, const float velocity)
{
	const ScopedLock sl (lock);
	
	// If hitting a note that's still ringing, stop it first (it could still be playing because of the sustain or sostenuto pedal).
	for (auto* voice : voices)
	{
		if (voice->getCurrentlyPlayingNote() == midiPitch) { stopVoice (voice, 1.0f, true); }
	}
	
	startVoice(findFreeVoice (midiPitch, shouldStealNotes), midiPitch, velocity);
	
};

void Harmonizer::startVoice(HarmonizerVoice* voice, const int midiPitch, const float velocity)
{
	if(voice != nullptr)
	{
		voice->currentlyPlayingNote = midiPitch;
		voice->noteOnTime = ++lastNoteOnCounter;
		voice->setKeyDown (true);
		voice->setSostenutoPedalDown (false);
		
		voice->startNote (midiPitch, velocity, lastPitchWheelValue);
	}
};

void Harmonizer::noteOff (const int midiNoteNumber, const float velocity, const bool allowTailOff)
{
	const ScopedLock sl (lock);
	
	for (auto* voice : voices)
	{
		if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
		{
			voice->setKeyDown (false);
			if (! (voice->isSustainPedalDown() || voice->isSostenutoPedalDown()))
				stopVoice (voice, velocity, allowTailOff);
		}
	}
};

void Harmonizer::stopVoice (HarmonizerVoice* voice, const float velocity, const bool allowTailOff)
{
	if(voice != nullptr)
	{
		voice->stopNote (velocity, allowTailOff);
	}
};

void Harmonizer::allNotesOff(const bool allowTailOff)
{
	const ScopedLock sl (lock);
	
	for (auto* voice : voices)
		voice->stopNote (1.0f, allowTailOff);
};

void Harmonizer::handlePitchWheel(const int wheelValue)
{
	const ScopedLock sl (lock);
	
	for (auto* voice : voices)
		voice->pitchWheelMoved (wheelValue);
};

void Harmonizer::updatePitchbendSettings(const int rangeUp, const int rangeDown)
{
	const ScopedLock sl (lock);
	for(auto* voice : voices)
		voice->updatePitchbendSettings(rangeUp, rangeDown);
};

void Harmonizer::handleAftertouch(const int midiNoteNumber, const int aftertouchValue)
{
	const ScopedLock sl (lock);
	
	for (auto* voice : voices)
		if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
			voice->aftertouchChanged (aftertouchValue);
};

void Harmonizer::handleChannelPressure(const int channelPressureValue)
{
	const ScopedLock sl (lock);
	
	for (auto* voice : voices)
		voice->channelPressureChanged (channelPressureValue);
};

void Harmonizer::handleController(const int controllerNumber, const int controllerValue)
{
	switch (controllerNumber)
	{
		case 0x40:  handleSustainPedal   (controllerValue >= 64); return;
		case 0x42:  handleSostenutoPedal (controllerValue >= 64); return;
		case 0x43:  handleSoftPedal      (controllerValue >= 64); return;
		default:    break;
	}
	
	const ScopedLock sl (lock);
	
	for (auto* voice : voices)
		voice->controllerMoved (controllerNumber, controllerValue);
};

void Harmonizer::handleSustainPedal(const bool isDown)
{
	const ScopedLock sl (lock);
	
	if (isDown)
	{
		for (auto* voice : voices)
			voice->setSustainPedalDown (true);
	}
	else
	{
		for (auto* voice : voices)
		{
			voice->setSustainPedalDown (false);
			
			if (! (voice->isKeyDown() || voice->isSostenutoPedalDown()))
				stopVoice (voice, 1.0f, true);
		}
	}
};

void Harmonizer::handleSostenutoPedal(const bool isDown)
{
	const ScopedLock sl (lock);
	
	for (auto* voice : voices)
	{
		if (isDown)
			voice->setSostenutoPedalDown (true);
		else if (voice->isSostenutoPedalDown())
			stopVoice (voice, 1.0f, true);
	}
};

void Harmonizer::handleSoftPedal(const bool isDown)
{
	ignoreUnused(isDown);
};


// voice allocation----------------------------------------------------------------------------------------------------------------------------------
HarmonizerVoice* Harmonizer::findFreeVoice (const int midiNoteNumber, const bool stealIfNoneAvailable) const
{
	const ScopedLock sl (lock);
	
	for (auto* voice : voices)
		if (! voice->isVoiceActive())
			return voice;
	
	if (stealIfNoneAvailable)
		return findVoiceToSteal (midiNoteNumber);
	
	return nullptr;
};

HarmonizerVoice* Harmonizer::findVoiceToSteal (const int midiNoteNumber) const
{
	// This voice-stealing algorithm applies the following heuristics:
	// - Re-use the oldest notes first
	// - Protect the lowest & topmost notes, even if sustained, but not if they've been released.
	
	jassert (! voices.isEmpty());
	
	// These are the voices we want to protect (ie: only steal if unavoidable)
	HarmonizerVoice* low = nullptr; // Lowest sounding note, might be sustained, but NOT in release phase
	HarmonizerVoice* top = nullptr; // Highest sounding note, might be sustained, but NOT in release phase
	
	// this is a list of voices we can steal, sorted by how long they've been running
	Array<HarmonizerVoice*> usableVoices;
	usableVoices.ensureStorageAllocated (voices.size());
	
	for (auto* voice : voices)
	{
		
		jassert (voice->isVoiceActive()); // We wouldn't be here otherwise
		
		usableVoices.add (voice);
		
		// NB: Using a functor rather than a lambda here due to scare-stories about
		// compilers generating code containing heap allocations..
		struct Sorter
		{
			bool operator() (const HarmonizerVoice* a, const HarmonizerVoice* b) const noexcept { return a->wasStartedBefore (*b); }
		};
		
		std::sort (usableVoices.begin(), usableVoices.end(), Sorter());
		
		if (! voice->isPlayingButReleased()) // Don't protect released notes
		{
			auto note = voice->getCurrentlyPlayingNote();
			
			if (low == nullptr || note < low->getCurrentlyPlayingNote())
				low = voice;
			
			if (top == nullptr || note > top->getCurrentlyPlayingNote())
				top = voice;
		}
		
	}
	
	// Eliminate pathological cases (ie: only 1 note playing): we always give precedence to the lowest note(s)
	if (top == low)
		top = nullptr;
	
	// The oldest note that's playing with the target pitch is ideal..
	for (auto* voice : usableVoices)
		if (voice->getCurrentlyPlayingNote() == midiNoteNumber)
			return voice;
	
	// Oldest voice that has been released (no finger on it and not held by sustain pedal)
	for (auto* voice : usableVoices)
		if (voice != low && voice != top && voice->isPlayingButReleased())
			return voice;
	
	// Oldest voice that doesn't have a finger on it:
	for (auto* voice : usableVoices)
		if (voice != low && voice != top && ! voice->isKeyDown())
			return voice;
	
	// Oldest voice that isn't protected
	for (auto* voice : usableVoices)
		if (voice != low && voice != top)
			return voice;
	
	// We've only got "protected" voices now: lowest note takes priority
	jassert (low != nullptr);
	
	// Duophonic synth: give priority to the bass note:
	if (top != nullptr)
		return top;
	
	return low;
};


// update ADSR settings------------------------------------------------------------------------------------------------------------------------------
void Harmonizer::updateADSRsettings(const float attack, const float decay, const float sustain, const float release)
{
	// attack/decay/release time in SECONDS; sustain ratio 0.0 - 1.0
	
	const ScopedLock sl (lock);
	
	for (auto* voice : voices)
		voice->updateAdsrSettings(attack, decay, sustain, release);
};

void Harmonizer::setADSRonOff(const bool shouldBeOn)
{
	const ScopedLock sl (lock);
	
	for(auto* voice : voices)
		voice->setAdsrOnOff(shouldBeOn);
};


// functions for management of HarmonizerVoices------------------------------------------------------------------------------------------------------
HarmonizerVoice* Harmonizer::addVoice(HarmonizerVoice* newVoice)
{
	const ScopedLock sl (lock);
	newVoice->setCurrentPlaybackSamplerate(sampleRate);
	return voices.add(newVoice);
};

void Harmonizer::removeVoice(const int index)
{
	const ScopedLock sl (lock);
	voices.remove(index);
};

HarmonizerVoice* Harmonizer::getVoice(const int index) const
{
	const ScopedLock sl (lock);
	return voices[index];
};

void Harmonizer::deleteAllVoices()
{
	const ScopedLock sl (lock);
	voices.clear();
};
