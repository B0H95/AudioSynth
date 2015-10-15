#include <iostream>
#include <SDL2/SDL.h>

#include "audiofilter.hh"
#include "audiomixer.hh"
#include "audiooscillator.hh"
#include "audiostream.hh"
#include "inputkeyboardnote.hh"
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

    AudioOscillator osc1 (44100);
    osc1.SetADSR(0.25f, 0.25f, 0.6f, 0.5f);
    osc1.SetAmplitude(1.0f);
    osc1.SetSustain(false);
    if (!osc1.SetWaveform("pulse", wavetable))
    {
	cout << "Tried to load a non-existing waveform. Closed." << endl;
	return 1;
    }

    InputKeyboardNote keys;
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
	cerr << "Could not open video: " << SDL_GetError() << endl;
	return 1;
    }
    
    const int SCREEN_WIDTH = 640;
    const int SCREEN_HEIGHT = 480;

    SDL_Window* window = SDL_CreateWindow("AudioSynth", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    bool running = true;
    SDL_Event e;
    Uint32 beginTime = SDL_GetTicks();
    float deltaTime = 0.0f;
    Uint32 endTime = 0;

    while (running)
    {
	endTime = SDL_GetTicks();
	deltaTime = 0.001 * (endTime - beginTime);
	beginTime = endTime;

        while (SDL_PollEvent(&e))
	{
	    if (e.type == SDL_QUIT)
	    {
		running = false;
	    }
	    else if (e.type == SDL_KEYDOWN)
	    {
		if (e.key.keysym.sym == SDLK_ESCAPE)
		{
		    running = false;
		}
		else
		{
		    osc1.Trigger(keys.GetNoteFromKeypress(e), 1.0f, e.key.keysym.sym);
		}
	    }
	}

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	for (float i = 0.0f; i <= deltaTime; i += 1.0/44100.0f)
	{
	    while (!as.HasSpaceLeft()) {}
	    as << osc1.GetSample();
	    osc1.NextSample();
	}
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
