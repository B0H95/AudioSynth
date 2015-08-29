#include "audiocomponent.hh"

// PUBLIC

AudioComponent::AudioComponent()
{
}

void AudioComponent::AddSource(int sourceIndex, AudioComponent* component)
{
    if (SourcesHasKey(sourceIndex))
    {
	
    }
    else
    {
    }
}

float AudioComponent::MixSource(int sourceIndex)
{
    return 0.0f;
}

void AudioComponent::RemoveSource(AudioComponent* component)
{
}

void AudioComponent::RemoveSource(int sourceIndex, AudioComponent* component)
{
}

// PRIVATE

bool AudioComponent::SourcesHasKey(int sourceIndex)
{
    if (audioSources.find(sourceIndex) != audioSources.end())
    {
	return true;
    }
    else
    {
	return false;
    }
}
