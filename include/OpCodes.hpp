#ifndef OPCODE_HPP
#define OPCODE_HPP

#include <cstdint>

enum class OpCode : uint8_t
{
    Immediate_ADC   = 0x69,
    ZeroPage_ADC    = 0x65,
    ZeroPage_X_ADC  = 0x75,
    Absolute_ADC    = 0x6D,
    Absolute_X_ADC  = 0x7D,
    Absolute_Y_ADC  = 0x79,
    Indirect_X_ADC  = 0x61,
    Indirect_Y_ADC  = 0x71,
    Immediate_AND   = 0x29,
    ZeroPage_AND    = 0x25,
    ZeroPage_X_AND  = 0x35,
    Absolute_AND    = 0x2D,
    Absolute_X_AND  = 0x3D,
    Absolute_Y_AND  = 0x39,
    Indirect_X_AND  = 0x21,
    Indirect_Y_AND  = 0x31,
    Accumulator_ASL = 0x0A,
    ZeroPage_ASL    = 0x06,
    ZeroPage_X_ASL  = 0x16,
    Absolute_ASL    = 0x0E,
    Absolute_X_ASL  = 0x1E,
    Relative_BCC    = 0x90,
    Relative_BCS    = 0xB0,
    Relative_BEQ    = 0xF0,
    ZeroPage_BIT    = 0x24,
    Absolute_BIT    = 0x2C,
    Relative_BMI    = 0x30,
    Relative_BNE    = 0xD0,
    Relative_BPL    = 0x10,
    Implied_BRK     = 0x00,
    Relative_BVC    = 0x50,
    Relative_BVS    = 0x70,
    Implied_CLC     = 0x18,
    Implied_CLD     = 0xD8,
    Implied_CLI     = 0x58,
    Implied_CLV     = 0xB8,
    Immediate_CMP   = 0xC9,
    ZeroPage_CMP    = 0xC5,
    ZeroPage_X_CMP  = 0xD5,
    Absolute_CMP    = 0xCD,
    Absolute_X_CMP  = 0xDD,
    Absolute_Y_CMP  = 0xD9,
    Indirect_X_CMP  = 0xC1,
    Indirect_Y_CMP  = 0xD1,
    Immediate_CPX   = 0xE0,
    ZeroPage_CPX    = 0xE4,
    Absolute_CPX    = 0xEC,
    Immediate_CPY   = 0xC0,
    ZeroPage_CPY    = 0xC4,
    Absolute_CPY    = 0xCC,
    ZeroPage_DEC    = 0xC6,
    ZeroPage_X_DEC  = 0xD6,
    Absolute_DEC    = 0xCE,
    Absolute_X_DEC  = 0xDE,
    Implied_DEX     = 0xCA,
    Implied_DEY     = 0x88,
    Immediate_EOR   = 0x49,
    ZeroPage_EOR    = 0x45,
    ZeroPage_X_EOR  = 0x55,
    Absolute_EOR    = 0x4D,
    Absolute_X_EOR  = 0x5D,
    Absolute_Y_EOR  = 0x59,
    Indirect_X_EOR  = 0x41,
    Indirect_Y_EOR  = 0x51,
    ZeroPage_INC    = 0xE6,
    ZeroPage_X_INC  = 0xF6,
    Absolute_INC    = 0xEE,
    Absolute_X_INC  = 0xFE,
    Implied_INX     = 0xE8,
    Implied_INY     = 0xC8,
    Absolute_JMP    = 0x4C,
    Indirect_JMP    = 0x6C,
    Absolute_JSR    = 0x20,
    Immediate_LDA   = 0xA9,
    ZeroPage_LDA    = 0xA5,
    ZeroPage_X_LDA  = 0xB5,
    Absolute_LDA    = 0xAD,
    Absolute_X_LDA  = 0xBD,
    Absolute_Y_LDA  = 0xB9,
    Indirect_X_LDA  = 0xA1,
    Indirect_Y_LDA  = 0xB1,
    Immediate_LDX   = 0xA2,
    ZeroPage_LDX    = 0xA6,
    ZeroPage_Y_LDX  = 0xB6,
    Absolute_LDX    = 0xAE,
    Absolute_Y_LDX  = 0xBE,
    Immediate_LDY   = 0xA0,
    ZeroPage_LDY    = 0xA4,
    ZeroPage_X_LDY  = 0xB4,
    Absolute_LDY    = 0xAC,
    Absolute_X_LDY  = 0xBC,
    Accumulator_LSR = 0x4A,
    ZeroPage_LSR    = 0x46,
    ZeroPage_X_LSR  = 0x56,
    Absolute_LSR    = 0x4E,
    Absolute_X_LSR  = 0x5E,
    Implied_NOP     = 0xEA,
    Immediate_ORA   = 0x09,
    ZeroPage_ORA    = 0x05,
    ZeroPage_X_ORA  = 0x15,
    Absolute_ORA    = 0x0D,
    Absolute_X_ORA  = 0x1D,
    Absolute_Y_ORA  = 0x19,
    Indirect_X_ORA  = 0x01,
    Indirect_Y_ORA  = 0x11,
    Implied_PHA     = 0x48,
    Implied_PHP     = 0x08,
    Implied_PLA     = 0x68,
    Implied_PLP     = 0x28,
    Accumulator_ROL = 0x2A,
    ZeroPage_ROL    = 0x26,
    ZeroPage_X_ROL  = 0x36,
    Absolute_ROL    = 0x2E,
    Absolute_X_ROL  = 0x3E,
    Accumulator_ROR = 0x6A,
    ZeroPage_ROR    = 0x66,
    ZeroPage_X_ROR  = 0x76,
    Absolute_ROR    = 0x6E,
    Absolute_X_ROR  = 0x7E,
    Implied_RTI     = 0x40,
    Implied_RTS     = 0x60,
    Immediate_SBC   = 0xE9,
    ZeroPage_SBC    = 0xE5,
    ZeroPage_X_SBC  = 0xF5,
    Absolute_SBC    = 0xED,
    Absolute_X_SBC  = 0xFD,
    Absolute_Y_SBC  = 0xF9,
    Indirect_X_SBC  = 0xE1,
    Indirect_Y_SBC  = 0xF1,
    Implied_SEC     = 0x38,
    Implied_SED     = 0xF8,
    Implied_SEI     = 0x78,
    ZeroPage_STA    = 0x85,
    ZeroPage_X_STA  = 0x95,
    Absolute_STA    = 0x8D,
    Absolute_X_STA  = 0x9D,
    Absolute_Y_STA  = 0x99,
    Indirect_X_STA  = 0x81,
    Indirect_Y_STA  = 0x91,
    ZeroPage_STX    = 0x86,
    ZeroPage_Y_STX  = 0x96,
    Absolute_STX    = 0x8E,
    ZeroPage_STY    = 0x84,
    ZeroPage_X_STY  = 0x94,
    Absolute_STY    = 0x8C,
    Implied_TAX     = 0xAA,
    Implied_TAY     = 0xA8,
    Implied_TSX     = 0xBA,
    Implied_TXA     = 0x8A,
    Implied_TXS     = 0x9A,
    Implied_TYA     = 0x98,
    INVALID_CODE    = 0xFF
};

#endif
