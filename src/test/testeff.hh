#ifndef TESTEFF_H
#define TESTEFF_H

#include "audiocomponent.hh"

class TestEff : public AudioComponent
{
public:
    TestEff() : AudioComponent(), sample(0.0f), sampleGenerated(false) {}
    float GetSample()
    {
	if (!sampleGenerated)
	{
	    sample = MixSource(0);
	    sample *= 0.5;
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
	SendResetSignal();
    }

private:
    float sample;
    bool sampleGenerated;
    
};

#endif
