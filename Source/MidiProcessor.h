/*
  ==============================================================================

    MidiProcessor.h
    Created: 3 Nov 2020 1:44:22am
    Author:  Ben Vining
 
 	This class processes incoming MIDI. Note ons and offs are routed to the appropriate functions of the PolhyphonyVoiceManager class (PolyphonyVoiceManager will actually DO the reporting/storing/recalling of voices), and resulting data is routed to the appropraite instance of HarmonyVoice.

  ==============================================================================
*/

#pragma once

#include "PolyphonyVoiceManager.h"
#include "MidiPanningManager.h"
#include "MidiLatchManager.h"

class MidiProcessor
{
	
public:
	
											
	void processIncomingMidi (MidiBuffer& midiMessages, OwnedArray<HarmonyVoice>& harmonyEngine)
	{
		
		for (const auto meta : midiMessages)
		{
			const auto currentMessage = meta.getMessage();
			
				if(currentMessage.isNoteOnOrOff())
				{
					if(midiLatch == false) {
						if(currentMessage.isNoteOn()) {
							harmonyNoteOn(currentMessage, harmonyEngine);
						} else {
							harmonyNoteOff(currentMessage.getNoteNumber(), harmonyEngine);
						}
					} else {
						processActiveLatch(currentMessage, harmonyEngine);
					}
				}
				else
				{
					if(currentMessage.isPitchWheel())
					{
						const int pitchBend = currentMessage.getPitchWheelValue();
						for(int i = 0; i < numberOfVoices; ++i) {
							if(harmonyEngine[i]->voiceIsOn) {
								harmonyEngine[i]->pitchBend(pitchBend);
							}
						}
						lastRecievedPitchBend = pitchBend;
					} else {
						// non-note events go to here...
						// sustain pedal, aftertouch, key pressure, etc...
					}
				}
			
		}
	};

	
	void updateStereoWidth(float* newStereoWidth) {
		midiPanningManager.updateStereoWidth(*newStereoWidth);
	};
	
	
	void refreshMidiPanVal (OwnedArray<HarmonyVoice>& harmonyEngine, const int voiceNumber, const int indexToRead) {
		const int newPanVal = midiPanningManager.retrievePanVal(indexToRead);
		harmonyEngine[voiceNumber]->changePanning(newPanVal);
	};
	
	
	// actually sends a new note on out to the harmony engine
	void harmonyNoteOn(const MidiMessage currentMessage, OwnedArray<HarmonyVoice>& harmonyEngine)
	{
		const int newPitch = currentMessage.getNoteNumber();
		const int newVelocity = currentMessage.getVelocity();
		const int newVoiceNumber = polyphonyManager.nextAvailableVoice();  // returns -1 if no voices are available
		
		if(newVoiceNumber >= 0) {
			polyphonyManager.updatePitchCollection(newVoiceNumber, newPitch);
			harmonyEngine[newVoiceNumber]->startNote(newPitch, newVelocity, midiPanningManager.getNextPanVal(), lastRecievedPitchBend);
		} else {
			// no voices are available to turn on
			// if voice stealing is enabled, then assign the note to the voice that has been holding its note the LONGEST
		}
	};
	
	
	// actually sends a note off out to the harmony engine
	void harmonyNoteOff(const int pitch, OwnedArray<HarmonyVoice>& harmonyEngine) {
		const int voiceToTurnOff = polyphonyManager.turnOffNote(pitch);
		polyphonyManager.updatePitchCollection(voiceToTurnOff, -1);
		harmonyEngine[voiceToTurnOff]->stopNote();
	};
	
	
	
	
	void processActiveLatch(const MidiMessage currentMessage, OwnedArray<HarmonyVoice>& harmonyEngine)
	{
		const int midiPitch = currentMessage.getNoteNumber();
		if(currentMessage.isNoteOn())
		{
			if (polyphonyManager.isPitchActive(midiPitch) == false) {
				harmonyNoteOn(currentMessage, harmonyEngine);  // if note isn't already on (latched), then turn it on
			} else {
				latchManager.noteOnRecieved(midiPitch);
			}
		} else {
			latchManager.noteOffRecieved(midiPitch);
		}
	}; // processes note events that occur while midiLatch is active
	
	
	
	
private:
	PolyphonyVoiceManager polyphonyManager;
	MidiPanningManager midiPanningManager;
	
	const static int numberOfVoices = 12;  // link this to global # of voices setting
	
	int lastRecievedPitchBend = 64;
	
	bool midiLatch = false; // link this to global midi latch toggle on/off setting
	MidiLatchManager latchManager;
};
