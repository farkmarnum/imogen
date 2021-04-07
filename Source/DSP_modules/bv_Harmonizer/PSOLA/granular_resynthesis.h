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
 
 granular_resynthesis.h:   This file defines AnalysisGrain and SynthesisGrain classes, which are used in the PSOLA process.
 
======================================================================================================================================================*/


namespace bav
{

/*------------------------------------------------------------------------------------------------------------------------------------------------------
 AnalysisGrain :    This class stores the actual audio samples that comprise a single audio grain, with a Hann window applied.
------------------------------------------------------------------------------------------------------------------------------------------------------*/

template<typename SampleType>
class AnalysisGrain
{
public:
    AnalysisGrain(): numActive(0), origStart(0), origEnd(0), size(0) { }
    
    void reserveSize (int numSamples) { samples.setSize(1, numSamples); }
    
    void incNumActive() noexcept { ++numActive; }
    
    void decNumActive() noexcept { --numActive; }
    
    void storeNewGrain (const SampleType* inputSamples, int startSample, int endSample)
    {
        samples.clear();
        origStart = startSample;
        origEnd = endSample;
        size = endSample - startSample + 1;
        jassert (size > 0);
        jassert (samples.getNumSamples() >= size);
        
        auto* writing = samples.getWritePointer(0);
        vecops::copy (inputSamples + startSample, writing, size);
        
        //  apply Hann window to input samples
        for (int s = 0; s < size; ++s)
            writing[s] *= getWindowValue (size, s);
    }
    
    void clear()
    {
        samples.clear();
        size = 0;
        origStart = 0;
        origEnd = 0;
        numActive = 0;
    }
    
    int numReferences() const noexcept { return numActive; }
    
    SampleType getSample (int index) const
    {
        jassert (index < size);
        return samples.getSample (0, index);
    }
    
    int getSize() const noexcept { return size; }
    
    int getStartSample() const noexcept { return origStart; }
    
    int getEndSample() const noexcept { return origEnd; }
    
    bool isEmpty() const noexcept { return size < 1; }
    
    
private:
    inline SampleType getWindowValue (int windowSize, int index)
    {
        const auto cos2 = std::cos (static_cast<SampleType> (2 * index)
                                    * juce::MathConstants<SampleType>::pi / static_cast<SampleType> (windowSize - 1));
        
        return static_cast<SampleType> (0.5 - 0.5 * cos2);
    }
    
    int numActive; // this counts the number of SynthesisGrains that are referring to this AnalysisGrain
    
    int origStart, origEnd;  // the original start & end sample indices of this grain
    
    int size;
    
    juce::AudioBuffer<SampleType> samples;
};

template class AnalysisGrain<float>;
template class AnalysisGrain<double>;
    
    

/*------------------------------------------------------------------------------------------------------------------------------------------------------
SynthesisGrain :   This class holds a pointer to a specific AnalysisGrain, and its respacing information so it can be used to create a stream of repitched audio.
------------------------------------------------------------------------------------------------------------------------------------------------------*/

template<typename SampleType>
class SynthesisGrain
{
    using Grain = AnalysisGrain<SampleType>;
    
public:
    SynthesisGrain(): readingIndex(0), grain(nullptr), zeroesLeft(0), halfIndex(0) { }
    
    bool isActive() const noexcept { return size > 0; }
    
    int halfwayIndex() const noexcept { return halfIndex; }
    
    void startNewGrain (Grain* newGrain, int synthesisMarker)
    {
        jassert (newGrain != nullptr && ! newGrain->isEmpty());
        newGrain->incNumActive();
        grain = newGrain;
        
        readingIndex = 0;
        zeroesLeft = synthesisMarker;
        size = grain->getSize();
        jassert (size > 0);
        halfIndex = juce::roundToInt (size * 0.5f);
    }
    
    SampleType getNextSample()
    {
        jassert (isActive() && ! grain->isEmpty());
        
        if (zeroesLeft > 0)
        {
            jassert (readingIndex == 0);
            --zeroesLeft;
            return SampleType(0);
        }
        
        const auto sample = grain->getSample (readingIndex);
        
        ++readingIndex;

        if (readingIndex >= size)
            stop();

        return sample;
    }
    
    int samplesLeft() const
    {
        if (isActive())
            return grain->getSize() - readingIndex + std::max(0, zeroesLeft);
        
        return 0;
    }
    
    void stop()
    {
        readingIndex = 0;
        zeroesLeft = 0;
        halfIndex = 0;
        size = 0;
        
        if (grain != nullptr)
        {
            grain->decNumActive();
            grain = nullptr;
        }
    }
    
    
private:
    int readingIndex;  // the next index to be read from the AnalysisGrain's buffer
    Grain* grain;
    int zeroesLeft;  // the number of zeroes this grain will output before actually outputting its samples. This allows grains to be respaced into the future.
    int halfIndex;  // marks the halfway point for this grain
    int size;
};
    
template class SynthesisGrain<float>;
template class SynthesisGrain<double>;


}  // namespace
