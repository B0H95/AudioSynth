#include <iostream>
#include <SDL2/SDL.h>

#include "audiofilter.hh"
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
    osc.SetADSR(0.00001f, 0.00001f, 1.0f, 1.0f);
    osc.SetAmplitude(1.0f);
    osc.SetSustain(true);
    osc.TriggerNote(440.00f);
    if (!osc.SetWaveform("pulse", wavetable))
    {
	cout << "Tried to load a non-existing waveform. Closed." << endl;
	return 1;
    }

    float tc = 0.000001f;
    AudioFilter lowpass (44100);
    lowpass.SetTimeConstant(tc);
    lowpass.AddSource(0, &osc);

    int playtime;
    cout << "Enter playtime in milliseconds: ";
    cin >> playtime;

    for (int i=0; i<44*playtime; ++i)
    {
	//as << lowpass.GetSample();
	DrawSample(-1.0f, lowpass.GetSample(), 1.0f, 40);
	lowpass.NextSample();
	if (tc <= 1.0f)
	{
	    tc += 0.000000003f;
	}
	lowpass.SetTimeConstant(tc);
    }

    SDL_Delay(playtime);

    return 0;
}
