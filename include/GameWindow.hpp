#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <tuple>
#include <utility>
#include <SDL.h>
#include "../library/DearImGui/imgui.h"
#include "../library/DearImGui/imfilebrowser.h"

// Rendering window
constexpr int SCREEN_WIDTH = 256;
constexpr int SCREEN_HEIGHT = 240;
constexpr int CHANNELS = 3;
constexpr int DEPTH = CHANNELS * 8;
constexpr int PITCH = SCREEN_WIDTH * CHANNELS;
constexpr int WINDOW_SCALE = 3;
constexpr float ASPECT_RATIO = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;

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
    static std::unordered_map<ClockMultiplier, std::string> clockMultiplierMap_;

    ClockMultiplier clockMultiplier_;

    void LoadCartridge(std::filesystem::path romPath);
    void SetControllerInputs();
    void UpdateClockMultiplier(bool increase);

// Save states
private:
    void CreateSaveState();
    void LoadSaveState();

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
    void LockAudio();
    void UnlockAudio();
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
    ImGui::FileBrowser fileBrowser_;

    std::array<std::tuple<SDL_Texture*, std::filesystem::file_time_type, bool>, 5> saveStateImages_;

    int windowWidth_;
    int windowHeight_;
    float halfWindowWidth_;
    float buttonXPos_;
    float buttonYPos_;
    ImVec2 buttonSize_;

    float imageYPos_;
    ImVec2 imageSize_;

    ImVec2 keyBindButtonSize_;
    float keyBindButtonXPos_;
    ImVec2 restoreDefaultsButtonSize_;
    float restoreDefaultsButtonXPos_;

    ImFont* smallFont_;
    ImFont* mediumFont_;
    ImFont* largeFont_;

// Options menu
private:
    bool pauseMenuOpen_;

    enum class RightMenuOption { BLANK, SETTINGS, SAVE, LOAD };
    RightMenuOption rightMenuOption_;

    bool overscan_;
    bool mute_;
    int audioVolume_;

    enum WindowScale { TWO = 2, THREE, FOUR, FIVE };
    WindowScale windowScale_;
    static std::unordered_map<WindowScale, std::string> windowScaleMap_;

private:
    void InitializeImGui();
    void OptionsMenu();
    void ClosePauseMenu();
    void UpdateWindowSize(bool increase);
    void ScaleGui();
    void ShowSaveStates(bool save);
    void LoadSaveStateImages();

// Key bindings
private:
    enum class InputType { UP, DOWN, LEFT, RIGHT, A, B, START, SELECT, MUTE, OVERSCAN, RESET, SPEEDDOWN, SPEEDUP, INVALID };

    std::array<std::tuple<InputType, std::string, int>, 13> INPUT_DATA = {{
        {InputType::UP,         "Up:         ", 26},
        {InputType::DOWN,       "Down:       ", 22},
        {InputType::LEFT,       "Left:       ", 4},
        {InputType::RIGHT,      "Right:      ", 7},
        {InputType::A,          "A:          ", 15},
        {InputType::B,          "B:          ", 14},
        {InputType::START,      "Start:      ", 19},
        {InputType::SELECT,     "Select:     ", 18},
        {InputType::MUTE,       "Mute:       ", 16},
        {InputType::OVERSCAN,   "Overscan:   ", 23},
        {InputType::RESET,      "Reset:      ", 21},
        {InputType::SPEEDDOWN,  "SpeedDown:  ", 80},
        {InputType::SPEEDUP,    "SpeedUp:    ", 79}
    }};

    std::unordered_map<InputType, std::pair<std::string, SDL_Scancode>> keyBindings_;
    std::unordered_map<SDL_Scancode, InputType> reverseKeyBindings_;

    InputType inputToBind_;
    std::string oldKeyStr_;

    void LoadKeyBindings();
    void SaveKeyBindings(bool restoreDefaults);
    void SetKeyBindings(SDL_Scancode scancode);
    void PrepareForKeyBinding(InputType keyToBind);
};

#endif
