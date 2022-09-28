#include "../include/GameWindow.hpp"
#include "../include/NesComponent.hpp"
#include "../include/Paths.hpp"
#include <filesystem>
#include <utility>
#include <SDL.h>
#include <SDL_image.h>

void GameWindow::InitializeSDL()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    // Rendering setup
    window_ = SDL_CreateWindow("NES EMU",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               SCREEN_WIDTH * WINDOW_SCALE,
                               SCREEN_HEIGHT * WINDOW_SCALE,
                               SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_SetWindowMinimumSize(window_, SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    UpdateTitle();
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

    std::filesystem::path iconPath = RESOURCES_PATH;
    iconPath += "icon.png";
    SDL_Surface* surface = IMG_Load(iconPath.string().c_str());
    SDL_SetWindowIcon(window_, surface);
    SDL_FreeSurface(surface);

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
    renderThread_ = nullptr;
}

void GameWindow::HandleSDLInputs(SDL_Scancode scancode)
{
    if (!pauseMenuOpen_)
    {
        if (scancode == keyBindings_[InputType::RESET].second)
        {
            resetNES_ = true;
        }
        else if (scancode == keyBindings_[InputType::OVERSCAN].second)
        {
            overscan_ = !overscan_;
            nes_.SetOverscan(overscan_);
        }
        else if (scancode == keyBindings_[InputType::MUTE].second)
        {
            mute_ = !mute_;
        }
        else if (scancode == keyBindings_[InputType::SPEEDDOWN].second)
        {
            UpdateClockMultiplier(false);
        }
        else if (scancode == keyBindings_[InputType::SPEEDUP].second)
        {
            UpdateClockMultiplier(true);
        }
        else
        {
            switch (scancode)
            {
                case SDL_SCANCODE_ESCAPE:
                    pauseMenuOpen_ = true;
                    LockAudio();

                    if ((rightMenuOption_ == RightMenuOption::SAVE) || (rightMenuOption_ == RightMenuOption::LOAD))
                    {
                        LoadSaveStateImages();
                    }
                    break;
                case SDL_SCANCODE_1 ... SDL_SCANCODE_5:
                    saveStateNum_ = static_cast<int>(scancode) - 29;
                    serialize_ = true;
                    break;
                case SDL_SCANCODE_F1 ... SDL_SCANCODE_F5:
                    saveStateNum_ = static_cast<int>(scancode) - 57;
                    deserialize_ = true;
                    break;
                default:
                    break;
            }
        }
    }
    else if (scancode == SDL_SCANCODE_ESCAPE)
    {
        ClosePauseMenu();
    }
    else if (inputToBind_ != InputType::INVALID)
    {
        SetKeyBindings(scancode);
    }
}

void GameWindow::UpdateTitle()
{
    SDL_SetWindowTitle(window_, (std::string("NES EMU - ") + fileName_).c_str());
}

void GameWindow::LockAudio()
{
    SDL_LockAudioDevice(audioDevice_);
    SDL_PauseAudioDevice(audioDevice_, 1);
    SDL_Delay(20);
}

void GameWindow::UnlockAudio()
{
    SDL_UnlockAudioDevice(audioDevice_);
    SDL_PauseAudioDevice(audioDevice_, 0);
}
