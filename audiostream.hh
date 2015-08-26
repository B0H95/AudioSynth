#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <iostream>
#include <vector>
#include <SDL2/SDL.h>

#define SINT32_MAX 2147483647

class AudioStream
{
public:
    AudioStream();
    ~AudioStream();
    void operator<<(float sample);
    
private:
    SDL_AudioSpec audioSpec;
    std::vector<Sint32> sampleList;
};

void AudioCallback(void *userdata, Uint8 *stream, int len);

#endif
