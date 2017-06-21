#include "io/audio.h"

#include <stddef.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#define CHANNEL_COUNT 2
#define BUFFER_LENGTH 512
#define BUFFER_SIZE CHANNEL_COUNT*BUFFER_LENGTH
#define SINT32_MAX 2147483647

typedef struct callback_data
{
    void (*callback)(float*, unsigned int, void*);
    void* custom_data;
} callback_data;

static float buffer[BUFFER_SIZE];
static Sint32 output_buffer[BUFFER_SIZE];
static callback_data cbdata;

static void audio_output_callback(void *userdata, Uint8* stream, int len)
{
    (void)(userdata);
    
    cbdata.callback(buffer, BUFFER_SIZE, cbdata.custom_data);
    for (unsigned int i = 0; i < BUFFER_SIZE; ++i)
    {
        output_buffer[i] = buffer[i] * SINT32_MAX;
    }
    
    SDL_memset(stream, 0, len);

    Uint8* sampleptr = (Uint8*)output_buffer;
    SDL_MixAudio(stream, sampleptr, len, SDL_MIX_MAXVOLUME);
}

bool start_audio_output(void callback(float*, unsigned int, void*), void* data)
{
    cbdata.callback = callback;
    cbdata.custom_data = data;
    
    SDL_AudioSpec settings;
    settings.freq = 44100;
    settings.format = AUDIO_S32SYS;
    settings.samples = BUFFER_LENGTH;
    settings.callback = audio_output_callback;
    settings.channels = CHANNEL_COUNT;
    settings.userdata = &cbdata;
    
    SDL_AudioSpec wanted_settings = settings;
    
    if (SDL_OpenAudio(&settings, NULL) < 0)
    {
        return false;
    }

    if (wanted_settings.freq != settings.freq ||
        wanted_settings.format != settings.format ||
        wanted_settings.samples != settings.samples ||
        wanted_settings.callback != settings.callback ||
        wanted_settings.channels != settings.channels ||
        wanted_settings.userdata != settings.userdata)
    {
        SDL_CloseAudio();
        return false;
    }

    SDL_PauseAudio(0);
    
    return true;
}

void stop_audio_output()
{
    SDL_PauseAudio(1);
    SDL_CloseAudio();
}
