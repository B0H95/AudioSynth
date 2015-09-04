#ifndef AUDIOCOMPONENT_H
#define AUDIOCOMPONENT_H

#include <algorithm>
#include <map>
#include <vector>

class AudioComponent
{
public:
    AudioComponent();

    virtual float GetSample() = 0;
    virtual void NextSample() = 0;
    virtual void Reset() = 0;

    void AddSource(int sourceIndex, AudioComponent* component);
    void RemoveSource(int sourceIndex, AudioComponent* component);

protected:
    float MixSource(int sourceIndex);
    void SendNextSampleSignal();
    void SendResetSignal();

private:
    std::map<int, std::vector<AudioComponent*>> audioSources;

    bool FindComponent(int sourceIndex, AudioComponent* component);
    bool SourcesHasKey(int sourceIndex);
};

#endif
