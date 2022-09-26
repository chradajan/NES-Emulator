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
    imGuiContext_ = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    io.Fonts->AddFontFromFileTTF("../library/imgui/fonts/DroidSans.ttf", 34.0);

    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer_Init(renderer_);

    fileBrowser_.SetTitle("ROM Select");
    fileBrowser_.SetTypeFilters({".nes"});
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

        if (ImGui::Button("Continue", buttonSize_))
        {
            ClosePauseMenu();
        }

        ImGui::SetCursorPosX(buttonXPos_);

        if (ImGui::Button("Select ROM", buttonSize_))
        {
            fileBrowser_.Open();
        }

        ImGui::SetCursorPosX(buttonXPos_);

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

        if (ImGui::Button("Reset", buttonSize_))
        {
            nes_.Reset();
        }

        ImGui::SetCursorPosX(buttonXPos_);

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
        ImGui::Begin("More Options", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        switch (rightMenuOption_)
        {
            case RightMenuOption::SETTINGS:
            {
                ImGui::Checkbox("Overscan", &overscan_);
                nes_.SetOverscan(overscan_);

                ImGui::Checkbox("Mute audio", &mute_);

                ImGui::NewLine();
                ImGui::Text("CPU Speed");

                if (ImGui::ArrowButton("left", ImGuiDir_Left))
                {
                    UpdateClockMultiplier(false);
                }

                ImGui::SameLine();
                ImGui::Text(clockMultiplierMap_[clockMultiplier_].c_str());

                ImGui::SameLine();

                if (ImGui::ArrowButton("right", ImGuiDir_Right))
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
    if (nes_.Ready())
    {
        pauseMenuOpen_ = false;
        UnlockAudio();
    }
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
