#include "wavetable.hh"

WaveTable::WaveTable() :
    waveData()
{
}

// NOT WAVETABLE

float GeneratePulseWave(float timePassed, float frequency, float pulseWidth)
{
    float sample = ((timePassed * frequency) - floor(timePassed * frequency));
    if (sample < pulseWidth)
    {
	return 1.0f;
    }
    return -1.0f;
}

float GenerateSawWave(float timePassed, float frequency, bool ascending)
{
    float sample = 2 * ((timePassed * frequency) - floor(timePassed * frequency)) - 1;
    if (ascending)
    {
	return sample;
    }
    return -sample;
}

float GenerateSineWave(float timePassed, float frequency)
{
    return sin(2*3.141592*timePassed*(frequency));
}

float GenerateTriangleWave(float timePassed, float frequency, bool ascending)
{
    float sample = ((timePassed * frequency) - floor(timePassed * frequency));
    if (sample > 0.5f)
    {
	sample = 1 - sample;
    }
    sample = (4 * sample) - 1;
    if (ascending)
    {
	return sample;
    }
    return -sample;
}
