#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H

#include "audiocomponent.hh"

class AudioMixer : public AudioComponent
{
public:
    AudioMixer(int newSampleRate);

    float GetSample();
    void NextSample();
    void Reset();

    void SetAmplitudeModifier(float amp);

private:
    float amplitude;
    float currentSample;
    bool sampleComplete;
    int sampleRate;
};

#endif
