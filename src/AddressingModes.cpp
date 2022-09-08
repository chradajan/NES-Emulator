#include <cstdint>
#include "../include/CPU.hpp"

void CPU::Immediate()
{
    switch (cycle_)
    {
        case 1:
            iData_ = ReadAndIncrementPC();
            break;
        case 2:
            instruction_();
            SetNextOpCode();
    }
}

void CPU::Absolute()
{
    switch (cycle_)
    {
        case 1:
            iData_ = ReadAndIncrementPC();
            break;
        case 2:
            iAddr_ = (ReadAndIncrementPC() << 8) | iData_;
            break;
        case 3:
            if (isStoreOp_)
            {
                Write(iAddr_, regData_);
            }
            else
            {
                iData_ = Read(iAddr_);
            }
            break;
        case 4:
            instruction_();
            SetNextOpCode();
    }
}

void CPU::ZeroPage()
{
    switch (cycle_)
    {
        case 1:
            iAddr_ = ReadAndIncrementPC();
            break;
        case 2:
            if (isStoreOp_)
            {
                Write(iAddr_, regData_);
            }
            else
            {
                iData_ = Read(iAddr_);
            }
            break;
        case 3:
            instruction_();
            SetNextOpCode();
    }
}

void CPU::Implied()
{
    switch (cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            break;
        case 2:
            instruction_();
            SetNextOpCode();
    }
}

void CPU::AbsoluteIndexed()
{
    switch (cycle_)
    {
        case 1:
            iData_ = ReadAndIncrementPC();
            break;
        case 2:
            iAddr_ = (ReadAndIncrementPC() << 8) | iData_;
            break;
        case 3:
            if (isStoreOp_ || (((iAddr_ + instructionIndex_) & PAGE_MASK) != (iAddr_ & PAGE_MASK)))
            {
                // Dummy Read
                Read((iAddr_ & PAGE_MASK) | ((iAddr_ + instructionIndex_) & ZERO_PAGE_MASK));
                iAddr_ += instructionIndex_;
            }
            else
            {
                iAddr_ += instructionIndex_;
                iData_ = Read(iAddr_);
                ++cycle_;
            }
            break;
        case 4:
            if (isStoreOp_)
            {
                Write(iAddr_, regData_);
            }
            else
            {
                iData_ = Read(iAddr_);
            }
            break;
        case 5:
            instruction_();
            SetNextOpCode();
    }
}

void CPU::ZeroPageIndexed()
{
    switch (cycle_)
    {
        case 1:
            iAddr_ = ReadAndIncrementPC();
            break;
        case 2:
            // Dummy Read
            Read(iAddr_);
            iAddr_ = (iAddr_ + instructionIndex_) & ZERO_PAGE_MASK;
            break;
        case 3:
            if (isStoreOp_)
            {
                Write(iAddr_, regData_);
            }
            else
            {
                iData_ = Read(iAddr_);
            }
            break;
        case 4:
            instruction_();
            SetNextOpCode();
    }
}

void CPU::IndirectX()
{
    switch (cycle_)
    {
        case 1:
            iData_ = ReadAndIncrementPC();
            break;
        case 2:
            // Dummy Read
            Read(iData_);
            iData_ = (iData_ + Registers_.x) & ZERO_PAGE_MASK;
            break;
        case 3:
            iAddr_ = Read(iData_);
            iData_ = (iData_ + 0x01) & ZERO_PAGE_MASK;
            break;
        case 4:
            iAddr_ = (Read(iData_) << 8) | iAddr_;
            break;
        case 5:
            if (isStoreOp_)
            {
                Write(iAddr_, regData_);
            }
            else
            {
                iData_ = Read(iAddr_);
            }
            break;
        case 6:
            instruction_();
            SetNextOpCode();
    }
}

