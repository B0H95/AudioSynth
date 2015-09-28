#ifndef AUDIOFILTER_H
#define AUDIOFILTER_H

#include "audiocomponent.hh"

enum FilterType {LOWPASS, HIGHPASS};

class AudioFilter : public AudioComponent
{
public:
    AudioFilter(int newSampleRate);

    float GetSample();
    void NextSample();
    void Reset();

    void SetCutoffValue(float cutoff);
    void SetFilterType(FilterType ftype);

private:
    float currentSample;
    float cutoffValue;
    FilterType filterType;
    float previousMix;
    bool sampleComplete;
    int sampleRate;
};

#endif
