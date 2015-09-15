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

    WaveTable wavetable (1000000);
    cout << "Loading standard waveforms..." << flush;
    wavetable.LoadStandardWaveforms();
    cout << "Complete" << endl;

    AudioOscillator osc (44100);
    osc.SetADSR(0.5f, 0.2f, 0.6f, 4.0f);
    osc.SetAmplitude(0.33f);
    osc.SetSustain(false);
    osc.TriggerNote(293.66f);
    osc.TriggerNote(349.23f);
    osc.TriggerNote(440.00f);
    if (!osc.SetWaveform("tri asc", wavetable))
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
