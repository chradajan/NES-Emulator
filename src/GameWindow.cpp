#include "../include/GameWindow.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/NES.hpp"
#include "../include/PPU.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <SDL2/SDL.h>

GameWindow::GameWindow(NES& nes, char* frameBuffer, float* audioBuffer) :
    nes_(nes),
    frameBuffer_(frameBuffer),
    audioBuffer_(audioBuffer)
{

}

void GameWindow::Run()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    // Rendering setup
    SDL_Window* window = SDL_CreateWindow("NES",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH * 4,
                                          SCREEN_HEIGHT * 4,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_SetWindowMinimumSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Audio setup
    SDL_AudioSpec audioSpec;
    SDL_zero(audioSpec);
    audioSpec.freq = 44100;
    audioSpec.format = AUDIO_S32SYS;
    audioSpec.channels = 1;
    audioSpec.samples = AUDIO_SAMPLE_BUFFER_COUNT;
    audioSpec.callback = NULL;

    audioDevice_ = SDL_OpenAudioDevice(NULL, 0, &audioSpec, NULL, 0);
    SDL_PauseAudioDevice(audioDevice_, 0);

    std::function<void()> playAudio = std::bind(&GameWindow::PlayAudio, this);

    bool exit = false;
    bool resetNES = false;
    bool capFramerate = true;
    int saveStateNum;
    bool serialize = false;
    bool deserialize = false;

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
            else if (event.type == SDL_DROPFILE)
            {
                std::filesystem::path romPath = event.drop.file;
                std::filesystem::path savePath = "../saves/";
                savePath += romPath.filename();
                savePath.replace_extension(".sav");
                nes_.LoadCartridge(romPath, savePath);
            }
            else if (event.type == SDL_KEYUP)
            {
                switch (event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_R:
                        resetNES = true;
                        break;
                    case SDL_SCANCODE_T:
                        capFramerate = !capFramerate;
                        break;
                    case SDL_SCANCODE_1 ... SDL_SCANCODE_0:
                        if (event.key.keysym.scancode == SDL_SCANCODE_0)
                        {
                            saveStateNum = 0;
                        }
                        else
                        {
                            saveStateNum = (int)event.key.keysym.scancode - 29;
                        }

                        serialize = true;
                        break;
                    case SDL_SCANCODE_F1 ... SDL_SCANCODE_F10:
                        if (event.key.keysym.scancode == SDL_SCANCODE_F10)
                        {
                            saveStateNum = 0;
                        }
                        else
                        {
                            saveStateNum = (int)event.key.keysym.scancode - 57;
                        }

                        deserialize = true;
                        break;
                    default:
                        break;
                }
            }
        }

        nes_.Run(playAudio);

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

        if (resetNES)
        {
            nes_.Reset();
            resetNES = false;
        }
        else if (serialize)
        {
            serialize = false;

            if (nes_.Ready())
            {
                std::string fileName = nes_.GetFileName();
                nes_.RunUntilSerializable(playAudio);
                std::string path = "../savestates/" + fileName + std::to_string(saveStateNum) + ".sav";
                std::ofstream saveState(path, std::ios::binary);

                if (!saveState.fail())
                {
                    nes_.Serialize(saveState);
                }
            }
        }
        else if (deserialize)
        {
            deserialize = false;

            if (nes_.Ready())
            {
                std::string fileName = nes_.GetFileName();
                std::string path = "../savestates/" + fileName + std::to_string(saveStateNum) + ".sav";
                std::ifstream saveState(path, std::ios::binary);

                if (!saveState.fail())
                {
                    nes_.Deserialize(saveState);
                }
            }
        }

        uint32_t frameTicks = SDL_GetTicks() - startTime;

        if (capFramerate && (frameTicks < SCREEN_TICKS_PER_FRAME))
        {
            SDL_Delay(SCREEN_TICKS_PER_FRAME - frameTicks);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_CloseAudioDevice(audioDevice_);
    SDL_Quit();
}

void GameWindow::PlayAudio()
{
    SDL_QueueAudio(audioDevice_, (void*)audioBuffer_, AUDIO_SAMPLE_BUFFER_SIZE);
}
