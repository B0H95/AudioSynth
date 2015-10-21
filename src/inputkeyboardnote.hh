#ifndef INPUTKEYBOARDNOTE_H
#define INPUTKEYBOARDNOTE_H

#include <math.h>
#include <SDL2/SDL.h>

class InputKeyboardNote
{
public:
    InputKeyboardNote();

    void DecreaseOctave();
    float GetNoteFromKeypress(SDL_Event e);
    void IncreaseOctave();
    
private:
    const float a;
    int halfStepMod;
};

#endif
