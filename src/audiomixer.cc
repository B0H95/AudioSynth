#include "audiomixer.hh"

AudioMixer::AudioMixer(int newSampleRate) :
    amplitude(1.0f),
    currentSample(0.0f),
    sampleComplete(false),
    sampleRate(newSampleRate)
{
}

float AudioMixer::GetSample()
{
    if (!sampleComplete)
    {
	currentSample = 0.0f;
	for (auto& source : GetAvailableSources())
	{
	    currentSample += MixSource(source);
	}
	currentSample *= amplitude;
	sampleComplete = true;
    }
    return currentSample;
}

void AudioMixer::NextSample()
{
    sampleComplete = false;
    SendNextSampleSignal();
}

void AudioMixer::Reset()
{
    sampleComplete = false;
    currentSample = 0.0f;
    SendResetSignal();
}

void AudioMixer::SetAmplitudeModifier(float amp)
{
    amplitude = amp;
}
