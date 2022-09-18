#ifndef NES_HPP
#define NES_HPP

#include <cstdint>
#include <filesystem>
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
    NES(char* frameBuffer);
    ~NES();

    std::string GetFileName();
    void LoadCartridge(std::filesystem::path romPath, std::filesystem::path savePath);
    void UnloadCartridge();

    void Run();
    void Reset();
    bool Ready();

    void RunUntilSerializable();
    void Serialize(std::ofstream& saveState);
    void Deserialize(std::ifstream& saveState);

private:
    std::unique_ptr<APU> apu_;
    std::unique_ptr<Cartridge> cartridge_;
    std::unique_ptr<Controller> controller_;
    std::unique_ptr<PPU> ppu_;
    std::unique_ptr<CPU> cpu_;

    std::string fileName_;
    bool cartLoaded_;

    void InitializeCartridge(std::string romPath, std::string savePath);
};

#endif
