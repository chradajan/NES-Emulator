#include "../include/GameWindow.hpp"
#include "../include/NesComponent.hpp"
#include "../include/Paths.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <SDL.h>
#include <SDL_image.h>
#include "../library/imgui/imgui.h"
#include "../library/imgui/imgui_impl_sdl.h"
#include "../library/imgui/imgui_impl_sdlrenderer.h"
#include "../library/imgui/imfilebrowser.h"

std::unordered_map<GameWindow::ClockMultiplier, std::string> GameWindow::clockMultiplierMap_ = {
    {ClockMultiplier::QUARTER,      "0.25x"},
    {ClockMultiplier::HALF,         "0.50x"},
    {ClockMultiplier::NORMAL,       "1.00x"},
    {ClockMultiplier::DOUBLE,       "2.00x"},
    {ClockMultiplier::QUADRUPLE,    "4.00x"},
};

std::unordered_map<GameWindow::WindowScale, std::string> GameWindow::windowScaleMap_ = {
    {WindowScale::TWO,      "2x"},
    {WindowScale::THREE,    "3x"},
    {WindowScale::FOUR,     "4x"},
    {WindowScale::FIVE,     "5x"},
};

template <typename T>
std::time_t to_time_t(T time)
{
    using namespace std::chrono;
    auto systemClock = time_point_cast<system_clock::duration>(time - T::clock::now() + system_clock::now());
    return system_clock::to_time_t(systemClock);
}

void GameWindow::InitializeImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    smallFont_ = io.Fonts->AddFontFromFileTTF("../library/imgui/fonts/DroidSans.ttf", 16.0);
    mediumFont_ = io.Fonts->AddFontFromFileTTF("../library/imgui/fonts/DroidSans.ttf", 24.0);
    largeFont_ = io.Fonts->AddFontFromFileTTF("../library/imgui/fonts/DroidSans.ttf", 34.0);

    io.FontDefault = largeFont_;

    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer_Init(renderer_);

    fileBrowser_.SetTitle("ROM Select");
    fileBrowser_.SetTypeFilters({".nes"});
    inputToBind_ = InputType::INVALID;
    oldKeyStr_ = "";
}

