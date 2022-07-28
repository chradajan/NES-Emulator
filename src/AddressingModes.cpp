#include <cstdint>
#include "../include/CPU.hpp"

void CPU::Immediate()
{
    switch(cycle)
    {
        case 1:
            iData = ReadAndIncrementPC();
            break;
        case 2:
            instruction();
            SetNextOpCode();
    }
}

void CPU::Absolute()
{
    switch(cycle)
    {
        case 1:
            iData = ReadAndIncrementPC();
            break;
        case 2:
            iAddr = (ReadAndIncrementPC() << 8) | iData;
            break;
        case 3:
            if (isStoreOp)
            {
                Write(iAddr, regData);
            }
            else
            {
                iData = Read(iAddr);
            }
            break;
        case 4:
            instruction();
            SetNextOpCode();
    }
}

void CPU::ZeroPage()
{
    switch(cycle)
    {
        case 1:
            iAddr = ReadAndIncrementPC();
            break;
        case 2:
            if (isStoreOp)
            {
                Write(iAddr, regData);
            }
            else
            {
                iData = Read(iAddr);
            }
            break;
        case 3:
            instruction();
            SetNextOpCode();
    }
}

void CPU::Implied()
{
    switch(cycle)
    {
        case 1:
            // Dummy Read
            Read(Registers.programCounter);
            break;
        case 2:
            instruction();
            SetNextOpCode();
    }
}

void CPU::AbsoluteIndexed()
{
    switch(cycle)
    {
        case 1:
            iData = ReadAndIncrementPC();
            break;
        case 2:
            iAddr = (ReadAndIncrementPC() << 8) | iData;
            break;
        case 3:
            if (isStoreOp || (((iAddr + index) & PAGE_MASK) != (iAddr & PAGE_MASK)))
            {
                // Dummy Read
                Read((iAddr & PAGE_MASK) | ((iAddr + index) & ZERO_PAGE_MASK));
                iAddr += index;
            }
            else
            {
                iAddr += index;
                iData = Read(iAddr);
                ++cycle;
            }
            break;
        case 4:
            if (isStoreOp)
            {
                Write(iAddr, regData);
            }
            else
            {
                iData = Read(iAddr);
            }
            break;
        case 5:
            instruction();
            SetNextOpCode();
    }
}

void CPU::ZeroPageIndexed()
{
    switch(cycle)
    {
        case 1:
            iAddr = ReadAndIncrementPC();
            break;
        case 2:
            // Dummy Read
            Read(iAddr);
            iAddr = (iAddr + index) & ZERO_PAGE_MASK;
            break;
        case 3:
            if (isStoreOp)
            {
                Write(iAddr, regData);
            }
            else
            {
                iData = Read(iAddr);
            }
            break;
        case 4:
            instruction();
            SetNextOpCode();
    }
}

void CPU::IndirectX()
{
    switch(cycle)
    {
        case 1:
            iData = ReadAndIncrementPC();
            break;
        case 2:
            // Dummy Read
            Read(iData);
            iData = (iData + Registers.x) & ZERO_PAGE_MASK;
            break;
        case 3:
            iAddr = Read(iData);
            iData = (iData + 0x01) & ZERO_PAGE_MASK;
            break;
        case 4:
            iAddr = (Read(iData) << 8) | iAddr;
            break;
        case 5:
            if (isStoreOp)
            {
                Write(iAddr, regData);
            }
            else
            {
                iData = Read(iAddr);
            }
            break;
        case 6:
            instruction();
            SetNextOpCode();
    }
}

void CPU::IndirectY()
{
    switch(cycle)
    {
        case 1:
            iData = ReadAndIncrementPC();
            break;
        case 2:
            iAddr = Read(iData);
            iData = (iData + Registers.y) & ZERO_PAGE_MASK;
            break;
        case 3:
            iAddr = (Read(iData) << 8) | iAddr;
            break;
        case 4:
            if (isStoreOp || (((iAddr + Registers.y) & PAGE_MASK) != (iAddr & PAGE_MASK)))
            {
                Read((iAddr & PAGE_MASK) + ((iAddr + Registers.y) & ZERO_PAGE_MASK));
                iAddr = (iAddr + Registers.y) & 0xFFFF;
            }
            else
            {
                iAddr += Registers.y;
                iData = Read(iAddr);
                ++cycle;
            }
            break;
        case 5:
            if (isStoreOp)
            {
                Write(iAddr, regData);
            }
            else
            {
                iData = Read(iAddr);
            }
            break;
        case 6:
            instruction();
            SetNextOpCode();
    }
}

void CPU::Relative()
{
    switch(cycle)
    {
        case 1:
            iData = ReadAndIncrementPC();
            iAddr = Registers.programCounter + (int8_t)iData;
            break;
        case 2:
            if (branchCondition)
            {
                // Dummy Read when branch condition is true
                Read((Registers.programCounter & PAGE_MASK) + (iAddr & ZERO_PAGE_MASK));
            }
            else
            {
                SetNextOpCode();
            }
            break;
        case 3:
            if ((Registers.programCounter & PAGE_MASK) != (iAddr & PAGE_MASK))
            {
                // Dummy Read when page boundary crossed on branch
                Read(iAddr);
            }
            else
            {
                Registers.programCounter = iAddr;
                SetNextOpCode();
            }
            break;
        case 4:
            Registers.programCounter = iAddr;
            SetNextOpCode();
    }
}

void CPU::Accumulator()
{
    switch(cycle)
    {
        case 1:
            // Dummy Read
            Read(Registers.programCounter);
            iData = Registers.accumulator;
            break;
        case 2:
            instruction();
            Registers.accumulator = iData;
            SetNextOpCode();
    }
}

void CPU::ZeroPageRMW()
{
    switch(cycle)
    {
        case 1:
            iAddr = ReadAndIncrementPC();
            break;
        case 2:
            iData = Read(iAddr);
            break;
        case 3:
            // Dummy Write
            Write(iAddr, 0xFF);
            break;
        case 4:
            instruction();
            Write(iAddr, iData);
            break;
        case 5:
            SetNextOpCode();
    }
}

void CPU::ZeroPageIndexedRMW()
{
    switch(cycle)
    {
        case 1:
            iAddr = ReadAndIncrementPC();
            break;
        case 2:
            // Dummy Read
            Read(iAddr);
            iAddr = (iAddr + index) % ZERO_PAGE_MASK;
            break;
        case 3:
            iData = Read(iAddr);
            break;
        case 4:
            // Dummy Write
            Write(iAddr, 0xFF);
            break;
        case 5:
            instruction();
            Write(iAddr, iData);
            break;
        case 6:
            SetNextOpCode();
    }
}

void CPU::AbsoluteRWM()
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
            break;
        case 4:
            // Dummy Write
            Write(iAddr, 0xFF);
            break;
        case 5:
            instruction();
            Write(iAddr, iData);
            break;
        case 6:
            SetNextOpCode();
    }
}

void CPU::AbsoluteIndexedRMW()
{
    switch(cycle)
    {
        case 1:
            iData = ReadAndIncrementPC();
            break;
        case 2:
            iAddr = ReadAndIncrementPC();
            break;
        case 3:
            // Dummy Read
            Read((iAddr << 8) | ((iData + index) & ZERO_PAGE_MASK));
            iAddr = (((iAddr << 8) | iData) + index) & 0xFFFF;
            break;
        case 4:
            iData = Read(iAddr);
            break;
        case 5:
            // Dummy Write
            Write(iAddr, 0xFF);
            break;
        case 6:
            instruction();
            Write(iAddr, iData);
            break;
        case 7:
            SetNextOpCode();
    }
}
