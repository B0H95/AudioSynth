#include "audiocomponent.hh"

// PUBLIC

AudioComponent::AudioComponent()
{
}

void AudioComponent::AddSource(int sourceIndex, AudioComponent* component)
{
    if (SourcesHasKey(sourceIndex))
    {
	if (!FindComponent(sourceIndex, component))
	{
	    audioSources[sourceIndex].push_back(component);
	}
    }
    else
    {
	audioSources[sourceIndex] == std::vector<AudioComponent*>(1, component);
    }
}

void AudioComponent::RemoveSource(int sourceIndex, AudioComponent* component)
{
    if (SourcesHasKey(sourceIndex) && FindComponent(sourceIndex, component))
    {
	std::vector<AudioComponent*>::iterator itBegin = audioSources[sourceIndex].begin();
	std::vector<AudioComponent*>::iterator itEnd = audioSources[sourceIndex].end();
	audioSources[sourceIndex].erase(std::remove(itBegin, itEnd, component), itEnd);
    }
}

// PROTECTED

float AudioComponent::MixSource(int sourceIndex)
{
    if (SourcesHasKey(sourceIndex))
    {
	float mix = 0.0f;
	for (AudioComponent*& component : audioSources[sourceIndex])
	{
	    mix += component->GetSample();
	}
	return mix;
    }
    else
    {
	return 0.0f;
    }
}

void AudioComponent::SendNextSampleSignal()
{
    for (auto& audioSource : audioSources)
    {
	for (AudioComponent*& component : audioSource.second)
	{
	    component->NextSample();
	}
    }
}

void AudioComponent::SendResetSignal()
{
    for (auto& audioSource : audioSources)
    {
	for (AudioComponent*& component : audioSource.second)
	{
	    component->Reset();
	}
    }
}

// PRIVATE

bool AudioComponent::FindComponent(int sourceIndex, AudioComponent* component)
{
    return (std::find(audioSources[sourceIndex].begin(), audioSources[sourceIndex].end(), component) == audioSources[sourceIndex].end());
}

bool AudioComponent::SourcesHasKey(int sourceIndex)
{
    return (audioSources.find(sourceIndex) != audioSources.end());
}
