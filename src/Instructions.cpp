#include <cstdint>
#include "../include/CPU.hpp"

void CPU::ADC()
{
    uint16_t temp = iData + Registers.accumulator + (IsCarry() ? 1 : 0);
    SetZero(temp == 0x0000);
    SetNegative((temp & MSB) == MSB);
    SetOverflow((((Registers.accumulator ^ iData) & 0x80) != 0x80) && (((Registers.accumulator ^ temp) & 0x80) == 0x80));
    SetCarry(temp > 0xFF);
    Registers.accumulator = temp & 0xFF;
}

void CPU::AND()
{
    iData &= Registers.accumulator;
    SetNegative((iData & MSB) == MSB);
    SetZero(iData == 0x00);
    Registers.accumulator = iData;
}

void CPU::ASL()
{
    SetCarry((iData & MSB) == MSB);
    iData <<= 1;
    SetNegative((iData & MSB) == MSB);
    SetZero(iData == 0x00);
}

void CPU::BIT()
{
    SetNegative((iData & MSB) == MSB);
    SetOverflow((iData & OVERFLOW_FLAG) == OVERFLOW_FLAG);
    SetZero((iData & Registers.accumulator) == 0);
}

void CPU::BRK()
{
    switch(cycle)
    {
        case 1:
            ReadAndIncrementPC();
            break;
        case 2:
            Push(Registers.programCounter >> 8);
            break;
        case 3:
            Push(Registers.programCounter & ZERO_PAGE_MASK);
            break;
        case 4:
            Push(Registers.status | 0x30);
            break;
        case 5:
            iAddr = Read(BRK_VECTOR_LO);
            break;
        case 6:
            iAddr = (Read(BRK_VECTOR_HI) << 8) | iAddr;
            break;
        case 7:
            Registers.programCounter = iAddr;
            SetNextOpCode();
    }
}

void CPU::CMP()
{
    uint16_t temp = regData - iData;
    SetCarry(temp < 0x0100);
    SetNegative((temp & MSB) == MSB);
    SetZero((temp & 0xFF) == 0x0000);
}

void CPU::DEC()
{
    --iData;
    SetNegative((iData & MSB) == MSB);
    SetZero(iData == 0x00);
}

void CPU::DEX()
{
    --Registers.x;
    SetNegative((Registers.x & MSB) == MSB);
    SetZero(Registers.x == 0x00);
}

void CPU::DEY()
{
    --Registers.y;
    SetNegative((Registers.y & MSB) == MSB);
    SetZero(Registers.y == 0x00);
}

void CPU::EOR()
{
    Registers.accumulator ^= iData;
    SetNegative((Registers.accumulator & MSB) == MSB);
    SetZero(Registers.accumulator == 0x00);
}

void CPU::INC()
{
    ++iData;
    SetNegative((iData & MSB) == MSB);
    SetZero(iData == 0x00);
}

void CPU::INX()
{
    ++Registers.x;
    SetNegative((Registers.x & MSB) == MSB);
    SetZero(Registers.x == 0x00);
}

void CPU::INY()
{
    ++Registers.y;
    SetNegative((Registers.y & MSB) == MSB);
    SetZero(Registers.y == 0x00);
}

void CPU::JSR()
{
    switch(cycle)
    {
        case 1:
            iAddr = ReadAndIncrementPC();
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers.stackPointer);
            break;
        case 3:
            Push(Registers.programCounter >> 8);
            break;
        case 4:
            Push(Registers.programCounter & ZERO_PAGE_MASK);
            break;
        case 5:
            iAddr = (ReadAndIncrementPC() << 8) | iAddr;
            break;
        case 6:
            Registers.programCounter = iAddr;
            SetNextOpCode();
    }
}

void CPU::LDA()
{
    Registers.accumulator = iData;
    SetNegative((Registers.accumulator & MSB) == MSB);
    SetZero(Registers.accumulator == 0x00);
}

void CPU::LDX()
{
    Registers.x = iData;
    SetNegative((Registers.x & MSB) == MSB);
    SetZero(Registers.x == 0x00);
}

void CPU::LDY()
{
    Registers.y = iData;
    SetNegative((Registers.y & MSB) == MSB);
    SetZero(Registers.y == 0x00);
}

void CPU::LSR()
{
    SetCarry((iData & LSB) == LSB);
    iData >>= 1;
    SetNegative(false);
    SetZero(iData == 0x00);
}

void CPU::ORA()
{
    Registers.accumulator |= iData;
    SetNegative((Registers.accumulator & MSB) == MSB);
    SetZero(Registers.accumulator == 0x00);
}

void CPU::PHA()
{
    switch(cycle)
    {
        case 1:
            // Dummy Read
            Read(Registers.programCounter);
            break;
        case 2:
            Push(Registers.accumulator);
            break;
        case 3:
            SetNextOpCode();
    }
}

