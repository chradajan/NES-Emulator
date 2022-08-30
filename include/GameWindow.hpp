#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include "NES.hpp"
#include <memory>
#include <string>
#include <SDL2/SDL.h>
#include <array>

constexpr int SCREEN_WIDTH = 256;
constexpr int SCREEN_HEIGHT = 240;
constexpr int CHANNELS = 3;
constexpr int SCREEN_FPS = 60;
constexpr int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

class GameWindow
{
public:
    GameWindow(std::string romPath, std::string romName);
    ~GameWindow();
    void StartEmulator();

private:
    std::unique_ptr<NES> nes;
    SDL_Window* window;
    SDL_Surface* screenSurface;
    SDL_Surface* windowSurface;
    char* frameBuffer;

    void UpdateScreen();
};

#endif