#include "../include/GameWindow.hpp"
#include "../include/NesComponent.hpp"
#include <unordered_map>
#include <SDL.h>
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

void GameWindow::InitializeImGui()
{
    IMGUI_CHECKVERSION();
    imGuiContext_ = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    io.Fonts->AddFontFromFileTTF("../library/imgui/fonts/DroidSans.ttf", 36.0);

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
            CloseOptionsMenu();
        }

        ImGui::SetCursorPosX(buttonXPos_);

        if (ImGui::Button("Select ROM", buttonSize_))
        {
            fileBrowser_.Open();
        }

        ImGui::SetCursorPosX(buttonXPos_);

        if (ImGui::Button("Settings", buttonSize_))
        {
            settingsOpen_ = !settingsOpen_;
        }

        ImGui::SetCursorPosX(buttonXPos_);

        if (ImGui::Button("Save State", buttonSize_))
        {
            // TODO
        }

        ImGui::SetCursorPosX(buttonXPos_);

        if (ImGui::Button("Load State", buttonSize_))
        {
            // TODO
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

    // Right
    {
        ImGui::SetNextWindowSize(ImVec2(halfWindowWidth_, windowHeight_));
        ImGui::SetNextWindowPos(ImVec2(halfWindowWidth_, 0));
        ImGui::Begin("More Options", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

        if (settingsOpen_)
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
        }

        ImGui::End();
    }

    fileBrowser_.Display();

    if (fileBrowser_.HasSelected())
    {
        LoadCartridge(fileBrowser_.GetSelected());
        fileBrowser_.ClearSelected();
    }

    ImGui::Render();
    SDL_RenderClear(renderer_);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer_);
}

void GameWindow::CloseOptionsMenu()
{
    if (nes_.Ready())
    {
        optionsMenuOpen_ = false;
        SDL_PauseAudioDevice(audioDevice_, 0);
    }
}

void GameWindow::ScaleGui()
{
    SDL_GetWindowSize(window_, &windowWidth_, &windowHeight_);

    halfWindowWidth_ = windowWidth_ / 2;

    float buttonWidth = halfWindowWidth_ * 0.7;
    float buttonHeight = windowHeight_ / (BUTTONS_COUNT + 2);
    buttonSize_ = ImVec2(buttonWidth, buttonHeight);

    buttonXPos_ = (halfWindowWidth_ / 2) - (buttonWidth / 2);
    buttonYPos_ = (windowHeight_ - (BUTTONS_COUNT * buttonHeight)) / 2;

    fileBrowser_.SetWindowSize(windowWidth_, windowHeight_);
    fileBrowser_.SetWindowPos(0, 0);
}
