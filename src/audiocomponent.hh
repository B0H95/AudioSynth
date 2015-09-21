#ifndef AUDIOCOMPONENT_H
#define AUDIOCOMPONENT_H

#include <map>
#include <set>

class AudioComponent
{
public:
    AudioComponent();

    virtual float GetSample() = 0;
    virtual void NextSample() = 0;
    virtual void Reset() = 0;

    void RemoveSource(int sourceIndex);
    void SetSource(int sourceIndex, AudioComponent* component);

protected:
    std::set<int> const& GetAvailableSources();
    float MixSource(int sourceIndex);
    void SendNextSampleSignal();
    void SendResetSignal();

private:
    std::set<int> availableSources;
    std::map<int, AudioComponent*> audioSources;
};

#endif
