#ifndef AUDIOGENERATORS_H
#define AUDIOGENERATORS_H

#include <math.h>

float GeneratePulseWave(float timePassed, float frequency, float pulseWidth);
float GenerateSawWave(float timePassed, float frequency, bool ascending);
float GenerateSineWave(float timePassed, float frequency);
float GenerateTriangleWave(float timePassed, float frequency, bool ascending);

#endif
