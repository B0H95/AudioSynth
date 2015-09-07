#ifndef WAVETABLE_H
#define WAVETABLE_H

#include <map>
#include <math.h>
#include <string>
#include <vector>

/* 
   Standard wave names:
    - Pulse wave = "pulse"
    - Ascending saw wave = "saw asc"
    - Descending saw wave = "saw desc"
    - Sine wave = "sine"
    - Ascending triangle wave = "tri asc"
    - Descending triangle wave = "tri desc"
 */

class WaveTable
{
public:
    WaveTable(int newSampleAmount);

    int GetWaveformSize();
    void LoadStandardWaveforms();
    std::vector<float>* RequestWaveform(std::string waveformName);

private:
    int sampleAmount;
    std::map<std::string, std::vector<float>> waveData;

    void LoadPulseWave();
    void LoadSawWaves();
    void LoadSineWave();
    void LoadTriangleWaves();
};

float GeneratePulseWave(float timePassed, float frequency, float pulseWidth);
float GenerateSawWave(float timePassed, float frequency, bool ascending);
float GenerateSineWave(float timePassed, float frequency);
float GenerateTriangleWave(float timePassed, float frequency, bool ascending);

#endif
