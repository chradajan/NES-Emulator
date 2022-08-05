#include "../include/GameWindow.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/NES.hpp"
#include "../include/PPU.hpp"
#include <memory>
#include <string>
#include <SDL2/SDL.h>

GameWindow::GameWindow(std::string const romPath)
{
    nes = std::make_unique<NES>(romPath);
    frameBuffer = nes->GetFrameBuffer();
    screenSurface = SDL_CreateRGBSurfaceFrom((void*)frameBuffer, SCREEN_WIDTH, SCREEN_HEIGHT, CHANNELS * 8, SCREEN_WIDTH * CHANNELS, 0x0000FF, 0x00FF00, 0xFF0000, 0);
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    windowSurface = SDL_GetWindowSurface(window);
}

void GameWindow::StartEmulator()
{
    uint64_t startTime;
    uint64_t frameTicks;
    bool quit = false;
    SDL_Event e;

    while (!quit)
    {
        startTime = SDL_GetTicks();

        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
        }

        nes->Run();
        frameTicks = SDL_GetTicks() - startTime;

        if (frameTicks < SCREEN_TICKS_PER_FRAME)
        {
            SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
        }

        UpdateScreen();
    }
}

void GameWindow::UpdateScreen()
{
    SDL_BlitSurface(screenSurface, NULL, windowSurface, NULL);
    SDL_UpdateWindowSurface(window);
}
