#include "../include/CPU.hpp"
#include "../include/APU.hpp"
#include "../include/Cartridge.hpp"
#include "../include/Controller.hpp"
#include "../include/PPU.hpp"
#include "../include/RegisterAddresses.hpp"
#include <cstdint>
#include <functional>
#include <iostream>

CPU::CPU(APU& apu, Cartridge& cartridge, Controller& controller, PPU& ppu) :
    apu(apu),
    cartridge(cartridge),
    controller(controller),
    ppu(ppu)
{
    Registers.accumulator = 0x00;
    Registers.status = 0x34;
    Registers.stackPointer = 0xFD;
    Registers.x = 0x00;
    Registers.y = 0x00;
    RAM.fill(0x00);
}

void CPU::Tick()
{

}

void CPU::Reset()
{

}

uint8_t CPU::Read(uint16_t addr)
{
    if (addr < 0x2000)
    {
        return RAM[addr % 0x0800];
    }
    else if (addr < 0x4000)
    {
        return ppu.ReadReg(addr);
    }
    else if (addr < 0x4016)
    {
        return apu.ReadReg(addr);
    }
    else if (addr < 0x4018)
    {
        return controller.ReadReg(addr);
    }
    else if (addr < 0x4020)
    {
        // CPU Test Mode
        return 0x00;
    }
    else
    {
        return cartridge.ReadPRG(addr);
    }
}

void CPU::Write(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        RAM[addr % 0x0800] = data;
    }
    else if (addr < 0x4000)
    {
        ppu.WriteReg(addr, data);
    }
    else if (addr == OAMDMA_ADDR)
    {
        InitiateOamDmaTransfer(data);
    }
    else if ((addr < 0x4016) || (addr == JOY2_ADDR))
    {
        apu.WriteReg(addr, data);
    }
    else if (addr == JOY1_ADDR)
    {
        controller.WriteReg(data);
    }
    else if (addr < 0x4020)
    {
        // CPU Test Mode
        return;
    }
    else
    {
        cartridge.WritePRG(addr, data);
    }
}

uint8_t CPU::Pop()
{
    ++Registers.stackPointer;
    return RAM[STACK_PAGE | Registers.stackPointer];
}

void CPU::Push(uint8_t data)
{
    RAM[STACK_PAGE | Registers.stackPointer] = data;
    --Registers.stackPointer;
}

void CPU::SetNextOpCode()
{
    if (ppu.NMI())
    {
        // TODO: Handle NMI
    }
    else
    {
        opCode = static_cast<OpCode>(ReadAndIncrementPC());
        cycle = 0;
    }
}

uint8_t CPU::ReadAndIncrementPC()
{
    uint8_t data = Read(Registers.programCounter);
    ++Registers.programCounter;
    return data;
}

bool CPU::IsCarry() const
{
    return (Registers.status & CARRY_FLAG) == CARRY_FLAG;
}

void CPU::SetCarry(bool val)
{
    if (val)
    {
        Registers.status |= CARRY_FLAG;
    }
    else
    {
        Registers.status &= ~CARRY_FLAG;
    }
}

bool CPU::IsZero() const
{
    return (Registers.status & ZERO_FLAG) == ZERO_FLAG;
}

void CPU::SetZero(bool val)
{
    if (val)
    {
        Registers.status |= ZERO_FLAG;
    }
    else
    {
        Registers.status &= ~ZERO_FLAG;
    }
}

bool CPU::IsInterruptDisable() const
{
    return (Registers.status & INTERRUPT_DISABLE_FLAG) == INTERRUPT_DISABLE_FLAG;
}

void CPU::SetInterruptDisable(bool val)
{
    if (val)
    {
        Registers.status |= INTERRUPT_DISABLE_FLAG;
    }
    else
    {
        Registers.status &= ~INTERRUPT_DISABLE_FLAG;
    }
}

bool CPU::IsDecimal() const
{
    return (Registers.status & DECIMAL_FLAG) == DECIMAL_FLAG;
}

void CPU::SetDecimal(bool val)
{
    if (val)
    {
        Registers.status |= DECIMAL_FLAG;
    }
    else
    {
        Registers.status &= ~DECIMAL_FLAG;
    }
}

