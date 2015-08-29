#ifndef AUDIOCOMPONENT_H
#define AUDIOCOMPONENT_H

#include <vector>
#include <map>

class AudioComponent
{
public:
    AudioComponent();

    virtual float GetSample() = 0;
    virtual void NextSample() = 0;
    virtual void Reset() = 0;

    void AddSource(int sourceIndex, AudioComponent* component);
    float MixSource(int sourceIndex);
    void RemoveSource(AudioComponent* component);
    void RemoveSource(int sourceIndex, AudioComponent* component);

private:
    std::map<int, std::vector<AudioComponent*>> audioSources;

    bool SourcesHasKey(int sourceIndex);
};

#endif
