#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include "NES.hpp"
#include <SDL2/SDL.h>
#include <array>
#include <memory>
#include <string>

constexpr int SCREEN_WIDTH = 256;
constexpr int SCREEN_HEIGHT = 240;
constexpr int CHANNELS = 3;
constexpr int DEPTH = CHANNELS * 8;
constexpr int PITCH = SCREEN_WIDTH * CHANNELS;
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
    SDL_Renderer* renderer;
    std::array<char, SCREEN_WIDTH * SCREEN_HEIGHT * CHANNELS> frameBuffer;

    void UpdateScreen();
};

#endif