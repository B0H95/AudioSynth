#include "audiofilter.hh"

AudioFilter::AudioFilter(int newSampleRate) :
    AudioComponent(),
    currentSample(0.0f),
    sampleComplete(false),
    sampleRate(newSampleRate),
    timeConstant(1.0f),
    timeInterval(1.0f/newSampleRate)
{
}

float AudioFilter::GetSample()
{
    if (!sampleComplete)
    {
	float alpha = timeInterval / (timeInterval + timeConstant);
	currentSample = currentSample + alpha * (MixSource(0) - currentSample);
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
    SendResetSignal();
}

void AudioFilter::SetTimeConstant(float tc)
{
    timeConstant = tc;
}
