#include "../include/CPU.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/Controller.hpp"
#include "../include/PPU.hpp"
#include "../include/RegisterAddresses.hpp"
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>

#ifdef LOGGING
#include <iomanip>
#include <fstream>
#endif

CPU::CPU(APU& apu, Controller& controller, PPU& ppu) :
    apu_(apu),
    controller_(controller),
    ppu_(ppu)
{
    Initialize();
    tickFunction_ = std::bind(&CPU::ResetVector, this);
}

void CPU::Clock()
{
    oddCycle_ = !oddCycle_;
    ++cycle_;
    ++totalCycles_;

    auto dmcRequest = apu_.DmcRequestSample();

    if (dmcRequest)
    {
        apu_.SetDmcSample(Read(dmcRequest.value()));
    }

    if (isOamDmaTransfer_)
    {
        ExecuteOamDmaTransfer();
    }
    else if (cycle_ == 1)
    {
        DecodeOpCode();
    }
    else
    {
        tickFunction_();
    }
}

void CPU::Reset()
{
    // Current state
    opCode_ = OpCode::INVALID_CODE;
    cycle_ = 1;
    oddCycle_ = false;

    // Registers
    Registers_.stackPointer -= 3;
    SetInterruptDisable(true);

    // OAM DMA
    isOamDmaTransfer_ = false;
    oamDmaData_ = 0x00;
    oamDmaAddr_ = 0x0000;
    oamDmaCycle_ = 0;
    postOamDmaReturnCycle_ = 0;

    // Instruction data
    iData_ = 0x00;
    iAddr_ = 0x0000;
    instructionIndex_ = 0x00;
    regData_ = 0x00;
    isStoreOp_ = false;
    branchCondition_ = false;
    instruction_ = [](){};
    tickFunction_ = std::bind(&CPU::ResetVector, this);
}

void CPU::LoadCartridge(Cartridge* cartridge)
{
    cartridge_ = cartridge;
}

void CPU::Initialize()
{
    // Current state
    opCode_ = OpCode::INVALID_CODE;
    cycle_ = 1;
    oddCycle_ = false;

    // Logging
    #ifdef LOGGING
    log_.open("../Logs/log.log");
    totalCycles_ = 0;
    #endif

    // Registers
    Registers_.accumulator = 0x00;
    Registers_.status = 0x24;
    Registers_.stackPointer = 0xFD;
    Registers_.x = 0x00;
    Registers_.y = 0x00;

    // RAM
    RAM_.fill(0x00);

    // OAM DMA
    isOamDmaTransfer_ = false;
    oamDmaData_ = 0x00;
    oamDmaAddr_ = 0x0000;
    oamDmaCycle_ = 0;
    postOamDmaReturnCycle_ = 0;

    // Instruction data
    iData_ = 0x00;
    iAddr_ = 0x0000;
    instructionIndex_ = 0x00;
    regData_ = 0x00;
    isStoreOp_ = false;
    branchCondition_ = false;
    instruction_ = [](){};
    tickFunction_ = [](){};
}

uint8_t CPU::Read(uint16_t addr)
{
    if (addr < 0x2000)
    {
        return RAM_[addr % 0x0800];
    }
    else if (addr < 0x4000)
    {
        return ppu_.ReadReg(addr);
    }
    else if (addr < 0x4016)
    {
        return apu_.ReadReg(addr);
    }
    else if (addr < 0x4018)
    {
        return controller_.ReadReg(addr);
    }
    else if (addr < 0x4020)
    {
        // CPU Test Mode
        return 0x00;
    }
    else
    {
        return cartridge_->ReadPRG(addr);
    }
}

void CPU::Write(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        RAM_[addr % 0x0800] = data;
    }
    else if (addr < 0x4000)
    {
        ppu_.WriteReg(addr, data);
    }
    else if (addr == OAMDMA_ADDR)
    {
        InitiateOamDmaTransfer(data);
    }
    else if ((addr < 0x4016) || (addr == FRAME_COUNTER_ADDR))
    {
        apu_.WriteReg(addr, data);
    }
    else if (addr == JOY1_ADDR)
    {
        controller_.WriteReg(data);
    }
    else if (addr < 0x4020)
    {
        // CPU Test Mode
        return;
    }
    else
    {
        cartridge_->WritePRG(addr, data);
    }
}

