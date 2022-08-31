#include "../include/GameWindow.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/NES.hpp"
#include "../include/PPU.hpp"
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <SDL2/SDL.h>

GameWindow::GameWindow(std::string const romPath, std::string const romName)
{
    std::string savePath = "../saves/" + romName + ".sav";
    nes = std::make_unique<NES>(romPath, savePath, frameBuffer.data());
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}

GameWindow::~GameWindow()
{
    SDL_DestroyWindow(window);
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
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(frameBuffer.data(), SCREEN_WIDTH, SCREEN_HEIGHT, DEPTH, PITCH, 0x0000FF, 0x00FF00, 0xFF0000, 0);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);
}
