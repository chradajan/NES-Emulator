#ifndef NES_HPP
#define NES_HPP

#include <cstdint>
#include <memory>
#include <string>

class APU;
class Cartridge;
class CPU;
class Controller;
class PPU;

class NES
{
public:
    NES(std::string romPath, char* frameBuffer);
    ~NES() = default;

    void Run();

private:
    std::unique_ptr<APU> apu;
    std::unique_ptr<Cartridge> cartridge;
    std::unique_ptr<Controller> controller;
    std::unique_ptr<PPU> ppu;
    std::unique_ptr<CPU> cpu;

    void InitializeCartridge(std::string romPath);
};

#endif
