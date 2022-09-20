#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include "NES.hpp"
#include <SDL2/SDL.h>
#include <array>
#include <string>
#include <utility>

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
    GameWindow(NES& nes, char* frameBuffer, int16_t* audioBuffer);
    ~GameWindow() = default;
    void Run();

    void PlayAudio();

private:
    NES& nes_;
    char* frameBuffer_;
    int16_t* audioBuffer_;
    SDL_AudioDeviceID audioDevice_;
};

#endif
