#include "audiocomponent.hh"

// PUBLIC

AudioComponent::AudioComponent() :
    availableSources(),
    audioSources()
{
}

void AudioComponent::RemoveSource(int sourceIndex)
{
    audioSources.erase(sourceIndex);
    availableSources.erase(sourceIndex);
}

void AudioComponent::SetSource(int sourceIndex, AudioComponent* component)
{
    audioSources[sourceIndex] = component;
    availableSources.insert(sourceIndex);
}

// PROTECTED

std::set<int> const& AudioComponent::GetAvailableSources()
{
    return availableSources;
}

float AudioComponent::MixSource(int sourceIndex)
{
    if (audioSources.find(sourceIndex) != audioSources.end())
    {
	return audioSources[sourceIndex]->GetSample();
    }
    return 0.0f;
}

void AudioComponent::SendNextSampleSignal()
{
    for (auto& audioSource : audioSources)
    {
	audioSource.second->NextSample();
    }
}

void AudioComponent::SendResetSignal()
{
    for (auto& audioSource : audioSources)
    {
	audioSource.second->Reset();
    }
}
