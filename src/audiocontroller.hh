#ifndef AUDIOCONTROLLER_H
#define AUDIOCONTROLLER_H

class AudioController
{
public:
    AudioController();

    virtual void Release(int index) = 0;
    virtual void Trigger(float freq, float force, int index) = 0;
    virtual bool Triggered(int index) = 0;

private:
};

#endif
