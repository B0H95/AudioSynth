#include "audiooscillator.hh"

// PUBLIC

AudioOscillator::AudioOscillator(int newSampleRate) :
    AudioComponent(),
    amplitude(1.0f),
    attack(0.0f),
    currentSample(0.0f),
    decay(0.0f),
    noteList(),
    release(0.0f),
    sampleComplete(false),
    sampleRate(newSampleRate),
    shallSustain(true),
    sustain(1.0f),
    waveform(nullptr),
    waveformSize(0)
{
}

float AudioOscillator::GetSample()
{
    if (!sampleComplete)
    {
	currentSample = HandleNotes();
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
    SendNextSampleSignal();
}

void AudioOscillator::Reset()
{
    sampleComplete = false;
    noteList.clear();
    currentSample = 0.0f;
    SendResetSignal();
}

void AudioOscillator::ReleaseNote(float freq)
{
    for (note& n : noteList)
    {
	if (n.frequency == freq)
	{
	    n.sustains = false;
	    return;
	}
    }
}

void AudioOscillator::SetADSR(float a, float d, float s, float r)
{
    attack = a;
    decay = d;
    sustain = s;
    release = r;
}

void AudioOscillator::SetAmplitude(float newAmp)
{
    amplitude = newAmp;
}

void AudioOscillator::SetSampleRate(int newSampleRate)
{
    sampleRate = newSampleRate;
}

void AudioOscillator::SetSustain(bool newSustain)
{
    shallSustain = newSustain;
}

bool AudioOscillator::SetWaveform(std::string waveformName, WaveTable& wavetable)
{
    waveformSize = wavetable.GetWaveformSize();
    return (waveform = wavetable.RequestWaveform(waveformName)) != nullptr;
}

void AudioOscillator::TriggerNote(float freq)
{
    for (note& n : noteList)
    {
	if (n.frequency == freq)
	{
	    n.time = 0.0f;
	    n.waveformProgress = 0;
	    n.sustains = shallSustain;
	    return;
	}
    }
    noteList.push_back(note{freq, shallSustain, 0.0f, 0});
}

// PRIVATE

float AudioOscillator::GetADSRModifier(float timePassed)
{
    if (timePassed <= attack)
    {
	return timePassed / attack;
    }
    else if (timePassed <= attack + decay)
    {
        return sustain + ((1 - ((timePassed - attack) / decay)) * (1 - sustain));
    }
    else if (timePassed <= attack + decay + release)
    {
	return sustain * (1 - ((timePassed - attack - decay) / release));
    }
    else
    {
	return 0.0f;
    }
}

float AudioOscillator::HandleNotes()
{
    float finalMix = 0.0f;
    for (note& n : noteList)
    {
	finalMix += (*waveform)[int(n.waveformProgress)] * GetADSRModifier(n.time);
	n.waveformProgress = (n.waveformProgress + int((waveformSize * n.frequency) / sampleRate)) % waveformSize;
	if (n.time >= attack + decay && n.sustains)
	{
	    n.time = attack + decay;
	}
	else
	{
	    n.time += 1.0f / sampleRate;
	}
    }
    return finalMix * amplitude;
}
