#include "audiostream.hh"

AudioStream::AudioStream() :
    audioSpec(),
    bufferBegin(0),
    bufferEnd(0),
    differenceBeginEnd(0),
    ringBuffer(),
    savedSamples()
{
    SDL_Init(SDL_INIT_AUDIO);

    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_S32SYS;
    audioSpec.samples = 1024;
    audioSpec.callback = AudioCallback;
    audioSpec.channels = 1;
    audioSpec.userdata = (void*)this;

    if (SDL_OpenAudio(&audioSpec, NULL) < 0)
    {
	std::cout << "Could not open audio: " << SDL_GetError() << std::endl;
	exit(1);
    }

    SDL_PauseAudio(0);
}

AudioStream::~AudioStream()
{
    SDL_PauseAudio(1);
    SDL_CloseAudio();
}

void AudioStream::operator<<(float sample)
{
    Sint32 newSample;
    if (sample > -1.0f && sample < 1.0f)
    {
	newSample = Sint32(sample * SINT32_MAX);
    }
    else if (sample >= 1.0f)
    {
	newSample = SINT32_MAX;
    }
    else if (sample <= -1.0f)
    {
	newSample = -SINT32_MAX;
    }
    ringBuffer[bufferEnd] = newSample;
    bufferEnd = (bufferEnd + 1) % BUFFER_SIZE;
    ++differenceBeginEnd;
}

bool AudioStream::HasSpaceLeft()
{
    return differenceBeginEnd < BUFFER_SIZE;
}

// NOT AUDIOSTREAM

void AudioCallback(void *userdata, Uint8 *stream, int len)
{
    SDL_memset(stream, 0, len);
    AudioStream* as = (AudioStream*)userdata; 
    if (as->differenceBeginEnd >= len / 4)
    {
	Uint8* sampleptr = reinterpret_cast<Uint8*>(&(as->ringBuffer[as->bufferBegin]));
	SDL_MixAudio(stream, sampleptr, len, SDL_MIX_MAXVOLUME);
	as->bufferBegin += len / 4;
	as->bufferBegin %= BUFFER_SIZE;
	as->differenceBeginEnd -= len / 4;
    }
}
