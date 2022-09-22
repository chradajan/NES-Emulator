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

static bool mute = false;
static double timePerNesClock = TIME_PER_NES_CLOCK;

GameWindow::GameWindow(NES& nes, uint8_t* frameBuffer) :
    nes_(nes),
    frameBuffer_(frameBuffer)
{
    clockMultiplier = ClockMultiplier::NORMAL;
    renderThread_ = nullptr;
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
    UpdateScreen(this);

    // Audio setup
    SDL_AudioSpec audioSpec;
    SDL_zero(audioSpec);
    audioSpec.freq = AUDIO_SAMPLE_RATE;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = 1;
    audioSpec.samples = AUDIO_SAMPLE_BUFFER_COUNT;
    audioSpec.callback = GameWindow::GetAudioSamples;
    audioSpec.userdata = this;

    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &audioSpec, nullptr, 0);
    SDL_PauseAudioDevice(audioDevice_, 0);

    bool exit = false;
    bool resetNES = false;
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
            else if (event.type == SDL_WINDOWEVENT)
            {
                if ((event.window.event == SDL_WINDOWEVENT_CLOSE) && (event.window.windowID == SDL_GetWindowID(window)))
                {
                    exit = true;
                }
                else if ((event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) || (event.window.event == SDL_WINDOWEVENT_MAXIMIZED))
                {
                    SDL_PauseAudioDevice(audioDevice_, 1);
                    UpdateScreen(this);
                    SDL_PauseAudioDevice(audioDevice_, 0);
                }
            }
            else if (event.type == SDL_DROPFILE)
            {
                SDL_PauseAudioDevice(audioDevice_, 1);
                SDL_ClearQueuedAudio(audioDevice_);
                std::filesystem::path romPath = event.drop.file;
                std::filesystem::path savePath = "../saves/";
                savePath += romPath.filename();
                savePath.replace_extension(".sav");
                nes_.LoadCartridge(romPath, savePath);
                SDL_PauseAudioDevice(audioDevice_, 0);
            }
            else if (event.type == SDL_KEYUP)
            {
                switch (event.key.keysym.scancode)
                {
                    case SDL_SCANCODE_R:
                        resetNES = true;
                        break;
                    case SDL_SCANCODE_T:
                        nes_.ToggleOverscan();
                        break;
                    case SDL_SCANCODE_M:
                        mute = !mute;
                        break;
                    case SDL_SCANCODE_LEFT:
                        UpdateClockMultiplier(false);
                        break;
                    case SDL_SCANCODE_RIGHT:
                        UpdateClockMultiplier(true);
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

        uint8_t const* keyStates = SDL_GetKeyboardState(nullptr);
        uint8_t controller1 = 0x00;

        controller1 |= keyStates[SDL_SCANCODE_L] ? 0x01 : 0x00; // A
        controller1 |= keyStates[SDL_SCANCODE_K] ? 0x02 : 0x00; // B
        controller1 |= keyStates[SDL_SCANCODE_O] ? 0x04 : 0x00; // SELECT
        controller1 |= keyStates[SDL_SCANCODE_P] ? 0x08 : 0x00; // START
        controller1 |= keyStates[SDL_SCANCODE_W] ? 0x10 : 0x00; // UP
        controller1 |= keyStates[SDL_SCANCODE_S] ? 0x20 : 0x00; // DOWN
        controller1 |= keyStates[SDL_SCANCODE_A] ? 0x40 : 0x00; // LEFT
        controller1 |= keyStates[SDL_SCANCODE_D] ? 0x80 : 0x00; // RIGHT

        nes_.SetControllerInputs(controller1, 0x00);

        if (resetNES)
        {
            SDL_PauseAudioDevice(audioDevice_, 1);
            SDL_ClearQueuedAudio(audioDevice_);
            nes_.Reset();
            resetNES = false;
            SDL_PauseAudioDevice(audioDevice_, 0);
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
                UpdateScreen(this);
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

int GameWindow::UpdateScreen(void* data)
{
    GameWindow* gameWindow = static_cast<GameWindow*>(data);

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(gameWindow->frameBuffer_,
                                                    SCREEN_WIDTH,
                                                    SCREEN_HEIGHT,
                                                    DEPTH,
                                                    PITCH,
                                                    0x0000FF,
                                                    0x00FF00,
                                                    0xFF0000,
                                                    0);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(gameWindow->renderer_, surface);
    SDL_RenderCopy(gameWindow->renderer_, texture, nullptr, nullptr);

    SDL_RenderPresent(gameWindow->renderer_);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    return 0;
}

void GameWindow::GetAudioSamples(void* userdata, Uint8* stream, int len)
{
    static double audioTime = 0.0;
    GameWindow* gameWindow = static_cast<GameWindow*>(userdata);
    int numSamples = len / sizeof(int16_t);
    int16_t* buffer = (int16_t*)stream;

    for (int i = 0; i < numSamples; ++i)
    {
        while (audioTime < TIME_PER_AUDIO_SAMPLE)
        {
            gameWindow->nes_.Clock();
            audioTime += timePerNesClock;

            if (gameWindow->nes_.FrameReady())
            {
                SDL_WaitThread(gameWindow->renderThread_, nullptr);
                gameWindow->renderThread_ = SDL_CreateThread(GameWindow::UpdateScreen, "UpdateScreen", gameWindow);
            }
        }

        audioTime -= TIME_PER_AUDIO_SAMPLE;

        if (mute)
        {
            buffer[i] = 0;
        }
        else
        {
            buffer[i] = gameWindow->nes_.GetAudioSample();
        }
    }
}

void GameWindow::UpdateClockMultiplier(bool increase)
{
    if (increase)
    {
        switch (clockMultiplier)
        {
            case ClockMultiplier::QUARTER ... ClockMultiplier::DOUBLE:
                clockMultiplier = static_cast<ClockMultiplier>(static_cast<int>(clockMultiplier) + 1);
                timePerNesClock *= 0.5;
                break;
            case ClockMultiplier::QUADRUPLE:
                break;
        }
    }
    else
    {
        switch (clockMultiplier)
        {
            case ClockMultiplier::HALF ... ClockMultiplier::QUADRUPLE:
                clockMultiplier = static_cast<ClockMultiplier>(static_cast<int>(clockMultiplier) - 1);
                timePerNesClock *= 2;
                break;
            case ClockMultiplier::QUARTER:
                break;
        }
    }
}
