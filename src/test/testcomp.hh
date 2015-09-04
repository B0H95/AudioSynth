#ifndef TESTCOMP_H
#define TESTCOMP_H

#include "audiocomponent.hh"
#include "audiogenerators.hh"

class TestComp : public AudioComponent
{
public:
    TestComp() : AudioComponent(), sample(0.0f), sampleCount(0), sampleGenerated(false) {}
    float GetSample()
    {
	if (!sampleGenerated)
	{
	    sample = GenerateSineWave(sampleCount/44100.0f, 440.0f);
	    ++sampleCount;
	    sampleGenerated = true;
	}
	return sample;
    }
    void NextSample()
    {
	if (sampleGenerated)
	{
	    sampleGenerated = false;
	    SendNextSampleSignal();
	}
    }
    void Reset()
    {
	sampleGenerated = false;
	sample = 0.0f;
	sampleCount = 0;
	SendResetSignal();
    }

private:
    float sample;
    int sampleCount;
    bool sampleGenerated;
    
};

#endif
