#ifndef WAVETABLE_H
#define WAVETABLE_H

#include <map>
#include <math.h>
#include <string>
#include <vector>

class WaveTable
{
public:
    WaveTable();

private:
    std::map<std::string, std::vector<float>> waveData;
};

float GeneratePulseWave(float timePassed, float frequency, float pulseWidth);
float GenerateSawWave(float timePassed, float frequency, bool ascending);
float GenerateSineWave(float timePassed, float frequency);
float GenerateTriangleWave(float timePassed, float frequency, bool ascending);

#endif
