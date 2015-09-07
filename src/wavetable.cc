#include "wavetable.hh"

// PUBLIC

WaveTable::WaveTable(int newSampleAmount) :
    sampleAmount(newSampleAmount),
    waveData()
{
}

int WaveTable::GetWaveformSize()
{
    return sampleAmount;
}

void WaveTable::LoadStandardWaveforms()
{
    LoadPulseWave();
    LoadSawWaves();
    LoadSineWave();
    LoadTriangleWaves();
}

std::vector<float>* WaveTable::RequestWaveform(std::string waveformName)
{
    if (waveData.find(waveformName) != waveData.end())
    {
	return &waveData[waveformName];
    }
    return nullptr;
}

// PRIVATE

void WaveTable::LoadPulseWave()
{
    if (waveData.find("pulse") == waveData.end())
    {
	waveData["pulse"] = std::vector<float>(sampleAmount, 0.0f);
	for (int sampleCounter = 0; sampleCounter < sampleAmount; ++sampleCounter)
	{
	    waveData["pulse"][sampleCounter] = GeneratePulseWave((float)sampleCounter/sampleAmount, 1.0f, 0.5f);
	}
    }
}

void WaveTable::LoadSawWaves()
{
    if (waveData.find("saw asc") == waveData.end())
    {
	waveData["saw asc"] = std::vector<float>(sampleAmount, 0.0f);
	for (int sampleCounter = 0; sampleCounter < sampleAmount; ++sampleCounter)
	{
	    waveData["saw asc"][sampleCounter] = GenerateSawWave((float)sampleCounter/sampleAmount, 1.0f, true);
	}
    }
    if (waveData.find("saw desc") == waveData.end())
    {
	waveData["saw desc"] = std::vector<float>(sampleAmount, 0.0f);
	for (int sampleCounter = 0; sampleCounter < sampleAmount; ++sampleCounter)
	{
	    waveData["saw desc"][sampleCounter] = GenerateSawWave((float)sampleCounter/sampleAmount, 1.0f, false);
	}
    }
}

void WaveTable::LoadSineWave()
{
    if (waveData.find("sine") == waveData.end())
    {
	waveData["sine"] = std::vector<float>(sampleAmount, 0.0f);
	for (int sampleCounter = 0; sampleCounter < sampleAmount; ++sampleCounter)
	{
	    waveData["sine"][sampleCounter] = GenerateSineWave((float)sampleCounter/sampleAmount, 1.0f);
	}
    }
}

void WaveTable::LoadTriangleWaves()
{
    if (waveData.find("tri asc") == waveData.end())
    {
	waveData["tri asc"] = std::vector<float>(sampleAmount, 0.0f);
	for (int sampleCounter = 0; sampleCounter < sampleAmount; ++sampleCounter)
	{
	    waveData["tri asc"][sampleCounter] = GenerateTriangleWave((float)sampleCounter/sampleAmount, 1.0f, true);
	}
    }
    if (waveData.find("tri desc") == waveData.end())
    {
	waveData["tri desc"] = std::vector<float>(sampleAmount, 0.0f);
	for (int sampleCounter = 0; sampleCounter < sampleAmount; ++sampleCounter)
	{
	    waveData["tri desc"][sampleCounter] = GenerateTriangleWave((float)sampleCounter/sampleAmount, 1.0f, false);
	}
    }
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
    return sin(2 * 3.141592 * timePassed * (frequency));
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