uint8_t CPU::Pop()
{
    ++Registers_.stackPointer;
    return RAM_[STACK_PAGE | Registers_.stackPointer];
}

void CPU::Push(uint8_t data)
{
    RAM_[STACK_PAGE | Registers_.stackPointer] = data;
    --Registers_.stackPointer;
}

void CPU::SetNextOpCode()
{
    #ifdef LOGGING

    uint16_t current_pc = Registers_.programCounter;
    opCode_ = static_cast<OpCode>(ReadAndIncrementPC());
    cycle_ = 0;
    auto [scanline, dot] = ppu.GetState();

    log_ << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << current_pc << "  " << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(opCode_) << "  ";
    log_ << "A:" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(Registers_.accumulator) << " X:" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(Registers_.x) << " Y:";
    log_ << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(Registers_.y) << " P:" << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(Registers_.status) << " SP:";
    log_ << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(Registers_.stackPointer) << " ";
    log_ << "PPU:" << std::dec << std::setw(3) << std::setfill(' ') << (unsigned int)scanline << ",";
    log_ << std::setw(3) << std::setfill(' ') << (unsigned int)dot << " ";
    log_ << "CYC:" << std::dec << static_cast<unsigned int>(totalCycles_) << "\n";

    if (ppu.NMI())
    {
        cycle_ = 1;
        --Registers_.programCounter;
        tickFunction_ = std::bind(&CPU::NMI, this);
        tickFunction_();
    }
    else if (!IsInterruptDisable() && cartridge.IRQ())
    {
        cycle_ = 1;
        --Registers_.programCounter;
        tickFunction_ = std::bind(&CPU::IRQ, this);
        tickFunction_();
    }

    #else

    if (ppu_.NMI())
    {
        cycle_ = 1;
        tickFunction_ = std::bind(&CPU::NMI, this);
        tickFunction_();
    }
    else if (!IsInterruptDisable() && cartridge_->IRQ())
    {
        cycle_ = 1;
        tickFunction_ = std::bind(&CPU::IRQ, this);
        tickFunction_();
    }
    else
    {
        opCode_ = static_cast<OpCode>(ReadAndIncrementPC());
        cycle_ = 0;
    }

    #endif
}

uint8_t CPU::ReadAndIncrementPC()
{
    uint8_t data = Read(Registers_.programCounter);
    ++Registers_.programCounter;
    return data;
}

void CPU::ResetVector()
{
    switch (cycle_)
    {
        case 6:
            iAddr_ = Read(RESET_VECTOR_LO);
            break;
        case 7:
            iAddr_ |= (Read(RESET_VECTOR_HI) << 8);
            break;
        case 8:
            Registers_.programCounter = iAddr_;
            SetNextOpCode();
            break;
    }
}

void CPU::IRQ()
{
    switch (cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 3:
            Push(Registers_.programCounter >> 8);
            break;
        case 4:
            Push(Registers_.programCounter & 0xFF);
            break;
        case 5:
            Push((Registers_.status | 0x20) & 0xEF);
            break;
        case 6:
            iAddr_ = Read(BRK_VECTOR_LO);
            break;
        case 7:
            iAddr_ |= (Read(BRK_VECTOR_HI) << 8);
            break;
        case 8:
            SetInterruptDisable(true);
            Registers_.programCounter = iAddr_;
            SetNextOpCode();
    }
}

void CPU::NMI()
{
    switch (cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 2:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 3:
            Push(Registers_.programCounter >> 8);
            break;
        case 4:
            Push(Registers_.programCounter & 0xFF);
            break;
        case 5:
            Push((Registers_.status | 0x20) & 0xEF);
            break;
        case 6:
            iAddr_ = Read(NMI_VECTOR_LO);
            break;
        case 7:
            iAddr_ |= (Read(NMI_VECTOR_HI) << 8);
            break;
        case 8:
            Registers_.programCounter = iAddr_;
            SetNextOpCode();
    }
}

bool CPU::IsCarry() const
{
    return (Registers_.status & CARRY_FLAG) == CARRY_FLAG;
}

