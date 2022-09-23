#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include <array>
#include <filesystem>
#include <string>
#include <utility>
#include <SDL.h>
#include "../library/imgui/imgui.h"
#include "../library/imgui/imfilebrowser.h"

// Rendering window
constexpr int SCREEN_WIDTH = 256;
constexpr int SCREEN_HEIGHT = 240;
constexpr int CHANNELS = 3;
constexpr int DEPTH = CHANNELS * 8;
constexpr int PITCH = SCREEN_WIDTH * CHANNELS;
constexpr int WINDOW_SCALE = 4;

// Audio
constexpr int AUDIO_SAMPLE_RATE = 44100;
constexpr double TIME_PER_AUDIO_SAMPLE = 1.0 / AUDIO_SAMPLE_RATE;
constexpr int CPU_CLOCK_SPEED = 1789773;
constexpr double TIME_PER_NES_CLOCK = 1.0 / CPU_CLOCK_SPEED;
constexpr int AUDIO_SAMPLE_BUFFER_COUNT = 256;

// GUI
constexpr int BUTTONS_COUNT = 7;

class NES;

class GameWindow
{
public:
    GameWindow(NES& nes, uint8_t* frameBuffer, std::filesystem::path romPath = "");
    ~GameWindow() = default;

    void Run();

// NES
private:
    NES& nes_;
    uint8_t* frameBuffer_;
    std::string romHash_;
    std::string fileName_;

    enum ClockMultiplier { QUARTER = 0, HALF, NORMAL, DOUBLE, QUADRUPLE };
    ClockMultiplier clockMultiplier_;

    void LoadCartridge(std::filesystem::path romPath);
    void SetControllerInputs();
    void UpdateClockMultiplier(bool increase);

// SDL Components
private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_AudioDeviceID audioDevice_;
    SDL_Thread* renderThread_;

// SDL helpers
private:
    void InitializeSDL();
    void HandleSDLInputs(SDL_Scancode key);
    void UpdateTitle();
    static int UpdateScreen(void* data);
    static void GetAudioSamples(void* userdata, Uint8* stream, int len);

// Main loop
private:
    bool exit_;
    bool resetNES_;
    int saveStateNum_;
    bool serialize_;
    bool deserialize_;

// ImGui
private:
    ImGuiContext* imGuiContext_;
    ImGui::FileBrowser fileBrowser_;

    int windowWidth_;
    int windowHeight_;
    float halfWindowWidth_;
    float buttonXPos_;
    float buttonYPos_;
    ImVec2 buttonSize_;

// Options menu
private:
    bool optionsMenuOpen_;
    bool settingsOpen_;
    bool overscan_;
    bool mute_;

private:
    void InitializeImGui();
    void OptionsMenu();
    void CloseOptionsMenu();
    void ScaleGui();
};

#endif
