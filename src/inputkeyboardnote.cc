#include "inputkeyboardnote.hh"

InputKeyboardNote::InputKeyboardNote()
{
}

float InputKeyboardNote::GetNoteFromKeypress(SDL_Event e)
{
    if (e.type == SDL_KEYDOWN)
    {
	switch (e.key.keysym.sym)
	{
	case SDLK_z:
	    return 261.63f;
	case SDLK_s:
	    return 277.18f;
	case SDLK_x:
	    return 293.66f;
	case SDLK_d:
	    return 311.13f;
	case SDLK_c:
	    return 329.63f;
	case SDLK_v:
	    return 349.23f;
	case SDLK_g:
	    return 369.99f;
	case SDLK_b:
	    return 392.00f;
	case SDLK_h:
	    return 415.30f;
	case SDLK_n:
	    return 440.00f;
	case SDLK_j:
	    return 466.16f;
	case SDLK_m:
	    return 493.88f;	
	case SDLK_q:
	    return 523.25f;
	case SDLK_2:
	    return 554.37f;
	case SDLK_w:
	    return 587.33f;
	case SDLK_3:
	    return 622.25f;
	case SDLK_e:
	    return 659.25f;
	case SDLK_r:
	    return 698.46f;
	case SDLK_5:
	    return 739.99f;
	case SDLK_t:
	    return 783.99f;
	case SDLK_6:
	    return 830.61f;
	case SDLK_y:
	    return 880.00f;
	case SDLK_7:
	    return 932.33f;
	case SDLK_u:
	    return 987.77f;
	default:
	    return 0.0f;
	}
    }
    else
    {
	return 0.0f;
    }
}
