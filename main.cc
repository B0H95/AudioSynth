#include <iostream>
#include <SDL2/SDL.h>

#include "audiogenerators.hh"
#include "audiostream.hh"

using namespace std;

int main()
{
    AudioStream as;
    
    float playtime;
    cout << "Input playtime in seconds: ";
    cin >> playtime;
    for (int i=0; i<int(44100.0f*playtime); ++i)
    {
	float n1 = GenerateSineWave(i/44100.0f, 440);
	as << n1;
    }
    
    SDL_Delay(int(playtime*1000.0f));

    return 0;
}