void CPU::PHP()
{
    switch(cycle)
    {
        case 1:
            // Dummy Read
            Read(Registers.programCounter);
            break;
        case 2:
            Push(Registers.status | 0x30);
            break;
        case 3:
            SetNextOpCode();
    }
}

void CPU::PLA()
{
    switch(cycle)
    {
        case 1:
            // Dummy Read
            Read(Registers.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers.stackPointer);
            break;
        case 3:
            Registers.accumulator = Pop();
            SetNegative((Registers.accumulator & MSB) == MSB);
            SetZero(Registers.accumulator == 0x00);
            break;
        case 4:
            SetNextOpCode();
    }
}

void CPU::PLP()
{
    switch(cycle)
    {
        case 1:
            // Dummy Read
            Read(Registers.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers.stackPointer);
            break;
        case 3:
            Registers.status = Pop() & 0xCF;
            break;
        case 4:
            SetNextOpCode();
    }
}

void CPU::ROL()
{
    bool setLSB = IsCarry();
    SetCarry((iData & MSB) == MSB);
    iData <<= 1;
    if (setLSB)
    {
        iData |= LSB;
    }
    SetNegative((iData & MSB) == MSB);
    SetZero(iData == 0x00);
}

void CPU::ROR()
{
    bool setMSB = IsCarry();
    SetCarry((iData & LSB) == LSB);
    iData >>= 1;
    if (setMSB)
    {
        iData |= MSB;
    }
    SetNegative((iData & MSB) == MSB);
    SetZero(iData == 0x00);
}

void CPU::RTI()
{
    switch(cycle)
    {
        case 1:
            // Dummy Read
            Read(Registers.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers.stackPointer);
            break;
        case 3:
            Registers.status = Pop() & 0xCF;
            break;
        case 4:
            iAddr = Pop();
            break;
        case 5:
            iAddr = (Pop() << 8) | iAddr;
            break;
        case 6:
            Registers.programCounter = iAddr;
            SetNextOpCode();
    }
}

void CPU::RTS()
{
    switch(cycle)
    {
        case 1:
            // Dummy Read
            Read(Registers.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers.stackPointer);
            break;
        case 3:
            iAddr = Pop();
            break;
        case 4:
            iAddr = (Pop() << 8) | iAddr;
            break;
        case 5:
            // Dummy Read
            Read(iAddr);
            break;
        case 6:
            Registers.programCounter = iAddr + 0x0001;
            SetNextOpCode();
    }
}

void CPU::SBC()
{
    uint16_t temp = Registers.accumulator - iData;
    if (IsCarry())
    {
        --temp;
    }
    SetNegative((temp & MSB) == MSB);
    SetZero((temp & 0x00FF) == 0x00);
    SetOverflow((((Registers.accumulator ^ temp) & 0x0080) == 0x0080) && (((Registers.accumulator ^ iData) & 0x0080) == 0x0080));
    SetCarry(temp < 0x0100);
    Registers.accumulator = temp & 0x00FF;
}

void CPU::TAX()
{
    Registers.x = Registers.accumulator;
    SetNegative((Registers.x & MSB) == MSB);
    SetZero(Registers.x == 0x00);
}

void CPU::TAY()
{
    Registers.y = Registers.accumulator;
    SetNegative((Registers.y & MSB) == MSB);
    SetZero(Registers.y == 0x00);
}

void CPU::TSX()
{
    Registers.x = Registers.stackPointer;
    SetNegative((Registers.x & MSB) == MSB);
    SetZero(Registers.x == 0x00);
}

void CPU::TXA()
{
    Registers.accumulator = Registers.x;
    SetNegative((Registers.accumulator & MSB) == MSB);
    SetZero(Registers.accumulator == 0x00);
}

void CPU::TXS()
{
    Registers.stackPointer = Registers.x;
}

void CPU::TYA()
{
    Registers.accumulator = Registers.y;
    SetNegative((Registers.accumulator & MSB) == MSB);
    SetZero(Registers.accumulator == 0x00);
}

void CPU::AbsoluteJMP()
{
    switch(cycle)
    {
        case 1:
            iAddr = ReadAndIncrementPC();
            break;
        case 2:
            iAddr = (ReadAndIncrementPC() << 8) | iAddr;
            break;
        case 3:
            Registers.programCounter = iAddr;
            SetNextOpCode();
    }
}

void CPU::IndirectJMP()
{
    switch(cycle)
    {
        case 1:
            iAddr = ReadAndIncrementPC();
            break;
        case 2:
            iAddr = (ReadAndIncrementPC() << 8) | iAddr;
            break;
        case 3:
            iData = Read(iAddr);
            iAddr = (iAddr & PAGE_MASK) | ((iAddr + 0x0001) & ZERO_PAGE_MASK);
            break;
        case 4:
            iAddr = (Read(iAddr) << 8) | iData;
            break;
        case 5:
            Registers.programCounter = iAddr;
            SetNextOpCode();
    }
}
