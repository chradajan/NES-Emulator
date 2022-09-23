#include "../include/GameWindow.hpp"
#include "../include/NesComponent.hpp"
#include "../include/Paths.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <SDL.h>
#include "../library/imgui/imgui.h"
#include "../library/imgui/imgui_impl_sdl.h"
#include "../library/imgui/imgui_impl_sdlrenderer.h"
#include "../library/imgui/imfilebrowser.h"
#include "../library/md5/md5.hpp"

static double timePerNesClock = TIME_PER_NES_CLOCK;

GameWindow::GameWindow(NES& nes, uint8_t* frameBuffer, std::filesystem::path romPath) :
    nes_(nes),
    frameBuffer_(frameBuffer),
    fileBrowser_(ImGuiFileBrowserFlags_NoModal)
{
    clockMultiplier_ = ClockMultiplier::NORMAL;
    romHash_ = "";
    fileName_ = "";

    exit_ = false;
    resetNES_ = false;
    serialize_ = false;
    deserialize_ = false;

    LoadCartridge(romPath);

    optionsMenuOpen_ = !nes_.Ready();
    settingsOpen_ = false;
    overscan_ = false;
    mute_ = false;
}

void GameWindow::Run()
{
    InitializeSDL();
    InitializeImGui();
    ScaleGui();

    if (!optionsMenuOpen_)
    {
        SDL_PauseAudioDevice(audioDevice_, 0);
    }

    while (!exit_)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (optionsMenuOpen_)
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
            }

            if (event.type == SDL_QUIT)
            {
                exit_ = true;
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                if ((event.window.event == SDL_WINDOWEVENT_CLOSE) && (event.window.windowID == SDL_GetWindowID(window_)))
                {
                    exit_ = true;
                }
                else if ((event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) || (event.window.event == SDL_WINDOWEVENT_MAXIMIZED))
                {
                    ScaleGui();

                    if (!optionsMenuOpen_)
                    {
                        SDL_PauseAudioDevice(audioDevice_, 1);
                        UpdateScreen(this);
                        SDL_PauseAudioDevice(audioDevice_, 0);
                    }
                    else
                    {
                        SDL_RenderClear(renderer_);
                        SDL_RenderPresent(renderer_);
                    }
                }
            }
            else if (event.type == SDL_DROPFILE)
            {
                if (!optionsMenuOpen_)
                {
                    SDL_PauseAudioDevice(audioDevice_, 1);
                    SDL_ClearQueuedAudio(audioDevice_);
                }

                LoadCartridge(event.drop.file);

                if (!optionsMenuOpen_)
                {
                    SDL_PauseAudioDevice(audioDevice_, 0);
                }
            }
            else if (event.type == SDL_KEYUP)
            {
                HandleSDLInputs(event.key.keysym.scancode);
            }
        }

        if (optionsMenuOpen_)
        {
            OptionsMenu();
        }
        else
        {
            SetControllerInputs();

            if (resetNES_)
            {
                SDL_PauseAudioDevice(audioDevice_, 1);
                SDL_ClearQueuedAudio(audioDevice_);
                nes_.Reset();
                resetNES_ = false;
                SDL_PauseAudioDevice(audioDevice_, 0);
            }
            else if (serialize_)
            {
                SDL_PauseAudioDevice(audioDevice_, 1);
                SDL_ClearQueuedAudio(audioDevice_);
                serialize_ = false;

                if (nes_.Ready())
                {
                    nes_.RunUntilFrameReady();
                    UpdateScreen(this);
                    nes_.RunUntilSerializable();

                    std::filesystem::path saveStatePath = SAVE_STATE_PATH;
                    saveStatePath += romHash_ + "_" + std::to_string(saveStateNum_);
                    std::ofstream saveState(saveStatePath, std::ios::binary);

                    if (!saveState.fail())
                    {
                        nes_.Serialize(saveState);
                    }
                }

                SDL_PauseAudioDevice(audioDevice_, 0);
            }
            else if (deserialize_)
            {
                SDL_PauseAudioDevice(audioDevice_, 1);
                SDL_ClearQueuedAudio(audioDevice_);
                deserialize_ = false;

                if (nes_.Ready())
                {
                    std::filesystem::path saveStatePath = SAVE_STATE_PATH;
                    saveStatePath += romHash_ + "_" + std::to_string(saveStateNum_);
                    std::ifstream saveState(saveStatePath, std::ios::binary);

                    if (!saveState.fail())
                    {
                        nes_.Deserialize(saveState);
                    }
                }

                SDL_PauseAudioDevice(audioDevice_, 0);
            }
        }

        SDL_Delay(1);
    }

    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(imGuiContext_);
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
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

        if (gameWindow->mute_)
        {
            buffer[i] = 0;
        }
        else
        {
            buffer[i] = gameWindow->nes_.GetAudioSample();
        }
    }
}

void GameWindow::LoadCartridge(std::filesystem::path romPath)
{
    romHash_ = "";
    fileName_ = "No ROM loaded";

    if (romPath != "")
    {
        auto romSize = std::filesystem::file_size(romPath);

        // Games should not typically exceed 1MB.
        if (romSize < 5000000)
        {
            std::vector<char> buffer(romSize);
            std::ifstream rom(romPath, std::ios::binary);
            rom.read(buffer.data(), romSize);
            rom.close();
            std::string romHash = md5(std::string(buffer.begin(), buffer.end()));

            std::filesystem::path savePath = SAVE_PATH;
            savePath += romHash + ".sav";

            if (nes_.LoadCartridge(romPath, savePath))
            {
                romHash_ = romHash;
                fileName_ = romPath.stem().string();
            }
        }
    }

    if (window_)
    {
        UpdateTitle();
    }
}

void GameWindow::SetControllerInputs()
{
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
}

void GameWindow::UpdateClockMultiplier(bool increase)
{
    if (increase)
    {
        switch (clockMultiplier_)
        {
            case ClockMultiplier::QUARTER ... ClockMultiplier::DOUBLE:
                clockMultiplier_ = static_cast<ClockMultiplier>(static_cast<int>(clockMultiplier_) + 1);
                timePerNesClock *= 0.5;
                break;
            case ClockMultiplier::QUADRUPLE:
                break;
        }
    }
    else
    {
        switch (clockMultiplier_)
        {
            case ClockMultiplier::HALF ... ClockMultiplier::QUADRUPLE:
                clockMultiplier_ = static_cast<ClockMultiplier>(static_cast<int>(clockMultiplier_) - 1);
                timePerNesClock *= 2;
                break;
            case ClockMultiplier::QUARTER:
                break;
        }
    }
}