bool CPU::IsOverflow() const
{
    return (Registers.status & OVERFLOW_FLAG) == OVERFLOW_FLAG;
}

void CPU::SetOverflow(bool val)
{
    if (val)
    {
        Registers.status |= OVERFLOW_FLAG;
    }
    else
    {
        Registers.status &= ~OVERFLOW_FLAG;
    }
}

bool CPU::IsNegative() const
{
    return (Registers.status & NEGATIVE_FLAG) == NEGATIVE_FLAG;
}

void CPU::SetNegative(bool val)
{
    if (val)
    {
        Registers.status |= NEGATIVE_FLAG;
    }
    else
    {
        Registers.status &= ~NEGATIVE_FLAG;
    }
}

void CPU::InitiateOamDmaTransfer(uint8_t sourcePage)
{
    isOamDmaTransfer = true;
    OamDmaData = 0x00;
    OamDmaAddr = (sourcePage << 8);
    OamDmaCycle = oddCycle ? 514 : 513;
}

void CPU::ExecuteOamDmaTransfer()
{
    if (OamDmaCycle <= 512)
    {
        if (OamDmaCycle % 2 == 0)
        {
            OamDmaData = Read(OamDmaAddr);
            ++OamDmaAddr;
        }
        else
        {
            ppu.WriteReg(OAMDATA_ADDR, OamDmaData);
        }
    }

    --OamDmaCycle;

    if (OamDmaCycle == 0)
    {
        isOamDmaTransfer = false;
    }
}

