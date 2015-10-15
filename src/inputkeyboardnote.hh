#ifndef INPUTKEYBOARDNOTE_H
#define INPUTKEYBOARDNOTE_H

#include <SDL2/SDL.h>

class InputKeyboardNote
{
public:
    InputKeyboardNote();

    float GetNoteFromKeypress(SDL_Event e);
    
private:
};

#endif