void GameWindow::OptionsMenu()
{
    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // Left
    {
        ImGui::SetNextWindowSize(ImVec2(halfWindowWidth_, windowHeight_));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        ImGui::SetCursorPosX(buttonXPos_);
        ImGui::SetCursorPosY(buttonYPos_);

        // Resume button
        if (ImGui::Button("Resume", buttonSize_))
        {
            ClosePauseMenu();
        }

        ImGui::SetCursorPosX(buttonXPos_);

        // ROM select button
        if (ImGui::Button("Select ROM", buttonSize_))
        {
            fileBrowser_.Open();
        }

        ImGui::SetCursorPosX(buttonXPos_);

        // Settings button
        if (ImGui::Button("Settings", buttonSize_))
        {
            if (rightMenuOption_ == RightMenuOption::SETTINGS)
            {
                rightMenuOption_ = RightMenuOption::BLANK;
            }
            else
            {
                rightMenuOption_ = RightMenuOption::SETTINGS;
            }
        }

        ImGui::SetCursorPosX(buttonXPos_);

        // Save state button
        if (ImGui::Button("Save State", buttonSize_))
        {
            if (rightMenuOption_ == RightMenuOption::SAVE)
            {
                rightMenuOption_ = RightMenuOption::BLANK;
            }
            else
            {
                rightMenuOption_ = RightMenuOption::SAVE;
                LoadSaveStateImages();
            }
        }

        ImGui::SetCursorPosX(buttonXPos_);

        // Load state button
        if (ImGui::Button("Load State", buttonSize_))
        {
            if (rightMenuOption_ == RightMenuOption::LOAD)
            {
                rightMenuOption_ = RightMenuOption::BLANK;
            }
            else
            {
                rightMenuOption_ = RightMenuOption::LOAD;
                LoadSaveStateImages();
            }
        }

        ImGui::SetCursorPosX(buttonXPos_);

        // Reset button
        if (ImGui::Button("Reset", buttonSize_))
        {
            nes_.Reset();
        }

        ImGui::SetCursorPosX(buttonXPos_);

        // Quit button
        if (ImGui::Button("Quit", buttonSize_))
        {
            exit_ = true;
        }

        ImGui::End();
    }

    ImGui::SetNextWindowSize(ImVec2(halfWindowWidth_, windowHeight_));
    ImGui::SetNextWindowPos(ImVec2(halfWindowWidth_, 0));

    // Right
    {
        ImGuiWindowFlags flags = (ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoMove);
        ImGui::Begin("More Options", nullptr, flags);

        switch (rightMenuOption_)
        {
            case RightMenuOption::SETTINGS:
            {
                // Overscan toggle
                ImGui::Checkbox("Overscan", &overscan_);
                nes_.SetOverscan(overscan_);

                // Mute toggle
                ImGui::Checkbox("Mute audio", &mute_);

                // Key binding
                ImGui::NewLine();
                ImGui::Text("NES Controller");
                ImGui::NewLine();

                ImGui::Text("Up");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::UP].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::UP);
                }

                ImGui::Text("Down");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if(ImGui::Button(keyBindings_[InputType::DOWN].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::DOWN);
                }

                ImGui::Text("Left");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if(ImGui::Button(keyBindings_[InputType::LEFT].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::LEFT);
                }

                ImGui::Text("Right");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::RIGHT].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::RIGHT);
                }

                ImGui::Text("A");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::A].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::A);
                }

                ImGui::Text("B");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::B].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::B);
                }

                ImGui::Text("Start");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::START].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::START);
                }

                ImGui::Text("Select");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::SELECT].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::SELECT);
                }

                ImGui::NewLine();
                ImGui::Text("Other controls");
                ImGui::NewLine();

                ImGui::Text("Mute");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::MUTE].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::MUTE);
                }

                ImGui::Text("Overscan");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::OVERSCAN].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::OVERSCAN);
                }

                ImGui::Text("Reset");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::RESET].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::RESET);
                }

                ImGui::Text("Speed down");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::SPEEDDOWN].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::SPEEDDOWN);
                }

                ImGui::Text("Speed up");
                ImGui::SameLine();
                ImGui::SetCursorPosX(keyBindButtonXPos_);
                if (ImGui::Button(keyBindings_[InputType::SPEEDUP].first.c_str(), keyBindButtonSize_))
                {
                    PrepareForBinding(InputType::SPEEDUP);
                }

                // Window scale arrows
                ImGui::NewLine();
                ImGui::Text("Window Scale");

                if (ImGui::ArrowButton("WindowScaleLeft", ImGuiDir_Left))
                {
                    UpdateWindowSize(false);
                }

                ImGui::SameLine();
                ImGui::Text(windowScaleMap_[windowScale_].c_str());

                ImGui::SameLine();

                if (ImGui::ArrowButton("WindowScaleRight", ImGuiDir_Right))
                {
                    UpdateWindowSize(true);
                }

                // CPU speed arrows
                ImGui::NewLine();
                ImGui::Text("CPU Speed");

                if (ImGui::ArrowButton("CPUSpeedLeft", ImGuiDir_Left))
                {
                    UpdateClockMultiplier(false);
                }

                ImGui::SameLine();
                ImGui::Text(clockMultiplierMap_[clockMultiplier_].c_str());

                ImGui::SameLine();

                if (ImGui::ArrowButton("CPUSpeedRight", ImGuiDir_Right))
                {
                    UpdateClockMultiplier(true);
                }
                break;
            }
            case RightMenuOption::SAVE:
            {
                ImGui::Text("Overwrite save slot:");
                ImGui::SetCursorPosY(imageYPos_);
                int i = 1;

                for (auto [texture, lastModified, valid] : saveStateImages_)
                {
                    std::string timeStr = std::to_string(i) + ". No save data";

                    if (valid)
                    {
                        std::time_t epochTime = to_time_t(lastModified);
                        std::tm* localTime = std::localtime(&epochTime);
                        std::stringstream timeStream;
                        timeStream << i << ". ";

                        timeStream << 1900 + localTime->tm_year << "-";
                        timeStream << std::setw(2) << std::setfill('0') << (1 + localTime->tm_mon) << "-";
                        timeStream << std::setw(2) << std::setfill('0') << localTime->tm_mday << " ";
                        timeStream << std::setw(2) << std::setfill('0') << localTime->tm_hour << ":";
                        timeStream << std::setw(2) << std::setfill('0') << localTime->tm_min << ":";
                        timeStream << std::setw(2) << std::setfill('0') << localTime->tm_sec;

                        timeStr = timeStream.str().c_str();
                    }

                    if (ImGui::ImageButton((ImTextureID)texture, imageSize_))
                    {
                        saveStateNum_ = i;
                        CreateSaveState();
                        LoadSaveStateImages();
                    }

                    ImGui::SameLine();
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (imageSize_.y / 2));
                    ImGui::Text(timeStr.c_str());

                    ++i;
                }
                break;
            }
            case RightMenuOption::LOAD:
            {
                ImGui::Text("Load from save slot:");
                ImGui::SetCursorPosY(imageYPos_);
                int i = 1;

                for (auto [texture, lastModified, valid] : saveStateImages_)
                {
                    if (valid)
                    {
                        std::time_t epochTime = to_time_t(lastModified);
                        std::tm* localTime = std::localtime(&epochTime);
                        std::stringstream timeStream;
                        timeStream << i << ". ";

                        timeStream << 1900 + localTime->tm_year << "-";
                        timeStream << std::setw(2) << std::setfill('0') << (1 + localTime->tm_mon) << "-";
                        timeStream << std::setw(2) << std::setfill('0') << localTime->tm_mday << " ";
                        timeStream << std::setw(2) << std::setfill('0') << localTime->tm_hour << ":";
                        timeStream << std::setw(2) << std::setfill('0') << localTime->tm_min << ":";
                        timeStream << std::setw(2) << std::setfill('0') << localTime->tm_sec;

                        if (ImGui::ImageButton((ImTextureID)texture, imageSize_))
                        {
                            saveStateNum_ = i;
                            LoadSaveState();
                        }

                        ImGui::SameLine();
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (imageSize_.y / 2));
                        ImGui::Text(timeStream.str().c_str());
                    }
                    else
                    {
                        ImGui::Image((ImTextureID)texture, imageSize_);
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (imageSize_.y / 2));
                        ImGui::Text((std::to_string(i) + ". No save data").c_str());
                    }

                    ++i;
                }
                break;
            }
            case RightMenuOption::BLANK:
                break;
        }

        ImGui::End();
    }

    fileBrowser_.Display();

    if (fileBrowser_.HasSelected())
    {
        LoadCartridge(fileBrowser_.GetSelected());
        fileBrowser_.ClearSelected();

        if ((rightMenuOption_ == RightMenuOption::SAVE) || (rightMenuOption_ == RightMenuOption::LOAD))
        {
            LoadSaveStateImages();
        }
    }

    ImGui::Render();
    SDL_RenderClear(renderer_);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer_);
}

