#include <SDL2/SDL.h>
#include <stdio.h>

#include "io/audio.h"

#include <math.h>
void test_callback(float* buffer, unsigned int len, void* data)
{
    (void)(data);
    
    unsigned int i = 0;
    while (i < len)
    {
        buffer[i] = sinf((float)i * 6.28f * 8.0f / (float)len);
        ++i;
        buffer[i] = sinf((float)i * 6.28f * 4.0f / (float)len);
        ++i;
    }
}

int main(int argc, char** argv)
{
    (void)(argc);
    (void)(argv);
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        return 1;
    }
    
    const int SCREEN_WIDTH = 640;
    const int SCREEN_HEIGHT = 480;

    SDL_Window* window = SDL_CreateWindow("AudioSynth",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH,
                                          SCREEN_HEIGHT,
                                          SDL_WINDOW_RESIZABLE);
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!start_audio_output(test_callback, NULL))
    {
        return 1;
    }
    
    int running = 1;
    SDL_Event e;

    while (running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                running = 0;
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = 0;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    stop_audio_output();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
