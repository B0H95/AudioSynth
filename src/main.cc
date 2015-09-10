#include <iostream>
#include <SDL2/SDL.h>

#include "audiooscillator.hh"
#include "audiostream.hh"
#include "utility.hh"
#include "wavetable.hh"

using namespace std;

int main()
{
    AudioStream as;

    WaveTable wavetable (100000);
    wavetable.LoadStandardWaveforms();

    AudioOscillator osc (44100);
    osc.SetFrequency(440.0f);
    if (!osc.SetWaveform("sine", wavetable))
    {
	cout << "Tried to load a non-existing waveform. Closed." << endl;
	return 1;
    }

    int playtime;
    cout << "Enter playtime in milliseconds: ";
    cin >> playtime;

    for (int i=0; i<44*playtime; ++i)
    {
	as << osc.GetSample();
	osc.NextSample();
    }

    SDL_Delay(playtime);

    return 0;
}
