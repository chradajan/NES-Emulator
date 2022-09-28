#include "../include/GameWindow.hpp"
#include "../include/NesComponent.hpp"
#include "../include/Paths.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <SDL.h>
#include <SDL_image.h>
#include "../library/imgui/imgui.h"
#include "../library/imgui/imgui_impl_sdl.h"
#include "../library/imgui/imgui_impl_sdlrenderer.h"
#include "../library/imgui/imfilebrowser.h"
#include "../library/md5/md5.hpp"

static double timePerNesClock = TIME_PER_NES_CLOCK;

GameWindow::GameWindow(NES& nes, uint8_t* frameBuffer, std::filesystem::path romPath) :
    nes_(nes),
    frameBuffer_(frameBuffer)
{
    clockMultiplier_ = ClockMultiplier::NORMAL;
    romHash_ = "";
    fileName_ = "";

    exit_ = false;
    resetNES_ = false;
    serialize_ = false;
    deserialize_ = false;

    LoadCartridge(romPath);

    pauseMenuOpen_ = !nes_.Ready();
    rightMenuOption_ = RightMenuOption::BLANK;
    overscan_ = false;
    mute_ = false;
    windowScale_ = static_cast<WindowScale>(WINDOW_SCALE);

    LoadKeyBindings();
}

void GameWindow::Run()
{
    InitializeSDL();
    InitializeImGui();
    ScaleGui();

    if (!pauseMenuOpen_)
    {
        SDL_PauseAudioDevice(audioDevice_, 0);
    }

    while (!exit_)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (pauseMenuOpen_)
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
                else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    ScaleGui();
                    SDL_RenderClear(renderer_);
                    SDL_RenderPresent(renderer_);
                }
            }
            else if (event.type == SDL_DROPFILE)
            {
                if (!pauseMenuOpen_)
                {
                    LockAudio();
                }

                LoadCartridge(event.drop.file);

                if (!pauseMenuOpen_)
                {
                    UnlockAudio();
                }
            }
            else if (event.type == SDL_KEYUP)
            {
                HandleSDLInputs(event.key.keysym.scancode);
            }
        }

        if (pauseMenuOpen_)
        {
            OptionsMenu();
        }
        else
        {
            SetControllerInputs();

            if (resetNES_)
            {
                LockAudio();
                nes_.Reset();
                resetNES_ = false;
                UnlockAudio();
            }
            else if (serialize_)
            {
                serialize_ = false;
                LockAudio();
                CreateSaveState();
                UnlockAudio();
            }
            else if (deserialize_)
            {
                deserialize_ = false;
                LockAudio();
                LoadSaveState();
                UnlockAudio();
            }
        }

        SDL_Delay(1);
    }

    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
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

    controller1 |= keyStates[keyBindings_[InputType::A].second] ? 0x01 : 0x00;
    controller1 |= keyStates[keyBindings_[InputType::B].second] ? 0x02 : 0x00;
    controller1 |= keyStates[keyBindings_[InputType::SELECT].second] ? 0x04 : 0x00;
    controller1 |= keyStates[keyBindings_[InputType::START].second] ? 0x08 : 0x00;
    controller1 |= keyStates[keyBindings_[InputType::UP].second] ? 0x10 : 0x00;
    controller1 |= keyStates[keyBindings_[InputType::DOWN].second] ? 0x20 : 0x00;
    controller1 |= keyStates[keyBindings_[InputType::LEFT].second] ? 0x40 : 0x00;
    controller1 |= keyStates[keyBindings_[InputType::RIGHT].second] ? 0x80 : 0x00;

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

void GameWindow::CreateSaveState()
{
    if (nes_.Ready())
    {
        nes_.RunUntilFrameReady();
        nes_.RunUntilSerializable();

        std::filesystem::path saveStatePath = SAVE_STATE_PATH;
        saveStatePath += romHash_ + "_" + std::to_string(saveStateNum_);
        std::ofstream saveState(saveStatePath, std::ios::binary);

        SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(frameBuffer_,
                                                        SCREEN_WIDTH,
                                                        SCREEN_HEIGHT,
                                                        DEPTH,
                                                        PITCH,
                                                        0x0000FF,
                                                        0x00FF00,
                                                        0xFF0000,
                                                        0);

        saveStatePath.replace_extension(".png");
        IMG_SavePNG(surface, saveStatePath.string().c_str());
        SDL_FreeSurface(surface);

        if (!saveState.fail())
        {
            nes_.Serialize(saveState);
        }
    }
}

void GameWindow::LoadSaveState()
{
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
}

void GameWindow::LoadKeyBindings()
{
    inputToBind_ = InputType::INVALID;
    std::ifstream keyBindingsFile(KEY_BINDINGS_PATH);

    if (keyBindingsFile.fail())
    {
        SaveKeyBindings(true);
        keyBindingsFile = std::ifstream(KEY_BINDINGS_PATH);
    }

    for (auto [inputType, inputStr, inputInt] : INPUT_DATA)
    {
        (void)inputStr; (void)inputInt;
        std::string scancodeName, scancodeStr;
        SDL_Scancode scancode;
        keyBindingsFile >> scancodeName >> scancodeStr;
        scancode = static_cast<SDL_Scancode>(std::stoi(scancodeStr));
        scancodeName = SDL_GetScancodeName(scancode);

        if (scancodeName.empty())
        {
            scancodeName = "NOT SET";
        }

        keyBindings_[inputType] = std::make_pair(scancodeName, scancode);
    }

    for (auto [inputType, inputInfo] : keyBindings_)
    {
        reverseKeyBindings_[inputInfo.second] = inputType;
    }
}

void GameWindow::SaveKeyBindings(bool restoreDefaults)
{
    std::ofstream keyBindingsFile(KEY_BINDINGS_PATH);

    for (auto [inputType, inputStr, inputInt] : INPUT_DATA)
    {
        if (!restoreDefaults)
        {
            inputInt = static_cast<int>(keyBindings_[inputType].second);
        }

        keyBindingsFile << inputStr << inputInt << "\n";
    }
}

void GameWindow::SetKeyBindings(SDL_Scancode scancode)
{
    if (((scancode >= SDL_SCANCODE_1) && (scancode <= SDL_SCANCODE_5)) ||
        ((scancode >= SDL_SCANCODE_F1) && (scancode <= SDL_SCANCODE_F5)))
    {
        return;
    }

    if (keyBindings_[inputToBind_].second == scancode)
    {
        keyBindings_[inputToBind_].first = oldKeyStr_;
        inputToBind_ = InputType::INVALID;
        return;
    }

    if (reverseKeyBindings_.count(scancode) == 1)
    {
        keyBindings_[reverseKeyBindings_[scancode]] = std::make_pair("NOT SET", SDL_SCANCODE_UNKNOWN);
    }

    reverseKeyBindings_.erase(keyBindings_[inputToBind_].second);
    reverseKeyBindings_[scancode] = inputToBind_;
    keyBindings_[inputToBind_] = std::make_pair(SDL_GetScancodeName(scancode), scancode);
    inputToBind_ = InputType::INVALID;
    SaveKeyBindings(false);
}

void GameWindow::PrepareForKeyBinding(InputType keyToBind)
{
    if (inputToBind_ != InputType::INVALID)
    {
        keyBindings_[inputToBind_].first = oldKeyStr_;
    }

    oldKeyStr_ = keyBindings_[keyToBind].first;
    keyBindings_[keyToBind].first = "...";
    inputToBind_ = keyToBind;
}