void CPU::DecodeOpCode()
{
    isStoreOp = false;
    switch(opCode)
    {
        case OpCode::Immediate_ADC:
            instruction = std::bind(&CPU::ADC, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_ADC:
            instruction = std::bind(&CPU::ADC, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_ADC:
            index = Registers.x;
            instruction = std::bind(&CPU::ADC, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_ADC:
            instruction = std::bind(&CPU::ADC, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_ADC:
            index = Registers.x;
            instruction = std::bind(&CPU::ADC, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_ADC:
            index = Registers.y;
            instruction = std::bind(&CPU::ADC, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_ADC:
            instruction = std::bind(&CPU::ADC, this);
            tickFunction = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_ADC:
            instruction = std::bind(&CPU::ADC, this);
            tickFunction = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Immediate_AND:
            instruction = std::bind(&CPU::AND, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_AND:
            instruction = std::bind(&CPU::AND, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_AND:
            index = Registers.x;
            instruction = std::bind(&CPU::AND, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_AND:
            instruction = std::bind(&CPU::AND, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_AND:
            index = Registers.x;
            instruction = std::bind(&CPU::AND, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_AND:
            index = Registers.y;
            instruction = std::bind(&CPU::AND, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_AND:
            instruction = std::bind(&CPU::AND, this);
            tickFunction = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_AND:
            instruction = std::bind(&CPU::AND, this);
            tickFunction = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Accumulator_ASL:
            instruction = std::bind(&CPU::ASL, this);
            tickFunction = std::bind(&CPU::Accumulator, this);
            break;
        case OpCode::ZeroPage_ASL:
            instruction = std::bind(&CPU::ASL, this);
            tickFunction = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_ASL:
            index = Registers.x;
            instruction = std::bind(&CPU::ASL, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_ASL:
            instruction = std::bind(&CPU::ASL, this);
            tickFunction = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_ASL:
            index = Registers.x;
            instruction = std::bind(&CPU::ASL, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Relative_BCC:
            branchCondition = !IsCarry();
            tickFunction = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BCS:
            branchCondition = IsCarry();
            tickFunction = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BEQ:
            branchCondition = IsZero();
            tickFunction = std::bind(&CPU::Relative, this);
            break;
        case OpCode::ZeroPage_BIT:
            instruction = std::bind(&CPU::BIT, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::Absolute_BIT:
            instruction = std::bind(&CPU::BIT, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Relative_BMI:
            branchCondition = IsNegative();
            tickFunction = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BNE:
            branchCondition = !IsZero();
            tickFunction = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BPL:
            branchCondition = !IsNegative();
            tickFunction = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Implied_BRK:
            tickFunction = std::bind(&CPU::BRK, this);
            break;
        case OpCode::Relative_BVC:
            branchCondition = !IsOverflow();
            tickFunction = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Relative_BVS:
            branchCondition = IsOverflow();
            tickFunction = std::bind(&CPU::Relative, this);
            break;
        case OpCode::Implied_CLC:
            instruction = std::bind(&CPU::SetCarry, this, false);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_CLD:
            instruction = std::bind(&CPU::SetDecimal, this, false);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_CLI:
            instruction = std::bind(&CPU::SetInterruptDisable, this, false);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_CLV:
            instruction = std::bind(&CPU::SetOverflow, this, false);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Immediate_CMP:
            regData = Registers.accumulator;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_CMP:
            regData = Registers.accumulator;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_CMP:
            regData = Registers.accumulator;
            index = Registers.x;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_CMP:
            regData = Registers.accumulator;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_CMP:
            regData = Registers.accumulator;
            index = Registers.x;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_CMP:
            regData = Registers.accumulator;
            index = Registers.y;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_CMP:
            regData = Registers.accumulator;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_CMP:
            regData = Registers.accumulator;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Immediate_CPX:
            regData = Registers.x;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_CPX:
            regData = Registers.x;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::Absolute_CPX:
            regData = Registers.x;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Immediate_CPY:
            regData = Registers.y;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_CPY:
            regData = Registers.y;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::Absolute_CPY:
            regData = Registers.y;
            instruction = std::bind(&CPU::CMP, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::ZeroPage_DEC:
            instruction = std::bind(&CPU::DEC, this);
            tickFunction = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_DEC:
            index = Registers.x;
            instruction = std::bind(&CPU::DEC, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_DEC:
            instruction = std::bind(&CPU::DEC, this);
            tickFunction = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_DEC:
            index = Registers.x;
            instruction = std::bind(&CPU::DEC, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Implied_DEX:
            instruction = std::bind(&CPU::DEX, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_DEY:
            instruction = std::bind(&CPU::DEY, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Immediate_EOR:
            instruction = std::bind(&CPU::EOR, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_EOR:
            instruction = std::bind(&CPU::EOR, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_EOR:
            index = Registers.x;
            instruction = std::bind(&CPU::EOR, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_EOR:
            instruction = std::bind(&CPU::EOR, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_EOR:
            index = Registers.x;
            instruction = std::bind(&CPU::EOR, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_EOR:
            index = Registers.y;
            instruction = std::bind(&CPU::EOR, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_EOR:
            instruction = std::bind(&CPU::EOR, this);
            tickFunction = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_EOR:
            instruction = std::bind(&CPU::EOR, this);
            tickFunction = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::ZeroPage_INC:
            instruction = std::bind(&CPU::INC, this);
            tickFunction = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_INC:
            index = Registers.x;
            instruction = std::bind(&CPU::INC, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_INC:
            instruction = std::bind(&CPU::INC, this);
            tickFunction = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_INC:
            index = Registers.x;
            instruction = std::bind(&CPU::INC, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Implied_INX:
            instruction = std::bind(&CPU::INX, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_INY:
            instruction = std::bind(&CPU::INY, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Absolute_JMP:
            tickFunction = std::bind(&CPU::AbsoluteJMP, this);
            break;
        case OpCode::Indirect_JMP:
            tickFunction = std::bind(&CPU::IndirectJMP, this);
            break;
        case OpCode::Absolute_JSR:
            tickFunction = std::bind(&CPU::JSR, this);
            break;
        case OpCode::Immediate_LDA:
            instruction = std::bind(&CPU::LDA, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_LDA:
            instruction = std::bind(&CPU::LDA, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_LDA:
            index = Registers.x;
            instruction = std::bind(&CPU::LDA, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_LDA:
            instruction = std::bind(&CPU::LDA, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_LDA:
            index = Registers.x;
            instruction = std::bind(&CPU::LDA, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_LDA:
            index = Registers.y;
            instruction = std::bind(&CPU::LDA, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::Indirect_X_LDA:
            instruction = std::bind(&CPU::LDA, this);
            tickFunction = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_LDA:
            instruction = std::bind(&CPU::LDA, this);
            tickFunction = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Immediate_LDX:
            instruction = std::bind(&CPU::LDX, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_LDX:
            instruction = std::bind(&CPU::LDX, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_Y_LDX:
            index = Registers.y;
            instruction = std::bind(&CPU::LDX, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_LDX:
            instruction = std::bind(&CPU::LDX, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_Y_LDX:
            index = Registers.y;
            instruction = std::bind(&CPU::LDX, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Immediate_LDY:
            instruction = std::bind(&CPU::LDY, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_LDY:
            instruction = std::bind(&CPU::LDY, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_LDY:
            index = Registers.x;
            instruction = std::bind(&CPU::LDY, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_LDY:
            instruction = std::bind(&CPU::LDY, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_LDY:
            index = Registers.x;
            instruction = std::bind(&CPU::LDY, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Accumulator_LSR:
            instruction = std::bind(&CPU::LSR, this);
            tickFunction = std::bind(&CPU::Accumulator, this);
            break;
        case OpCode::ZeroPage_LSR:
            instruction = std::bind(&CPU::LSR, this);
            tickFunction = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_LSR:
            index = Registers.x;
            instruction = std::bind(&CPU::LSR, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_LSR:
            instruction = std::bind(&CPU::LSR, this);
            tickFunction = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_LSR:
            index = Registers.x;
            instruction = std::bind(&CPU::LSR, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Implied_NOP:
            instruction = [](){};
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Immediate_ORA:
            instruction = std::bind(&CPU::ORA, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_ORA:
            instruction = std::bind(&CPU::ORA, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_ORA:
            index = Registers.x;
            instruction = std::bind(&CPU::ORA, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_ORA:
            instruction = std::bind(&CPU::ORA, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_ORA:
            index = Registers.x;
            instruction = std::bind(&CPU::ORA, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_ORA:
            index = Registers.y;
            instruction = std::bind(&CPU::ORA, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_ORA:
            instruction = std::bind(&CPU::ORA, this);
            tickFunction = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_ORA:
            instruction = std::bind(&CPU::ORA, this);
            tickFunction = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Implied_PHA:
            tickFunction = std::bind(&CPU::PHA, this);
            break;
        case OpCode::Implied_PHP:
            tickFunction = std::bind(&CPU::PHP, this);
            break;
        case OpCode::Implied_PLA:
            tickFunction = std::bind(&CPU::PLA, this);
            break;
        case OpCode::Implied_PLP:
            tickFunction = std::bind(&CPU::PLP, this);
            break;
        case OpCode::Accumulator_ROL:
            instruction = std::bind(&CPU::ROL, this);
            tickFunction = std::bind(&CPU::Accumulator, this);
            break;
        case OpCode::ZeroPage_ROL:
            instruction = std::bind(&CPU::ROL, this);
            tickFunction = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_ROL:
            index = Registers.x;
            instruction = std::bind(&CPU::ROL, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_ROL:
            instruction = std::bind(&CPU::ROL, this);
            tickFunction = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_ROL:
            index = Registers.x;
            instruction = std::bind(&CPU::ROL, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Accumulator_ROR:
            instruction = std::bind(&CPU::ROR, this);
            tickFunction = std::bind(&CPU::Accumulator, this);
            break;
        case OpCode::ZeroPage_ROR:
            instruction = std::bind(&CPU::ROR, this);
            tickFunction = std::bind(&CPU::ZeroPageRMW, this);
            break;
        case OpCode::ZeroPage_X_ROR:
            index = Registers.x;
            instruction = std::bind(&CPU::ROR, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexedRMW, this);
            break;
        case OpCode::Absolute_ROR:
            instruction = std::bind(&CPU::ROR, this);
            tickFunction = std::bind(&CPU::AbsoluteRWM, this);
            break;
        case OpCode::Absolute_X_ROR:
            index = Registers.x;
            instruction = std::bind(&CPU::ROR, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexedRMW, this);
            break;
        case OpCode::Implied_RTI:
            tickFunction = std::bind(&CPU::RTI, this);
            break;
        case OpCode::Implied_RTS:
            tickFunction = std::bind(&CPU::RTS, this);
            break;
        case OpCode::Immediate_SBC:
            instruction = std::bind(&CPU::SBC, this);
            tickFunction = std::bind(&CPU::Immediate, this);
            break;
        case OpCode::ZeroPage_SBC:
            instruction = std::bind(&CPU::SBC, this);
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_SBC:
            index = Registers.x;
            instruction = std::bind(&CPU::SBC, this);
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_SBC:
            instruction = std::bind(&CPU::SBC, this);
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_SBC:
            index = Registers.x;
            instruction = std::bind(&CPU::SBC, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_SBC:
            index = Registers.y;
            instruction = std::bind(&CPU::SBC, this);
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_SBC:
            instruction = std::bind(&CPU::SBC, this);
            tickFunction = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_SBC:
            instruction = std::bind(&CPU::SBC, this);
            tickFunction = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::Implied_SEC:
            instruction = std::bind(&CPU::SetCarry, this, true);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_SED:
            instruction = std::bind(&CPU::SetDecimal, this, true);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_SEI:
            instruction = std::bind(&CPU::SetInterruptDisable, this, true);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::ZeroPage_STA:
            isStoreOp = true;
            regData = Registers.accumulator;
            instruction = [](){};
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_STA:
            isStoreOp = true;
            regData = Registers.accumulator;
            index = Registers.x;
            instruction = [](){};
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_STA:
            isStoreOp = true;
            regData = Registers.accumulator;
            instruction = [](){};
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Absolute_X_STA:
            isStoreOp = true;
            regData = Registers.accumulator;
            index = Registers.x;
            instruction = [](){};
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Absolute_Y_STA:
            isStoreOp = true;
            regData = Registers.accumulator;
            index = Registers.y;
            instruction = [](){};
            tickFunction = std::bind(&CPU::AbsoluteIndexed, this);
            break;
        case OpCode::Indirect_X_STA:
            isStoreOp = true;
            regData = Registers.accumulator;
            instruction = [](){};
            tickFunction = std::bind(&CPU::IndirectX, this);
            break;
        case OpCode::Indirect_Y_STA:
            isStoreOp = true;
            regData = Registers.accumulator;
            instruction = [](){};
            tickFunction = std::bind(&CPU::IndirectY, this);
            break;
        case OpCode::ZeroPage_STX:
            isStoreOp = true;
            regData = Registers.x;
            instruction = [](){};
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_Y_STX:
            isStoreOp = true;
            regData = Registers.x;
            index = Registers.y;
            instruction = [](){};
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_STX:
            isStoreOp = true;
            regData = Registers.x;
            instruction = [](){};
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::ZeroPage_STY:
            isStoreOp = true;
            regData = Registers.y;
            instruction = [](){};
            tickFunction = std::bind(&CPU::ZeroPage, this);
            break;
        case OpCode::ZeroPage_X_STY:
            isStoreOp = true;
            regData = Registers.y;
            index = Registers.x;
            instruction = [](){};
            tickFunction = std::bind(&CPU::ZeroPageIndexed, this);
            break;
        case OpCode::Absolute_STY:
            isStoreOp = true;
            regData = Registers.y;
            instruction = [](){};
            tickFunction = std::bind(&CPU::Absolute, this);
            break;
        case OpCode::Implied_TAX:
            instruction = std::bind(&CPU::TAX, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TAY:
            instruction = std::bind(&CPU::TAY, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TSX:
            instruction = std::bind(&CPU::TSX, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TXA:
            instruction = std::bind(&CPU::TXA, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TXS:
            instruction = std::bind(&CPU::TXS, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        case OpCode::Implied_TYA:
            instruction = std::bind(&CPU::TYA, this);
            tickFunction = std::bind(&CPU::Implied, this);
            break;
        default:
            std::cout << "INVALID OPCODE" << (int)opCode << std::endl;
            SetNextOpCode();
    }
}
