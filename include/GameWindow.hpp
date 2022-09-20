#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include "NES.hpp"
#include <SDL2/SDL.h>
#include <array>
#include <string>
#include <utility>

// Rendering window
constexpr int SCREEN_WIDTH = 256;
constexpr int SCREEN_HEIGHT = 240;
constexpr int CHANNELS = 3;
constexpr int DEPTH = CHANNELS * 8;
constexpr int PITCH = SCREEN_WIDTH * CHANNELS;
constexpr int WINDOW_SCALE = 4;

// Audio
constexpr int AUDIO_SAMPLE_RATE = 44100;
constexpr double TIME_PER_AUDIO_SAMPLE = 1.0 / AUDIO_SAMPLE_RATE;
constexpr int CPU_CLOCK_SPEED = 1789773;
constexpr double TIME_PER_NES_CLOCK = 1.0 / CPU_CLOCK_SPEED;
constexpr int AUDIO_SAMPLE_BUFFER_COUNT = 512;


class GameWindow
{
public:
    GameWindow(NES& nes, char* frameBuffer);
    ~GameWindow() = default;
    void Run();

    // void PlayAudio();

private:
    NES& nes_;

    char* frameBuffer_;
    SDL_Renderer* renderer_;
    void UpdateScreen();

    SDL_AudioDeviceID audioDevice_;
    static void GetAudioSamples(void* userdata, Uint8* stream, int len);
};

#endif
