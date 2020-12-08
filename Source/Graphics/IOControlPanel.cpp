/*
  ==============================================================================

    IOControlPanel.cpp
    Created: 7 Dec 2020 11:18:06pm
    Author:  Ben Vining

  ==============================================================================
*/

#include <JuceHeader.h>
#include "IOControlPanel.h"

//==============================================================================
IOControlPanel::IOControlPanel(ImogenAudioProcessor& p): audioProcessor(p)
{
	// dry pan
	{
		dryPan.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
		dryPan.setRange(0, 127);
		dryPan.setTextBoxStyle(Slider::TextBoxBelow, false, 40, 20);
		addAndMakeVisible(dryPan);
		dryPanLink = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.tree, "dryPan", dryPan);
		dryPan.setValue(64);
		initializeLabel(drypanLabel, "Modulator pan");
		addAndMakeVisible(drypanLabel);
	}
	
	// master dry/wet
	{
		masterDryWet.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
		masterDryWet.setRange(0, 100);
		masterDryWet.setTextBoxStyle(Slider::TextBoxBelow, false, 40, 20);
		addAndMakeVisible(masterDryWet);
		masterDryWetLink = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.tree, "masterDryWet", masterDryWet);
		masterDryWet.setValue(100);
		initializeLabel(drywetLabel, "% wet signal");
		addAndMakeVisible(drywetLabel);
	}
	
	// input gain
	{
		inputGain.setSliderStyle(Slider::SliderStyle::LinearVertical);
		inputGain.setRange(-60.0f, 0.0f);
		inputGain.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
		addAndMakeVisible(inputGain);
		inputGainLink = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.tree, "inputGain", inputGain);
		inputGain.setValue(0.0f);
		initializeLabel(inputGainLabel, "Input gain");
		addAndMakeVisible(inputGainLabel);
	}
	
	// output gain
	{
		outputGain.setSliderStyle(Slider::SliderStyle::LinearVertical);
		outputGain.setRange(-60.0f, 0.0f);
		outputGain.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 15);
		addAndMakeVisible(outputGain);
		outputGainLink = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.tree, "outputGain", outputGain);
		outputGain.setValue(-4.0f);
		initializeLabel(outputgainLabel, "Output gain");
		addAndMakeVisible(outputgainLabel);
	}
	
	// set input channel
	{
		inputChannel.setSliderStyle(Slider::SliderStyle::LinearBarVertical);
		inputChannel.setRange(0, 16, 1);
		inputChannel.setTextBoxStyle(Slider::TextBoxBelow, false, 40, 20);
		addAndMakeVisible(inputChannel);
		inputChannelLink = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.tree, "inputChan", inputChannel);
		inputChannel.setValue(0);
		initializeLabel(inputChannelLabel, "Input channel");
		addAndMakeVisible(inputChannelLabel);
	}
	
}

IOControlPanel::~IOControlPanel()
{
}

void IOControlPanel::paint (juce::Graphics& g)
{
	g.fillAll (juce::Colours::burlywood);
	
	g.setColour(juce::Colours::steelblue);
	
	juce::Rectangle<int> inputControlPanel (5, 5, 290, 125);
	g.fillRect(inputControlPanel);
	
	juce::Rectangle<int> outputControlPanel (5, 135, 290, 115);
	g.fillRect(outputControlPanel);
}

void IOControlPanel::resized()
{
    // input gain
	{
		inputGainLabel.setBounds(10, 0, 75, 35);
		inputGain.setBounds		(22, 25, 50, 100);
	}
	
	// input channel
	{
		inputChannelLabel.setBounds	(95, 0, 90, 35);
		inputChannel.setBounds		(122, 40, 35, 35);
	}
	
	// dry pan
	{
		drypanLabel.setBounds(200, 0, 90, 35);
		dryPan.setBounds	 (210, 25, 75, 75);
	}
	
	// master dry/wet { % wet signal }
	{
		drywetLabel.setBounds (50, 138, 75, 35);
		masterDryWet.setBounds(50, 163, 75, 75);
	}
	
	// output gain
	{
		outputgainLabel.setBounds(165, 130, 75, 35);
		outputGain.setBounds	 (177, 155, 50, 90);
	}
}

void IOControlPanel::initializeLabel(Label& label, String labelText)
{
	label.setFont(juce::Font(14.0f, juce::Font::bold));
	label.setJustificationType(juce::Justification::centred);
	label.setColour(juce::Label::textColourId, juce::Colours::black);
	label.setText(labelText, juce::dontSendNotification);
};
