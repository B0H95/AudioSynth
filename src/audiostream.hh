#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <iostream>
#include <vector>
#include <SDL2/SDL.h>

#define SINT32_MAX 2147483647
#define BUFFER_SIZE 32768

class AudioStream
{
public:
    AudioStream();
    ~AudioStream();
    void operator<<(float sample);
    
    bool HasSpaceLeft();

    friend void AudioCallback(void *userdata, Uint8 *stream, int len);

private:
    SDL_AudioSpec audioSpec;
    int bufferBegin;
    int bufferEnd;
    int differenceBeginEnd;
    Sint32 ringBuffer[BUFFER_SIZE];
    std::vector<Sint32> savedSamples;
    
};

void AudioCallback(void *userdata, Uint8 *stream, int len);

#endif
