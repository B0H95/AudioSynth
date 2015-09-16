#ifndef AUDIOFILTER_H
#define AUDIOFILTER_H

#include "audiocomponent.hh"

class AudioFilter : public AudioComponent
{
public:
    AudioFilter(int newSampleRate);

    float GetSample();
    void NextSample();
    void Reset();

    void SetTimeConstant(float tc);

private:
    float currentSample;
    bool sampleComplete;
    int sampleRate;
    float timeConstant;
    float timeInterval;
};

#endif