void GameWindow::ClosePauseMenu()
{
    if (inputToBind_ != InputType::INVALID)
    {
        keyBindings_[inputToBind_].first = oldKeyStr_;
        inputToBind_ = InputType::INVALID;
    }

    if (nes_.Ready())
    {
        pauseMenuOpen_ = false;
        UnlockAudio();
    }
}

void GameWindow::UpdateWindowSize(bool increase)
{
    int windowScaleInt = static_cast<int>(windowScale_);

    if (increase)
    {
        switch (windowScale_)
        {
            case WindowScale::TWO:
                ImGui::GetIO().FontDefault = mediumFont_;
                ++windowScaleInt;
                break;
            case WindowScale::THREE:
                ImGui::GetIO().FontDefault = largeFont_;
                ++windowScaleInt;
                break;
            case WindowScale::FOUR:
                ++windowScaleInt;
                break;
            case WindowScale::FIVE:
                break;
        }
    }
    else
    {
        switch (windowScale_)
        {
            case WindowScale::TWO:
                break;
            case WindowScale::THREE:
                ImGui::GetIO().FontDefault = smallFont_;
                --windowScaleInt;
                break;
            case WindowScale::FOUR:
                ImGui::GetIO().FontDefault = mediumFont_;
                --windowScaleInt;
                break;
            case WindowScale::FIVE:
                --windowScaleInt;
                break;
        }
    }

    windowScale_ = static_cast<WindowScale>(windowScaleInt);
    SDL_SetWindowSize(window_, SCREEN_WIDTH * windowScaleInt, SCREEN_HEIGHT * windowScaleInt);
}

