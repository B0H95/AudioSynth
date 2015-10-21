#include "inputkeyboardnote.hh"

InputKeyboardNote::InputKeyboardNote() :
    a(1.0594630943592953f),
    halfStepMod(0)
{
}

float InputKeyboardNote::GetNoteFromKeypress(SDL_Event e)
{
    if (e.type == SDL_KEYDOWN)
    {
	switch (e.key.keysym.sym)
	{
	case SDLK_z:
	    return 440.00f * pow(a, halfStepMod - 9);
	case SDLK_s:
	    return 440.00f * pow(a, halfStepMod - 8);
	case SDLK_x:
	    return 440.00f * pow(a, halfStepMod - 7);
	case SDLK_d:
	    return 440.00f * pow(a, halfStepMod - 6);
	case SDLK_c:
	    return 440.00f * pow(a, halfStepMod - 5);
	case SDLK_v:
	    return 440.00f * pow(a, halfStepMod - 4);
	case SDLK_g:
	    return 440.00f * pow(a, halfStepMod - 3);
	case SDLK_b:
	    return 440.00f * pow(a, halfStepMod - 2);
	case SDLK_h:
	    return 440.00f * pow(a, halfStepMod - 1);
	case SDLK_n:
	    return 440.00f * pow(a, halfStepMod);
	case SDLK_j:
	    return 440.00f * pow(a, 1 + halfStepMod);
	case SDLK_m:
	    return 440.00f * pow(a, 2 + halfStepMod);	
	case SDLK_q:
	    return 440.00f * pow(a, 3 + halfStepMod);
	case SDLK_2:
	    return 440.00f * pow(a, 4 + halfStepMod);
	case SDLK_w:
	    return 440.00f * pow(a, 5 + halfStepMod);
	case SDLK_3:
	    return 440.00f * pow(a, 6 + halfStepMod);
	case SDLK_e:
	    return 440.00f * pow(a, 7 + halfStepMod);
	case SDLK_r:
	    return 440.00f * pow(a, 8 + halfStepMod);
	case SDLK_5:
	    return 440.00f * pow(a, 9 + halfStepMod);
	case SDLK_t:
	    return 440.00f * pow(a, 10 + halfStepMod);
	case SDLK_6:
	    return 440.00f * pow(a, 11 + halfStepMod);
	case SDLK_y:
	    return 440.00f * pow(a, 12 + halfStepMod);
	case SDLK_7:
	    return 440.00f * pow(a, 13 + halfStepMod);
	case SDLK_u:
	    return 440.00f * pow(a, 14 + halfStepMod);
	case SDLK_p:
	    halfStepMod += 12;
	    return 0.0f;
	case SDLK_o:
	    halfStepMod -= 12;
	    return 0.0f;
	default:
	    return 0.0f;
	}
    }
    else
    {
	return 0.0f;
    }
}