void CPU::IndirectY()
{
    switch (cycle_)
    {
        case 1:
            iData_ = ReadAndIncrementPC();
            break;
        case 2:
            iAddr_ = Read(iData_);
            ++iData_;
            break;
        case 3:
            iAddr_ |= (Read(iData_) << 8);
            break;
        case 4:
            if (isStoreOp_ || (((iAddr_ + Registers_.y) & PAGE_MASK) != (iAddr_ & PAGE_MASK)))
            {
                // Dummy Read
                Read((iAddr_ & PAGE_MASK) | ((iAddr_ + Registers_.y) & ZERO_PAGE_MASK));
                iAddr_ += Registers_.y;
            }
            else
            {
                iAddr_ += Registers_.y;
                iData_ = Read(iAddr_);
                ++cycle_;
            }
            break;
        case 5:
            if (isStoreOp_)
            {
                Write(iAddr_, regData_);
            }
            else
            {
                iData_ = Read(iAddr_);
            }
            break;
        case 6:
            instruction_();
            SetNextOpCode();
    }
}

void CPU::Relative()
{
    switch (cycle_)
    {
        case 1:
            iData_ = ReadAndIncrementPC();
            iAddr_ = Registers_.programCounter + (int8_t)iData_;
            break;
        case 2:
            if (branchCondition_)
            {
                // Dummy Read when branch condition is true
                Read((Registers_.programCounter & PAGE_MASK) + (iAddr_ & ZERO_PAGE_MASK));
            }
            else
            {
                SetNextOpCode();
            }
            break;
        case 3:
            if ((Registers_.programCounter & PAGE_MASK) != (iAddr_ & PAGE_MASK))
            {
                // Dummy Read when page boundary crossed on branch
                Read(iAddr_);
            }
            else
            {
                Registers_.programCounter = iAddr_;
                SetNextOpCode();
            }
            break;
        case 4:
            Registers_.programCounter = iAddr_;
            SetNextOpCode();
    }
}

void CPU::Accumulator()
{
    switch (cycle_)
    {
        case 1:
            // Dummy Read
            Read(Registers_.programCounter);
            iData_ = Registers_.accumulator;
            break;
        case 2:
            instruction_();
            Registers_.accumulator = iData_;
            SetNextOpCode();
    }
}

void CPU::ZeroPageRMW()
{
    switch (cycle_)
    {
        case 1:
            iAddr_ = ReadAndIncrementPC();
            break;
        case 2:
            iData_ = Read(iAddr_);
            break;
        case 3:
            // Dummy Write
            Write(iAddr_, 0xFF);
            break;
        case 4:
            instruction_();
            Write(iAddr_, iData_);
            break;
        case 5:
            SetNextOpCode();
    }
}

void CPU::ZeroPageIndexedRMW()
{
    switch (cycle_)
    {
        case 1:
            iAddr_ = ReadAndIncrementPC();
            break;
        case 2:
            // Dummy Read
            Read(iAddr_);
            iAddr_ = (iAddr_ + instructionIndex_) % ZERO_PAGE_MASK;
            break;
        case 3:
            iData_ = Read(iAddr_);
            break;
        case 4:
            // Dummy Write
            Write(iAddr_, 0xFF);
            break;
        case 5:
            instruction_();
            Write(iAddr_, iData_);
            break;
        case 6:
            SetNextOpCode();
    }
}

void CPU::AbsoluteRWM()
{
    switch (cycle_)
    {
        case 1:
            iAddr_ = ReadAndIncrementPC();
            break;
        case 2:
            iAddr_ = (ReadAndIncrementPC() << 8) | iAddr_;
            break;
        case 3:
            iData_ = Read(iAddr_);
            break;
        case 4:
            // Dummy Write
            Write(iAddr_, 0xFF);
            break;
        case 5:
            instruction_();
            Write(iAddr_, iData_);
            break;
        case 6:
            SetNextOpCode();
    }
}

void CPU::AbsoluteIndexedRMW()
{
    switch (cycle_)
    {
        case 1:
            iData_ = ReadAndIncrementPC();
            break;
        case 2:
            iAddr_ = ReadAndIncrementPC();
            break;
        case 3:
            // Dummy Read
            Read((iAddr_ << 8) | ((iData_ + instructionIndex_) & ZERO_PAGE_MASK));
            iAddr_ = (((iAddr_ << 8) | iData_) + instructionIndex_) & 0xFFFF;
            break;
        case 4:
            iData_ = Read(iAddr_);
            break;
        case 5:
            // Dummy Write
            Write(iAddr_, 0xFF);
            break;
        case 6:
            instruction_();
            Write(iAddr_, iData_);
            break;
        case 7:
            SetNextOpCode();
    }
}