void CPU::SetCarry(bool val)
{
    if (val)
    {
        Registers_.status |= CARRY_FLAG;
    }
    else
    {
        Registers_.status &= ~CARRY_FLAG;
    }
}

bool CPU::IsZero() const
{
    return (Registers_.status & ZERO_FLAG) == ZERO_FLAG;
}

void CPU::SetZero(bool val)
{
    if (val)
    {
        Registers_.status |= ZERO_FLAG;
    }
    else
    {
        Registers_.status &= ~ZERO_FLAG;
    }
}

bool CPU::IsInterruptDisable() const
{
    return (Registers_.status & INTERRUPT_DISABLE_FLAG) == INTERRUPT_DISABLE_FLAG;
}

void CPU::SetInterruptDisable(bool val)
{
    if (val)
    {
        Registers_.status |= INTERRUPT_DISABLE_FLAG;
    }
    else
    {
        Registers_.status &= ~INTERRUPT_DISABLE_FLAG;
    }
}

bool CPU::IsDecimal() const
{
    return (Registers_.status & DECIMAL_FLAG) == DECIMAL_FLAG;
}

void CPU::SetDecimal(bool val)
{
    if (val)
    {
        Registers_.status |= DECIMAL_FLAG;
    }
    else
    {
        Registers_.status &= ~DECIMAL_FLAG;
    }
}

bool CPU::IsOverflow() const
{
    return (Registers_.status & OVERFLOW_FLAG) == OVERFLOW_FLAG;
}

void CPU::SetOverflow(bool val)
{
    if (val)
    {
        Registers_.status |= OVERFLOW_FLAG;
    }
    else
    {
        Registers_.status &= ~OVERFLOW_FLAG;
    }
}

bool CPU::IsNegative() const
{
    return (Registers_.status & NEGATIVE_FLAG) == NEGATIVE_FLAG;
}

void CPU::SetNegative(bool val)
{
    if (val)
    {
        Registers_.status |= NEGATIVE_FLAG;
    }
    else
    {
        Registers_.status &= ~NEGATIVE_FLAG;
    }
}

void CPU::InitiateOamDmaTransfer(uint8_t sourcePage)
{
    isOamDmaTransfer_ = true;
    oamDmaData_ = 0x00;
    oamDmaAddr_ = (sourcePage << 8);
    oamDmaCycle_ = oddCycle_ ? 514 : 513;
    postOamDmaReturnCycle_ = cycle_;
}

void CPU::ExecuteOamDmaTransfer()
{
    if (oamDmaCycle_ <= 512)
    {
        if (oamDmaCycle_ % 2 == 0)
        {
            oamDmaData_ = Read(oamDmaAddr_);
            ++oamDmaAddr_;
        }
        else
        {
            ppu_.WriteReg(OAMDATA_ADDR, oamDmaData_);
        }
    }

    --oamDmaCycle_;

    if (oamDmaCycle_ == 0)
    {
        isOamDmaTransfer_ = false;
        cycle_ = postOamDmaReturnCycle_;
    }
}

