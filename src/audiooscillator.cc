#include "audiooscillator.hh"

AudioOscillator::AudioOscillator(int newSampleRate) :
    AudioComponent(),
    amplitude(1.0f),
    currentSample(0.0f),
    frequency(1.0f),
    sampleComplete(false),
    sampleRate(newSampleRate),
    waveform(nullptr),
    waveformProgress(0),
    waveformSize(0)
{
}

float AudioOscillator::GetSample()
{
    if (!sampleComplete)
    {
	currentSample = (*waveform)[waveformProgress] * amplitude;
	waveformProgress = (waveformProgress + int((waveformSize * frequency) / sampleRate)) % waveformSize;
	sampleComplete = true;
    }
    return currentSample;
}

void AudioOscillator::NextSample()
{
    if (sampleComplete)
    {
	sampleComplete = false;
    }
}

void AudioOscillator::Reset()
{
    sampleComplete = false;
    waveformProgress = 0.0f;
    currentSample = 0.0f;
}

void AudioOscillator::SetAmplitude(float newAmp)
{
    amplitude = newAmp;
}

void AudioOscillator::SetFrequency(float newFreq)
{
    frequency = newFreq;
}

void AudioOscillator::SetSampleRate(int newSampleRate)
{
    sampleRate = newSampleRate;
}

bool AudioOscillator::SetWaveform(std::string waveformName, WaveTable& wavetable)
{
    waveformSize = wavetable.GetWaveformSize();
    return (waveform = wavetable.RequestWaveform(waveformName)) != nullptr;
}
