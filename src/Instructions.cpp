#include "../include/CPU.hpp"
#include <cstdint>

void CPU::ADC()
{
    uint16_t temp = iData_ + Registers_.accumulator + (IsCarry() ? 1 : 0);
    SetZero((temp & 0x00FF) == 0x0000);
    SetNegative((temp & MSB) == MSB);
    SetOverflow((((Registers_.accumulator ^ iData_) & 0x80) != 0x80) && (((Registers_.accumulator ^ temp) & 0x80) == 0x80));
    SetCarry(temp > 0xFF);
    Registers_.accumulator = temp & 0xFF;
}

void CPU::AND()
{
    iData_ &= Registers_.accumulator;
    SetNegative((iData_ & MSB) == MSB);
    SetZero(iData_ == 0x00);
    Registers_.accumulator = iData_;
}

void CPU::ASL()
{
    SetCarry((iData_ & MSB) == MSB);
    iData_ <<= 1;
    SetNegative((iData_ & MSB) == MSB);
    SetZero(iData_ == 0x00);
}

void CPU::BIT()
{
    SetNegative((iData_ & NEGATIVE_FLAG) == NEGATIVE_FLAG);
    SetOverflow((iData_ & OVERFLOW_FLAG) == OVERFLOW_FLAG);
    SetZero((iData_ & Registers_.accumulator) == 0);
}

void CPU::BRK()
{
    switch(cycle_)
    {
        case 1:
            ReadAndIncrementPC();
            break;
        case 2:
            Push(Registers_.programCounter >> 8);
            break;
        case 3:
            Push(Registers_.programCounter & ZERO_PAGE_MASK);
            break;
        case 4:
            Push(Registers_.status | 0x30);
            break;
        case 5:
            iAddr_ = Read(BRK_VECTOR_LO);
            break;
        case 6:
            iAddr_ = (Read(BRK_VECTOR_HI) << 8) | iAddr_;
            break;
        case 7:
            Registers_.programCounter = iAddr_;
            SetNextOpCode();
    }
}

void CPU::CMP()
{
    uint16_t temp = regData_ - iData_;
    SetCarry(temp < 0x0100);
    SetNegative((temp & MSB) == MSB);
    SetZero((temp & 0xFF) == 0x0000);
}

void CPU::DEC()
{
    --iData_;
    SetNegative((iData_ & MSB) == MSB);
    SetZero(iData_ == 0x00);
}

void CPU::DEX()
{
    --Registers_.x;
    SetNegative((Registers_.x & MSB) == MSB);
    SetZero(Registers_.x == 0x00);
}

void CPU::DEY()
{
    --Registers_.y;
    SetNegative((Registers_.y & MSB) == MSB);
    SetZero(Registers_.y == 0x00);
}

void CPU::EOR()
{
    Registers_.accumulator ^= iData_;
    SetNegative((Registers_.accumulator & MSB) == MSB);
    SetZero(Registers_.accumulator == 0x00);
}

void CPU::INC()
{
    ++iData_;
    SetNegative((iData_ & MSB) == MSB);
    SetZero(iData_ == 0x00);
}

void CPU::INX()
{
    ++Registers_.x;
    SetNegative((Registers_.x & MSB) == MSB);
    SetZero(Registers_.x == 0x00);
}

void CPU::INY()
{
    ++Registers_.y;
    SetNegative((Registers_.y & MSB) == MSB);
    SetZero(Registers_.y == 0x00);
}

void CPU::JSR()
{
    switch(cycle_)
    {
        case 1:
            iAddr_ = ReadAndIncrementPC();
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers_.stackPointer);
            break;
        case 3:
            Push(Registers_.programCounter >> 8);
            break;
        case 4:
            Push(Registers_.programCounter & ZERO_PAGE_MASK);
            break;
        case 5:
            iAddr_ = (ReadAndIncrementPC() << 8) | iAddr_;
            break;
        case 6:
            Registers_.programCounter = iAddr_;
            SetNextOpCode();
    }
}

void CPU::LDA()
{
    Registers_.accumulator = iData_;
    SetNegative((Registers_.accumulator & MSB) == MSB);
    SetZero(Registers_.accumulator == 0x00);
}

void CPU::LDX()
{
    Registers_.x = iData_;
    SetNegative((Registers_.x & MSB) == MSB);
    SetZero(Registers_.x == 0x00);
}

void CPU::LDY()
{
    Registers_.y = iData_;
    SetNegative((Registers_.y & MSB) == MSB);
    SetZero(Registers_.y == 0x00);
}

void CPU::LSR()
{
    SetCarry((iData_ & LSB) == LSB);
    iData_ >>= 1;
    SetNegative(false);
    SetZero(iData_ == 0x00);
}

void CPU::ORA()
{
    Registers_.accumulator |= iData_;
    SetNegative((Registers_.accumulator & MSB) == MSB);
    SetZero(Registers_.accumulator == 0x00);
}

void CPU::PHA()
{
    switch(cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 2:
            Push(Registers_.accumulator);
            break;
        case 3:
            SetNextOpCode();
    }
}

