#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/CPU.hpp"
#include "../include/Controller.hpp"
#include "../include/GameWindow.hpp"
#include "../include/NES.hpp"
#include "../include/PPU.hpp"
#include <SDL2/SDL.h>

int main(int, char**)
{
    GameWindow window("../roms/nestest.nes");
    window.StartEmulator();
    return 0;
}