
#pragma once

namespace Imogen
{

struct Internals :  bav::ParameterList
{
    using IntParam   = bav::IntParam;
    using BoolParam  = bav::BoolParam;
    
    Internals()
    : ParameterList ("ImogenInternals")
    {
        addInternal (abletonLinkEnabled, abletonLinkSessionPeers, mtsEspIsConnected, editorPitchbend, lastMovedMidiController, lastMovedCCValue, guiDarkMode, currentCentsSharp, editorSizeX, editorSizeY);
    }
    
    
    BoolParam abletonLinkEnabled {"Toggle", "Ableton link toggle", false, l::toggle_stringFromBool, l::toggle_boolFromString};
    
    IntParam abletonLinkSessionPeers {"Num peers", "Ableton link num session peers", 0, 50, 0,
        [] (int value, int maximumStringLength)
        { return juce::String (value).substring (0, maximumStringLength); },
        nullptr};
    
    BoolParam mtsEspIsConnected {"Is connected", "MTS-ESP is connected", false, l::toggle_stringFromBool, l::toggle_boolFromString};
    
    IntParam editorPitchbend {"Pitchbend", "GUI pitchbend", 0, 127, 64,
        [] (int value, int maximumStringLength)
        { return juce::String (value).substring (0, maximumStringLength); },
        [] (const juce::String& text)
        { return text.retainCharacters ("1234567890").getIntValue(); }};
    
    IntParam lastMovedMidiController {"Number", "Last moved MIDI controller number", 0, 127, 0};
    
    IntParam lastMovedCCValue {"Value", "Last moved MIDI controller value", 0, 127, 0};
    
    BoolParam guiDarkMode {"Dark mode", "GUI Dark mode", true,
        [] (bool val, int maxLength)
        {
            if (val) return TRANS ("Dark mode is on").substring (0, maxLength);
            
            return TRANS ("Dark mode is off").substring (0, maxLength);
        },
        nullptr};
    
    IntParam currentCentsSharp {"Cents sharp", "Current input cents sharp", -100, 100, 0,
        [] (int cents, int maxLength)
        {
            if (cents == 0) return TRANS ("Perfect!");
            
            if (cents > 0) return (juce::String (cents) + TRANS (" cents sharp")).substring (0, maxLength);
            
            return (juce::String (abs (cents)) + TRANS (" cents flat")).substring (0, maxLength);
        },
        nullptr,
        TRANS ("cents")};
    
    IntParam editorSizeX {"editorSizeX", "editor size X", 0, 10000, 900};
    IntParam editorSizeY {"editorSizeY", "editor size Y", 0, 10000, 400};
    
};

}  // namespace


// auto scaleName = std::make_unique< StringNode > ("Scale name", "MTS-ESP scale name", "No active scale");
// auto currentNote = std::make_unique< StringNode > ("Current note", "Current input note as string", "-");
