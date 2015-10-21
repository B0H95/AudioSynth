#include "audiooscillator.hh"

// PUBLIC

AudioOscillator::AudioOscillator(int newSampleRate) :
    AudioComponent(),
    AudioController(),
    amplitude(1.0f),
    attack(0.0f),
    currentSample(0.0f),
    decay(0.0f),
    deletionMarks(),
    frequencyTuning(0.0f),
    noteMap(),
    release(0.0f),
    sampleComplete(false),
    sampleRate(newSampleRate),
    semitoneTuning(0),
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
    noteMap.clear();
    currentSample = 0.0f;
    SendResetSignal();
}

void AudioOscillator::Release(int index)
{
    if (noteMap.find(index) != noteMap.end())
    {
	noteMap[index].sustains = false;
    }
}

void AudioOscillator::Trigger(float freq, float force, int index)
{
    noteMap[index] = {force, freq * (float)pow(1.0594630943592953f, (float)semitoneTuning), shallSustain, 0.0f, 0};
}

bool AudioOscillator::Triggered(int index)
{
    return (noteMap.find(index) != noteMap.end()) && (noteMap[index].sustains);
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

void AudioOscillator::SetFrequencyTuning(float newFreqT)
{
    frequencyTuning = newFreqT;
}

void AudioOscillator::SetSampleRate(int newSampleRate)
{
    sampleRate = newSampleRate;
}

void AudioOscillator::SetSemitoneTuning(int newSemiT)
{
    semitoneTuning = newSemiT;
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

// PRIVATE

float AudioOscillator::GetADSRModifier(float timePassed)
{
    if (timePassed < attack)
    {
	return timePassed / attack;
    }
    else if (timePassed < attack + decay)
    {
        return sustain + ((1 - ((timePassed - attack) / decay)) * (1 - sustain));
    }
    else if (timePassed < attack + decay + release)
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
    for (std::pair<const int, note>& n : noteMap)
    {
	finalMix += (*waveform)[int(n.second.waveformProgress)] * GetADSRModifier(n.second.time) * n.second.force;
	
	n.second.waveformProgress = (n.second.waveformProgress + int((waveformSize * (n.second.frequency + frequencyTuning)) / sampleRate)) % waveformSize;
	if (n.second.waveformProgress < 0)
	{
	    n.second.waveformProgress = 0;
	}
	if (n.second.time >= attack + decay && n.second.sustains)
	{
	    n.second.time = attack + decay;
	}
	else
	{
	    n.second.time += 1.0f / sampleRate;
	}
	if (n.second.time > attack + decay + release)
	{
	    deletionMarks.push_back(n.first);
	}
    }
    for (int& element : deletionMarks)
    {
	noteMap.erase(element);
    }
    deletionMarks.clear();
    return finalMix * amplitude;
}
