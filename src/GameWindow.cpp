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

GameWindow::GameWindow(NES& nes, char* frameBuffer) :
    nes_(nes),
    frameBuffer_(frameBuffer)
{

}

void GameWindow::Run()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    // Rendering setup
    SDL_Window* window = SDL_CreateWindow("NES",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH * WINDOW_SCALE,
                                          SCREEN_HEIGHT * WINDOW_SCALE,
                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_SetWindowMinimumSize(window, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    renderer_ = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Audio setup
    SDL_AudioSpec audioSpec;
    SDL_zero(audioSpec);
    audioSpec.freq = AUDIO_SAMPLE_RATE;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 1;
    audioSpec.samples = AUDIO_SAMPLE_BUFFER_COUNT;
    audioSpec.callback = GameWindow::GetAudioSamples;
    audioSpec.userdata = &nes_;

    audioDevice_ = SDL_OpenAudioDevice(NULL, 0, &audioSpec, NULL, 0);
    SDL_PauseAudioDevice(audioDevice_, 0);

    bool exit = false;
    bool resetNES = false;
    bool capFramerate = true;
    int saveStateNum;
    bool serialize = false;
    bool deserialize = false;

    while (!exit)
    {
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

        if (nes_.FrameReady())
        {
            UpdateScreen();
        }

        if (resetNES)
        {
            nes_.Reset();
            resetNES = false;
        }
        else if (serialize)
        {
            SDL_PauseAudioDevice(audioDevice_, 1);
            SDL_ClearQueuedAudio(audioDevice_);
            serialize = false;

            if (nes_.Ready())
            {
                std::string fileName = nes_.GetFileName();

                nes_.RunUntilFrameReady();
                UpdateScreen();
                nes_.RunUntilSerializable();

                std::string path = "../savestates/" + fileName + std::to_string(saveStateNum) + ".sav";
                std::ofstream saveState(path, std::ios::binary);

                if (!saveState.fail())
                {
                    nes_.Serialize(saveState);
                }
            }

            SDL_PauseAudioDevice(audioDevice_, 0);
        }
        else if (deserialize)
        {
            SDL_PauseAudioDevice(audioDevice_, 1);
            SDL_ClearQueuedAudio(audioDevice_);
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

            SDL_PauseAudioDevice(audioDevice_, 0);
        }

        SDL_Delay(1);
    }

    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window);
    SDL_CloseAudioDevice(audioDevice_);
    SDL_Quit();
}

void GameWindow::UpdateScreen()
{
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(frameBuffer_,
                                                            SCREEN_WIDTH,
                                                            SCREEN_HEIGHT,
                                                            DEPTH,
                                                            PITCH,
                                                            0x0000FF,
                                                            0x00FF00,
                                                            0xFF0000,
                                                            0);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_RenderCopy(renderer_, texture, nullptr, nullptr);

    SDL_RenderPresent(renderer_);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void GameWindow::GetAudioSamples(void* userdata, Uint8* stream, int len)
{
    static double audioTime = 0.0;
    NES* nes = static_cast<NES*>(userdata);
    int numSamples = len / sizeof(int16_t);
    int16_t* buffer = (int16_t*)stream;

    for (int i = 0; i < numSamples; ++i)
    {
        while (audioTime < TIME_PER_AUDIO_SAMPLE)
        {
            nes->Clock();
            audioTime += TIME_PER_NES_CLOCK;
        }

        audioTime -= TIME_PER_AUDIO_SAMPLE;
        buffer[i] = nes->GetAudioSample();
    }
}
