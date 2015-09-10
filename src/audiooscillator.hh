#ifndef AUDIOOSCILLATOR_H
#define AUDIOOSCILLATOR_H

#include <string>
#include <vector>

#include "audiocomponent.hh"
#include "wavetable.hh"

class AudioOscillator : public AudioComponent
{
public:
    AudioOscillator(int newSampleRate);

    float GetSample();
    void NextSample();
    void Reset();

    void SetAmplitude(float newAmp);
    void SetFrequency(float newFreq);
    void SetSampleRate(int newSampleRate);
    bool SetWaveform(std::string waveformName, WaveTable& wavetable);

private:
    float amplitude;
    float currentSample;
    float frequency;
    bool sampleComplete;
    int sampleRate;
    std::vector<float>* waveform;
    int waveformProgress;
    int waveformSize;
};

#endif
