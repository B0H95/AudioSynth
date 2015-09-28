#include "audiofilter.hh"

AudioFilter::AudioFilter(int newSampleRate) :
    AudioComponent(),
    currentSample(0.0f),
    cutoffValue(1.0f),
    filterType(LOWPASS),
    previousMix(0.0f),
    sampleComplete(false),
    sampleRate(newSampleRate)
{
}

float AudioFilter::GetSample()
{
    if (!sampleComplete)
    {
	float mix = MixSource(0);
	if (filterType == LOWPASS)
	{
	    currentSample = currentSample + cutoffValue * (mix - currentSample);
	}
	else if (filterType == HIGHPASS)
	{
	    currentSample = cutoffValue * (currentSample + mix - previousMix);
	}
	previousMix = mix;
	sampleComplete = true;
    }
    return currentSample;
}

void AudioFilter::NextSample()
{
    sampleComplete = false;
    SendNextSampleSignal();
}

void AudioFilter::Reset()
{
    sampleComplete = false;
    currentSample = 0.0f;
    previousMix = 0.0f;
    SendResetSignal();
}

void AudioFilter::SetCutoffValue(float cutoff)
{
    if (0.0f <= cutoff && cutoff <= 1.0f)
    {
	cutoffValue = cutoff;
    }
}

void AudioFilter::SetFilterType(FilterType ftype)
{
    filterType = ftype;
}
