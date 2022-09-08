#ifndef CPU_HPP
#define CPU_HPP

#include "OpCodes.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <fstream>
#include <memory>

constexpr uint8_t CARRY_FLAG = 0x01;
constexpr uint8_t ZERO_FLAG = 0x02;
constexpr uint8_t INTERRUPT_DISABLE_FLAG = 0x04;
constexpr uint8_t DECIMAL_FLAG = 0x08;
constexpr uint8_t OVERFLOW_FLAG = 0x40;
constexpr uint8_t NEGATIVE_FLAG = 0x80;

constexpr uint16_t ZERO_PAGE_MASK = 0x00FF;
constexpr uint16_t PAGE_MASK = 0xFF00;
constexpr uint16_t STACK_PAGE = 0x0100;
constexpr uint8_t MSB = 0x80;
constexpr uint8_t LSB = 0x01;

constexpr uint16_t NMI_VECTOR_LO = 0xFFFA;
constexpr uint16_t NMI_VECTOR_HI = 0xFFFB;
constexpr uint16_t RESET_VECTOR_LO = 0xFFFC;
constexpr uint16_t RESET_VECTOR_HI = 0xFFFD;
constexpr uint16_t BRK_VECTOR_LO = 0xFFFE;
constexpr uint16_t BRK_VECTOR_HI = 0xFFFF;

class APU;
class Cartridge;
class Controller;
class PPU;

class CPU
{
public:
    CPU(APU& apu, Cartridge& cartridge, Controller& controller, PPU& ppu);
    ~CPU() = default;
    void Clock();
    void Reset();

private:
    void Initialize();

// Other components
private:
    APU& apu;
    Cartridge& cartridge;
    Controller& controller;
    PPU& ppu;

// R/W functions
private:
    uint8_t Read(uint16_t addr);
    void Write(uint16_t addr, uint8_t data);

    uint8_t Pop();
    void Push(uint8_t data);

    void SetNextOpCode();
    uint8_t ReadAndIncrementPC();

// Interrupt Vectors
private:
    void ResetVector();

// Interrupt Handling
private:
    void IRQ();
    void NMI();

// Current state
private:
    OpCode opCode_;
    size_t cycle_;
    bool oddCycle_;

// Logging
private:
    std::ofstream log_;
    uint64_t totalCycles_;

// Registers and memory
private:
    struct
    {
        uint8_t accumulator;        // A
        uint8_t status;             // P
        uint8_t stackPointer;       // S
        uint8_t x;                  // X
        uint8_t y;                  // Y
        uint16_t programCounter;    // PC
    } Registers_;

    std::array<uint8_t, 0x0800> RAM_;

    bool IsCarry() const;
    void SetCarry(bool val);

    bool IsZero() const;
    void SetZero(bool val);

    bool IsInterruptDisable() const;
    void SetInterruptDisable(bool val);

    bool IsDecimal() const;
    void SetDecimal(bool val);

    bool IsOverflow() const;
    void SetOverflow(bool val);

    bool IsNegative() const;
    void SetNegative(bool val);

// OAM DMA
private:
    void InitiateOamDmaTransfer(uint8_t sourcePage);
    void ExecuteOamDmaTransfer();

    bool isOamDmaTransfer_;
    uint8_t oamDmaData_;
    uint16_t oamDmaAddr_;
    size_t oamDmaCycle_;
    size_t postOamDmaReturnCycle_;

// Instruction Data
private:
    uint8_t iData_;
    uint16_t iAddr_;
    uint8_t instructionIndex_;
    uint8_t regData_;
    bool isStoreOp_;
    bool branchCondition_;
    std::function<void()> instruction_;
    std::function<void()> tickFunction_;

// Addressing Modes
private:
    void Immediate();
    void Absolute();
    void ZeroPage();
    void Implied();
    void AbsoluteIndexed();
    void ZeroPageIndexed();
    void IndirectX();
    void IndirectY();
    void Relative();

    void Accumulator();
    void ZeroPageRMW();
    void ZeroPageIndexedRMW();
    void AbsoluteRWM();
    void AbsoluteIndexedRMW();

// Instructions
private:
    void ADC();
    void AND();
    void ASL();
    void BIT();
    void BRK();
    void CMP();
    void DEC();
    void DEX();
    void DEY();
    void EOR();
    void INC();
    void INX();
    void INY();
    void JSR();
    void LDA();
    void LDX();
    void LDY();
    void LSR();
    void ORA();
    void PHA();
    void PHP();
    void PLA();
    void PLP();
    void ROL();
    void ROR();
    void RTI();
    void RTS();
    void SBC();
    void TAX();
    void TAY();
    void TSX();
    void TXA();
    void TXS();
    void TYA();
    void AbsoluteJMP();
    void IndirectJMP();

    void DecodeOpCode();
};

#endif
