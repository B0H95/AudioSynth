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
	float n1 = GenerateSawWave(i/44100.0f, 36.71f, true);
	float n2 = GenerateSawWave(i/44100.0f, 36.96f, true);
	float n3 = GenerateSawWave(i/44100.0f, 36.46f, true);
	as << (n1 + n2 + n3) / 3.0f;
    }
    
    SDL_Delay(int(playtime*1000.0f));

    return 0;
}
