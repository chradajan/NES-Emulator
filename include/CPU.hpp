#ifndef CPU_HPP
#define CPU_HPP

#include <array>
#include <cstdint>
#include <functional>
#include "OpCodes.hpp"

class CPU
{
public:
    CPU();
    ~CPU() = default;
    void Tick();
    void Reset();

// R/W functions
private:
    uint8_t Read(uint16_t addr);
    void Write(uint16_t addr, uint8_t data);

    uint8_t Pop();
    void Push(uint8_t data);

    void SetNextOpCode();
    uint8_t ReadAndIncrementPC();

// State variables
private:
    OpCode opCode;
    uint8_t cycle;
    bool oddCycle;

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
    } Registers;

    std::array<uint8_t, 0x0800> RAM;

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

    bool isOamDmaTransfer;
    uint8_t OamDmaData;
    uint16_t OamDmaAddr;
    uint16_t OamDmaCycle;

// Instruction Data
private:
    uint8_t iData;
    uint16_t iAddr;
    uint8_t index;
    uint8_t regData;
    bool isStoreOp;
    bool branchCondition;
    std::function<void()> instruction;
    std::function<void()> tickFunction;

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
