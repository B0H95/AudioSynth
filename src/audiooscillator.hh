#ifndef AUDIOOSCILLATOR_H
#define AUDIOOSCILLATOR_H

#include <map>
#include <math.h>
#include <string>
#include <vector>

#include "audiocomponent.hh"
#include "audiocontroller.hh"
#include "wavetable.hh"

class AudioOscillator : public AudioComponent, public AudioController
{
public:
    AudioOscillator(int newSampleRate);

    float GetSample();
    void NextSample();
    void Reset();

    void Release(int index);
    void Trigger(float freq, float force, int index);
    bool Triggered(int index);

    void SetADSR(float a, float d, float s, float r);
    void SetAmplitude(float newAmp);
    void SetFrequencyTuning(float newFreqT);
    void SetSampleRate(int newSampleRate);
    void SetSemitoneTuning(int newSemiT);
    void SetSustain(bool newSustain);
    bool SetWaveform(std::string waveformName, WaveTable& wavetable);

private:
    float GetADSRModifier(float timePassed);
    float HandleNotes();
    
    struct note
    {
	float force;
	float frequency;
	bool sustains;
	float time;
        int waveformProgress;
    };

    float amplitude;
    float attack;
    float currentSample;
    float decay;
    std::vector<int> deletionMarks;
    float frequencyTuning;
    std::map<int, note> noteMap;
    float release;
    bool sampleComplete;
    int sampleRate;
    int semitoneTuning;
    bool shallSustain;
    float sustain;
    std::vector<float>* waveform;
    int waveformSize;
};

#endif
