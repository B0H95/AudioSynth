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

    void ReleaseNote(float freq);
    void SetADSR(float a, float d, float s, float r);
    void SetAmplitude(float newAmp);
    void SetSampleRate(int newSampleRate);
    void SetSustain(bool newSustain);
    bool SetWaveform(std::string waveformName, WaveTable& wavetable);
    void TriggerNote(float freq);

private:
    float GetADSRModifier(float timePassed);
    float HandleNotes();
    
    struct note
    {
	float frequency;
	bool sustains;
	float time;
        int waveformProgress;
    };

    float amplitude;
    float attack;
    float currentSample;
    float decay;
    std::vector<note> noteList;
    float release;
    bool sampleComplete;
    int sampleRate;
    bool shallSustain;
    float sustain;
    std::vector<float>* waveform;
    int waveformSize;
};

#endif
