#include "../include/GameWindow.hpp"
#include "../include/NesComponent.hpp"
#include <SDL.h>

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

void GameWindow::HandleSDLInputs(SDL_Scancode key)
{
    if (!pauseMenuOpen_)
    {
        switch (key)
        {
            case SDL_SCANCODE_R:
                resetNES_ = true;
                break;
            case SDL_SCANCODE_T:
                overscan_ = !overscan_;
                nes_.SetOverscan(overscan_);
                break;
            case SDL_SCANCODE_M:
                mute_ = !mute_;
                break;
            case SDL_SCANCODE_ESCAPE:
                pauseMenuOpen_ = true;
                LockAudio();

                if ((rightMenuOption_ == RightMenuOption::SAVE) || (rightMenuOption_ == RightMenuOption::LOAD))
                {
                    LoadSaveStateImages();
                }
                break;
            case SDL_SCANCODE_LEFT:
                UpdateClockMultiplier(false);
                break;
            case SDL_SCANCODE_RIGHT:
                UpdateClockMultiplier(true);
                break;
            case SDL_SCANCODE_1 ... SDL_SCANCODE_5:
                saveStateNum_ = (int)key - 29;
                serialize_ = true;
                break;
            case SDL_SCANCODE_F1 ... SDL_SCANCODE_F5:
                saveStateNum_ = (int)key - 57;
                deserialize_ = true;
                break;
            default:
                break;
        }
    }
    else if (key == SDL_SCANCODE_ESCAPE)
    {
        ClosePauseMenu();
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
