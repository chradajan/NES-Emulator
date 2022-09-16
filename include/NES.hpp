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
    NES(std::string romPath, std::string savePath, char* frameBuffer);
    ~NES();

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

    bool cartLoaded_;

    void InitializeCartridge(std::string romPath, std::string savePath);
};

#endif
