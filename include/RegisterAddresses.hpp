#ifndef REGISTERADDRESSES_HPP
#define REGISTERADDRESSES_HPP

#include <cstdint>

// PPU
constexpr uint16_t PPUCTRL_ADDR =     0x2000;
constexpr uint16_t PPUMASK_ADDR =     0x2001;
constexpr uint16_t PPUSTATUS_ADDR =   0x2002;
constexpr uint16_t OAMADDR_ADDR =     0x2003;
constexpr uint16_t OAMDATA_ADDR =     0x2004;
constexpr uint16_t PPUSCROLL_ADDR =   0x2005;
constexpr uint16_t PPUADDR_ADDR =     0x2006;
constexpr uint16_t PPUDATA_ADDR =     0x2007;

// APU
constexpr uint16_t SQ1_VOL_ADDR =     0x4000;
constexpr uint16_t SQ1_SWEEP_ADDR =   0x4001;
constexpr uint16_t SQ1_LO_ADDR =      0x4002;
constexpr uint16_t SQ1_HI_ADDR =      0x4003;
constexpr uint16_t SQ2_VOL_ADDR =     0x4004;
constexpr uint16_t SQ2_SWEEP_ADDR =   0x4005;
constexpr uint16_t SQ2_LO_ADDR =      0x4006;
constexpr uint16_t SQ2_HI_ADDR =      0x4007;
constexpr uint16_t TRI_LINEAR_ADDR =  0x4008;
constexpr uint16_t UNUSED1_ADDR =     0x4009;
constexpr uint16_t TRI_LO_ADDR =      0x400A;
constexpr uint16_t TRI_HI_ADDR =      0x400B;
constexpr uint16_t NOISE_VOL_ADDR =   0x400C;
constexpr uint16_t UNUSED2_ADDR =     0x400D;
constexpr uint16_t NOISE_LO_ADDR =    0x400E;
constexpr uint16_t NOISE_HI_ADDR =    0x400F;
constexpr uint16_t DMC_FREQ_ADDR =    0x4010;
constexpr uint16_t DMC_RAW_ADDR =     0x4011;
constexpr uint16_t DMC_START_ADDR =   0x4012;
constexpr uint16_t DMC_LEN_ADDR =     0x4013;
constexpr uint16_t SND_CHN_ADDR =     0x4015;
constexpr uint16_t FRAME_COUNTER_ADDR = 0x4017;

// Controllers
constexpr uint16_t JOY1_ADDR =        0x4016;
constexpr uint16_t JOY2_ADDR =        0x4017;

// OAM
constexpr uint16_t OAMDMA_ADDR =      0x4014;

#endif