void CPU::DecodeOpCode()
{
    isStoreOp_ = false;

    switch (opCode_)
    {
        case OpCode::Immediate_ADC:
            instruction_ = std::bind(&CPU::ADC, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_ADC:
            instruction_ = std::bind(&CPU::ADC, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_ADC:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ADC, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_ADC:
            instruction_ = std::bind(&CPU::ADC, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_ADC:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ADC, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_ADC:
            instructionIndex_ = Registers_.y;
            instruction_ = std::bind(&CPU::ADC, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_ADC:
            instruction_ = std::bind(&CPU::ADC, this);
            tickFunction_ = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_ADC:
            instruction_ = std::bind(&CPU::ADC, this);
            tickFunction_ = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Immediate_AND:
            instruction_ = std::bind(&CPU::AND, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_AND:
            instruction_ = std::bind(&CPU::AND, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_AND:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::AND, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_AND:
            instruction_ = std::bind(&CPU::AND, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_AND:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::AND, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_AND:
            instructionIndex_ = Registers_.y;
            instruction_ = std::bind(&CPU::AND, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_AND:
            instruction_ = std::bind(&CPU::AND, this);
            tickFunction_ = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_AND:
            instruction_ = std::bind(&CPU::AND, this);
            tickFunction_ = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Accumulator_ASL:
            instruction_ = std::bind(&CPU::ASL, this);
            tickFunction_ = std::bind(&CPU::Accumulator, this);
            break;
        case OpCode::ZeroPage_ASL:
            instruction_ = std::bind(&CPU::ASL, this);
            tickFunction_ = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_ASL:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ASL, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_ASL:
            instruction_ = std::bind(&CPU::ASL, this);
            tickFunction_ = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_ASL:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ASL, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Relative_BCC:
            branchCondition_ = !IsCarry();
            tickFunction_ = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BCS:
            branchCondition_ = IsCarry();
            tickFunction_ = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BEQ:
            branchCondition_ = IsZero();
            tickFunction_ = std::bind(&CPU::Relative, this);
            break;
        case OpCode::ZeroPage_BIT:
            instruction_ = std::bind(&CPU::BIT, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::Absolute_BIT:
            instruction_ = std::bind(&CPU::BIT, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Relative_BMI:
            branchCondition_ = IsNegative();
            tickFunction_ = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BNE:
            branchCondition_ = !IsZero();
            tickFunction_ = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BPL:
            branchCondition_ = !IsNegative();
            tickFunction_ = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Implied_BRK:
            tickFunction_ = std::bind(&CPU::BRK, this);
            break;
        case OpCode::Relative_BVC:
            branchCondition_ = !IsOverflow();
            tickFunction_ = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BVS:
            branchCondition_ = IsOverflow();
            tickFunction_ = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Implied_CLC:
            instruction_ = std::bind(&CPU::SetCarry, this, false);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_CLD:
            instruction_ = std::bind(&CPU::SetDecimal, this, false);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_CLI:
            instruction_ = std::bind(&CPU::SetInterruptDisable, this, false);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_CLV:
            instruction_ = std::bind(&CPU::SetOverflow, this, false);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Immediate_CMP:
            regData_ = Registers_.accumulator;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_CMP:
            regData_ = Registers_.accumulator;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_CMP:
            regData_ = Registers_.accumulator;
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_CMP:
            regData_ = Registers_.accumulator;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_CMP:
            regData_ = Registers_.accumulator;
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_CMP:
            regData_ = Registers_.accumulator;
            instructionIndex_ = Registers_.y;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_CMP:
            regData_ = Registers_.accumulator;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_CMP:
            regData_ = Registers_.accumulator;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Immediate_CPX:
            regData_ = Registers_.x;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_CPX:
            regData_ = Registers_.x;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::Absolute_CPX:
            regData_ = Registers_.x;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Immediate_CPY:
            regData_ = Registers_.y;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_CPY:
            regData_ = Registers_.y;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::Absolute_CPY:
            regData_ = Registers_.y;
            instruction_ = std::bind(&CPU::CMP, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::ZeroPage_DEC:
            instruction_ = std::bind(&CPU::DEC, this);
            tickFunction_ = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_DEC:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::DEC, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_DEC:
            instruction_ = std::bind(&CPU::DEC, this);
            tickFunction_ = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_DEC:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::DEC, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Implied_DEX:
            instruction_ = std::bind(&CPU::DEX, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_DEY:
            instruction_ = std::bind(&CPU::DEY, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Immediate_EOR:
            instruction_ = std::bind(&CPU::EOR, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_EOR:
            instruction_ = std::bind(&CPU::EOR, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_EOR:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::EOR, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_EOR:
            instruction_ = std::bind(&CPU::EOR, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_EOR:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::EOR, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_EOR:
            instructionIndex_ = Registers_.y;
            instruction_ = std::bind(&CPU::EOR, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_EOR:
            instruction_ = std::bind(&CPU::EOR, this);
            tickFunction_ = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_EOR:
            instruction_ = std::bind(&CPU::EOR, this);
            tickFunction_ = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::ZeroPage_INC:
            instruction_ = std::bind(&CPU::INC, this);
            tickFunction_ = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_INC:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::INC, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_INC:
            instruction_ = std::bind(&CPU::INC, this);
            tickFunction_ = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_INC:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::INC, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Implied_INX:
            instruction_ = std::bind(&CPU::INX, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_INY:
            instruction_ = std::bind(&CPU::INY, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Absolute_JMP:
            tickFunction_ = std::bind(&CPU::AbsoluteJMP, this);
            break;
        case OpCode::Indirect_JMP:
            tickFunction_ = std::bind(&CPU::IndirectJMP, this);
            break;
        case OpCode::Absolute_JSR:
            tickFunction_ = std::bind(&CPU::JSR, this);
            break;
        case OpCode::Immediate_LDA:
            instruction_ = std::bind(&CPU::LDA, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_LDA:
            instruction_ = std::bind(&CPU::LDA, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_LDA:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::LDA, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_LDA:
            instruction_ = std::bind(&CPU::LDA, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_LDA:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::LDA, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_LDA:
            instructionIndex_ = Registers_.y;
            instruction_ = std::bind(&CPU::LDA, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_LDA:
            instruction_ = std::bind(&CPU::LDA, this);
            tickFunction_ = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_LDA:
            instruction_ = std::bind(&CPU::LDA, this);
            tickFunction_ = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Immediate_LDX:
            instruction_ = std::bind(&CPU::LDX, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_LDX:
            instruction_ = std::bind(&CPU::LDX, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_Y_LDX:
            instructionIndex_ = Registers_.y;
            instruction_ = std::bind(&CPU::LDX, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_LDX:
            instruction_ = std::bind(&CPU::LDX, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_Y_LDX:
            instructionIndex_ = Registers_.y;
            instruction_ = std::bind(&CPU::LDX, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Immediate_LDY:
            instruction_ = std::bind(&CPU::LDY, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_LDY:
            instruction_ = std::bind(&CPU::LDY, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_LDY:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::LDY, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_LDY:
            instruction_ = std::bind(&CPU::LDY, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_LDY:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::LDY, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Accumulator_LSR:
            instruction_ = std::bind(&CPU::LSR, this);
            tickFunction_ = std::bind(&CPU::Accumulator, this);
            break;
        case OpCode::ZeroPage_LSR:
            instruction_ = std::bind(&CPU::LSR, this);
            tickFunction_ = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_LSR:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::LSR, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_LSR:
            instruction_ = std::bind(&CPU::LSR, this);
            tickFunction_ = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_LSR:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::LSR, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Implied_NOP:
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Immediate_ORA:
            instruction_ = std::bind(&CPU::ORA, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_ORA:
            instruction_ = std::bind(&CPU::ORA, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_ORA:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ORA, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_ORA:
            instruction_ = std::bind(&CPU::ORA, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_ORA:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ORA, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_ORA:
            instructionIndex_ = Registers_.y;
            instruction_ = std::bind(&CPU::ORA, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_ORA:
            instruction_ = std::bind(&CPU::ORA, this);
            tickFunction_ = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_ORA:
            instruction_ = std::bind(&CPU::ORA, this);
            tickFunction_ = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Implied_PHA:
            tickFunction_ = std::bind(&CPU::PHA, this);
            break;
        case OpCode::Implied_PHP:
            tickFunction_ = std::bind(&CPU::PHP, this);
            break;
        case OpCode::Implied_PLA:
            tickFunction_ = std::bind(&CPU::PLA, this);
            break;
        case OpCode::Implied_PLP:
            tickFunction_ = std::bind(&CPU::PLP, this);
            break;
        case OpCode::Accumulator_ROL:
            instruction_ = std::bind(&CPU::ROL, this);
            tickFunction_ = std::bind(&CPU::Accumulator, this);
            break;
        case OpCode::ZeroPage_ROL:
            instruction_ = std::bind(&CPU::ROL, this);
            tickFunction_ = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_ROL:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ROL, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_ROL:
            instruction_ = std::bind(&CPU::ROL, this);
            tickFunction_ = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_ROL:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ROL, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Accumulator_ROR:
            instruction_ = std::bind(&CPU::ROR, this);
            tickFunction_ = std::bind(&CPU::Accumulator, this);
            break;
        case OpCode::ZeroPage_ROR:
            instruction_ = std::bind(&CPU::ROR, this);
            tickFunction_ = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_ROR:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ROR, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_ROR:
            instruction_ = std::bind(&CPU::ROR, this);
            tickFunction_ = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_ROR:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::ROR, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Implied_RTI:
            tickFunction_ = std::bind(&CPU::RTI, this);
            break;
        case OpCode::Implied_RTS:
            tickFunction_ = std::bind(&CPU::RTS, this);
            break;
        case OpCode::Immediate_SBC:
            instruction_ = std::bind(&CPU::SBC, this);
            tickFunction_ = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_SBC:
            instruction_ = std::bind(&CPU::SBC, this);
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_SBC:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::SBC, this);
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_SBC:
            instruction_ = std::bind(&CPU::SBC, this);
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_SBC:
            instructionIndex_ = Registers_.x;
            instruction_ = std::bind(&CPU::SBC, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_SBC:
            instructionIndex_ = Registers_.y;
            instruction_ = std::bind(&CPU::SBC, this);
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_SBC:
            instruction_ = std::bind(&CPU::SBC, this);
            tickFunction_ = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_SBC:
            instruction_ = std::bind(&CPU::SBC, this);
            tickFunction_ = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Implied_SEC:
            instruction_ = std::bind(&CPU::SetCarry, this, true);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_SED:
            instruction_ = std::bind(&CPU::SetDecimal, this, true);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_SEI:
            instruction_ = std::bind(&CPU::SetInterruptDisable, this, true);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::ZeroPage_STA:
            isStoreOp_ = true;
            regData_ = Registers_.accumulator;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_STA:
            isStoreOp_ = true;
            regData_ = Registers_.accumulator;
            instructionIndex_ = Registers_.x;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_STA:
            isStoreOp_ = true;
            regData_ = Registers_.accumulator;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_STA:
            isStoreOp_ = true;
            regData_ = Registers_.accumulator;
            instructionIndex_ = Registers_.x;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_STA:
            isStoreOp_ = true;
            regData_ = Registers_.accumulator;
            instructionIndex_ = Registers_.y;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_STA:
            isStoreOp_ = true;
            regData_ = Registers_.accumulator;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_STA:
            isStoreOp_ = true;
            regData_ = Registers_.accumulator;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::ZeroPage_STX:
            isStoreOp_ = true;
            regData_ = Registers_.x;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_Y_STX:
            isStoreOp_ = true;
            regData_ = Registers_.x;
            instructionIndex_ = Registers_.y;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_STX:
            isStoreOp_ = true;
            regData_ = Registers_.x;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::ZeroPage_STY:
            isStoreOp_ = true;
            regData_ = Registers_.y;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_STY:
            isStoreOp_ = true;
            regData_ = Registers_.y;
            instructionIndex_ = Registers_.x;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_STY:
            isStoreOp_ = true;
            regData_ = Registers_.y;
            instruction_ = [](){};
            tickFunction_ = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Implied_TAX:
            instruction_ = std::bind(&CPU::TAX, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TAY:
            instruction_ = std::bind(&CPU::TAY, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TSX:
            instruction_ = std::bind(&CPU::TSX, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TXA:
            instruction_ = std::bind(&CPU::TXA, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TXS:
            instruction_ = std::bind(&CPU::TXS, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TYA:
            instruction_ = std::bind(&CPU::TYA, this);
            tickFunction_ = std::bind(&CPU::Implied, this);
            break;
        default:
            std::cout << "INVALID OPCODE " << std::hex << (unsigned int)opCode_ << std::endl;
            SetNextOpCode();
    }

    tickFunction_();
}

bool CPU::Serializable()
{
    return (cycle_ == 0);
}

void CPU::Serialize(std::ofstream& saveState)
{
    uint16_t byteExpander = static_cast<uint16_t>(opCode_);
    saveState.write((char*)&byteExpander, sizeof(opCode_));
    saveState.write((char*)&oddCycle_, sizeof(oddCycle_));
    saveState.write((char*)&Registers_, sizeof(Registers_));
    saveState.write((char*)RAM_.data(), 0x0800);
}

void CPU::Deserialize(std::ifstream& saveState)
{
    saveState.read((char*)&opCode_, sizeof(opCode_));
    saveState.read((char*)&oddCycle_, sizeof(oddCycle_));
    saveState.read((char*)&Registers_, sizeof(Registers_));
    saveState.read((char*)RAM_.data(), 0x0800);

    cycle_ = 0;
    isOamDmaTransfer_ = false;
    isStoreOp_ = false;
    branchCondition_ = false;
}