void CPU::PHP()
{
    switch(cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 2:
            Push(Registers_.status | 0x30);
            break;
        case 3:
            SetNextOpCode();
    }
}

void CPU::PLA()
{
    switch(cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers_.stackPointer);
            break;
        case 3:
            Registers_.accumulator = Pop();
            SetNegative((Registers_.accumulator & MSB) == MSB);
            SetZero(Registers_.accumulator == 0x00);
            break;
        case 4:
            SetNextOpCode();
    }
}

void CPU::PLP()
{
    switch(cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers_.stackPointer);
            break;
        case 3:
            Registers_.status = Pop() & 0xCF;
            Registers_.status |= 0x20;
            break;
        case 4:
            SetNextOpCode();
    }
}

void CPU::ROL()
{
    bool setLSB = IsCarry();
    SetCarry((iData_ & MSB) == MSB);
    iData_ <<= 1;
    if (setLSB)
    {
        iData_ |= LSB;
    }
    SetNegative((iData_ & MSB) == MSB);
    SetZero(iData_ == 0x00);
}

void CPU::ROR()
{
    bool setMSB = IsCarry();
    SetCarry((iData_ & LSB) == LSB);
    iData_ >>= 1;
    if (setMSB)
    {
        iData_ |= MSB;
    }
    SetNegative((iData_ & MSB) == MSB);
    SetZero(iData_ == 0x00);
}

void CPU::RTI()
{
    switch(cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers_.stackPointer);
            break;
        case 3:
            Registers_.status = Pop() & 0xCF;
            Registers_.status |= 0x20;
            break;
        case 4:
            iAddr_ = Pop();
            break;
        case 5:
            iAddr_ = (Pop() << 8) | iAddr_;
            break;
        case 6:
            Registers_.programCounter = iAddr_;
            SetNextOpCode();
    }
}

void CPU::RTS()
{
    switch(cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(STACK_PAGE | Registers_.stackPointer);
            break;
        case 3:
            iAddr_ = Pop();
            break;
        case 4:
            iAddr_ = (Pop() << 8) | iAddr_;
            break;
        case 5:
            // Dummy Read
            Read(iAddr_);
            break;
        case 6:
            Registers_.programCounter = iAddr_ + 0x0001;
            SetNextOpCode();
    }
}

void CPU::SBC()
{
    uint16_t temp = Registers_.accumulator - iData_;

    if (!IsCarry())
    {
        --temp;
    }

    SetNegative((temp & MSB) == MSB);
    SetZero((temp & 0x00FF) == 0x0000);
    SetOverflow((((Registers_.accumulator ^ temp) & 0x0080) == 0x0080) && (((Registers_.accumulator ^ iData_) & 0x0080) == 0x0080));
    SetCarry(temp < 0x0100);
    Registers_.accumulator = temp & 0x00FF;
}

void CPU::TAX()
{
    Registers_.x = Registers_.accumulator;
    SetNegative((Registers_.x & MSB) == MSB);
    SetZero(Registers_.x == 0x00);
}

void CPU::TAY()
{
    Registers_.y = Registers_.accumulator;
    SetNegative((Registers_.y & MSB) == MSB);
    SetZero(Registers_.y == 0x00);
}

void CPU::TSX()
{
    Registers_.x = Registers_.stackPointer;
    SetNegative((Registers_.x & MSB) == MSB);
    SetZero(Registers_.x == 0x00);
}

void CPU::TXA()
{
    Registers_.accumulator = Registers_.x;
    SetNegative((Registers_.accumulator & MSB) == MSB);
    SetZero(Registers_.accumulator == 0x00);
}

void CPU::TXS()
{
    Registers_.stackPointer = Registers_.x;
}

void CPU::TYA()
{
    Registers_.accumulator = Registers_.y;
    SetNegative((Registers_.accumulator & MSB) == MSB);
    SetZero(Registers_.accumulator == 0x00);
}

void CPU::AbsoluteJMP()
{
    switch(cycle_)
    {
        case 1:
            iAddr_ = ReadAndIncrementPC();
            break;
        case 2:
            iAddr_ = (ReadAndIncrementPC() << 8) | iAddr_;
            break;
        case 3:
            Registers_.programCounter = iAddr_;
            SetNextOpCode();
    }
}

void CPU::IndirectJMP()
{
    switch(cycle_)
    {
        case 1:
            iAddr_ = ReadAndIncrementPC();
            break;
        case 2:
            iAddr_ = (ReadAndIncrementPC() << 8) | iAddr_;
            break;
        case 3:
            iData_ = Read(iAddr_);
            iAddr_ = (iAddr_ & PAGE_MASK) | ((iAddr_ + 0x0001) & ZERO_PAGE_MASK);
            break;
        case 4:
            iAddr_ = (Read(iAddr_) << 8) | iData_;
            break;
        case 5:
            Registers_.programCounter = iAddr_;
            SetNextOpCode();
    }
}
