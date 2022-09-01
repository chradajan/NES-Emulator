#include "../include/GameWindow.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/NES.hpp"
#include "../include/PPU.hpp"
#include <fstream>
#include <memory>
#include <string>
#include <SDL2/SDL.h>

GameWindow::GameWindow(NES& nes, char* frameBuffer) :
    nes_(nes),
    frameBuffer_(frameBuffer)
{
}

void GameWindow::Run()
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("NES",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH * 2,
                                          SCREEN_HEIGHT * 2,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_SetWindowMinimumSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    bool exit = false;

    while (!exit)
    {
        uint32_t startTime = SDL_GetTicks();
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                exit = true;
            }
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
            {
                exit = true;
            }
        }

        nes_.Run();

        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(frameBuffer_,
                                                        SCREEN_WIDTH,
                                                        SCREEN_HEIGHT,
                                                        DEPTH,
                                                        PITCH,
                                                        0x0000FF,
                                                        0x00FF00,
                                                        0xFF0000,
                                                        0);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);

        SDL_RenderPresent(renderer);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

        uint32_t frameTicks = SDL_GetTicks() - startTime;

        if (frameTicks < SCREEN_TICKS_PER_FRAME)
        {
            SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