void GameWindow::ScaleGui()
{
    SDL_GetWindowSize(window_, &windowWidth_, &windowHeight_);

    halfWindowWidth_ = windowWidth_ / 2.0;

    float buttonWidth = halfWindowWidth_ * 0.7;
    float buttonHeight = (float)windowHeight_ / (BUTTONS_COUNT + 2);
    buttonSize_ = ImVec2(buttonWidth, buttonHeight);

    buttonXPos_ = (halfWindowWidth_ / 2.0) - (buttonWidth / 2.0);
    buttonYPos_ = (windowHeight_ - (BUTTONS_COUNT * buttonHeight)) / 2.0;

    fileBrowser_.SetWindowSize(windowWidth_, windowHeight_);
    fileBrowser_.SetWindowPos(0, 0);

    float imageHeight_ = windowHeight_ / 6.0;
    float imageWidth_ = imageHeight_ * ASPECT_RATIO;
    imageSize_ = ImVec2(imageWidth_, imageHeight_);
    imageYPos_ = imageHeight_ / 2.5;

    keyBindButtonSize_ = ImVec2(halfWindowWidth_ / 3.5, windowHeight_ / 25);
    keyBindButtonXPos_ = halfWindowWidth_ / 2.5;
}

void GameWindow::LoadSaveStateImages()
{
    for (auto [texture, timestamp, valid] : saveStateImages_)
    {
        SDL_DestroyTexture(texture);
    }

    for (int i = 1; i <= 5; ++i)
    {
        std::filesystem::path saveStatePath = SAVE_STATE_PATH;
        saveStatePath += romHash_ + "_" + std::to_string(i) + ".png";

        if (std::filesystem::exists(saveStatePath))
        {
            SDL_Surface* surface = IMG_Load(saveStatePath.string().c_str());
            saveStateImages_[i-1] = std::make_tuple(SDL_CreateTextureFromSurface(renderer_, surface),
                                                    std::filesystem::last_write_time(saveStatePath),
                                                    true);
            SDL_FreeSurface(surface);
        }
        else
        {
            saveStatePath = RESOURCES_PATH;
            saveStatePath += "NoSaveState.png";
            SDL_Surface* surface = IMG_Load(saveStatePath.string().c_str());
            saveStateImages_[i-1] = std::make_tuple(SDL_CreateTextureFromSurface(renderer_, surface),
                                                    std::filesystem::file_time_type(),
                                                    false);
            SDL_FreeSurface(surface);
        }
    }
}

void GameWindow::PrepareForBinding(InputType keyToBind)
{
    if (inputToBind_ != InputType::INVALID)
    {
        keyBindings_[inputToBind_].first = oldKeyStr_;
    }

    oldKeyStr_ = keyBindings_[keyToBind].first;
    keyBindings_[keyToBind].first = "...";
    inputToBind_ = keyToBind;
}
