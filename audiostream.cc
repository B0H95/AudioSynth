#include "audiostream.hh"

AudioStream::AudioStream() :
    audioSpec(),
    sampleList()
{
    SDL_Init(SDL_INIT_AUDIO);

    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_S32SYS;
    audioSpec.samples = 1024;
    audioSpec.callback = AudioCallback;
    audioSpec.channels = 1;
    audioSpec.userdata = (void*)&sampleList;

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
    sampleList.push_back(newSample);
}

// NOT AUDIOSTREAM

void AudioCallback(void *userdata, Uint8 *stream, int len)
{
    SDL_memset(stream, 0, len);
    std::vector<Sint32>* samples = (std::vector<Sint32>*)userdata; 
    if (samples->size() >= len / 4)
    {
	Uint8* sampleptr = reinterpret_cast<Uint8*>(&(*samples)[0]);
	SDL_MixAudio(stream, sampleptr, len, SDL_MIX_MAXVOLUME);
	samples->erase(samples->begin(), samples->begin() + (len / 4));
    }
}
