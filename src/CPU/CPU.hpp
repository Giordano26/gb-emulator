#pragma once
#include "MMU/MMU.hpp"
#include <cstdint>
#include <sys/types.h>

class CPU {
private:
    MMU* mmu;

    uint8_t a, f;
    uint8_t b, c;
    uint8_t d, e;
    uint8_t h, l;

    uint16_t sp;
    uint16_t pc;

    bool isHalted;

    bool ime;

    void setZeroFlag(bool state);
    bool getZeroFlag();

    void setSubtractFlag(bool state);
    bool getSubtractFlag();

    void setHalfCarryFlag(bool state);
    bool getHalfCarryFlag();

    void setCarryFlag(bool state);
    bool getCarryFlag();

    uint16_t getNextd16();

    void inc8(uint8_t& reg);
    void dec8(uint8_t& reg);

    void addHL16(uint16_t& firstPair, uint16_t secondPair);

    void add8(uint8_t& firstReg, uint8_t secondReg);
    void adc8(uint8_t& firstReg, uint8_t secondReg);

    void sub8(uint8_t& firstReg, uint8_t secondReg);
    void sbc8(uint8_t& firstReg, uint8_t secondReg);

    void and8(uint8_t& firstReg, uint8_t secondReg);
    void xor8(uint8_t& firstReg, uint8_t secondReg);
    void or8(uint8_t& firstReg, uint8_t secondReg);

    void cp8(uint8_t firstReg, uint8_t secondReg);

    uint16_t pop();
    void push(uint16_t value);

    void rst(uint16_t address);

    uint8_t getNextByte();
    uint8_t decode(uint8_t opCode);
    uint8_t decodePrefix(uint8_t cbOpcode);

    void rlc(uint8_t& reg);
    void rrc(uint8_t& reg);

    void rl(uint8_t& reg);
    void rr(uint8_t& reg);

    void sla(uint8_t& reg);
    void sra(uint8_t& reg);

    void swap(uint8_t& reg);
    void srl(uint8_t& reg);

    void bit(uint8_t bitPosition, uint8_t reg);

    void res(uint8_t bitPosition, uint8_t& reg);

    void set(uint8_t bitPosition, uint8_t& reg);

    void handleInterrupts();

    const uint8_t opCycles[256] = {
        4, 12, 8, 8, 4, 4, 8, 4, 20, 8, 8, 8, 4, 4, 8, 4, // 0x00 - 0x0F
        4, 12, 8, 8, 4, 4, 8, 4, 12, 8, 8, 8, 4, 4, 8, 4, // 0x10 - 0x1F
        8, 12, 8, 8, 4, 4, 8, 4, 8,  12, 8, 8, 4, 4, 8, 4, // 0x20 - 0x2F
        8, 12, 8, 8, 12, 12, 12, 4, 8, 12, 8, 8, 4, 4, 8, 4, // 0x30 - 0x3F
        4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 0x40 - 0x4F
        4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 0x50 - 0x5F
        4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 0x60 - 0x6F
        8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, // 0x70 - 0x7F
        4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 0x80 - 0x8F
        4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 0x90 - 0x9F
        4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 0xA0 - 0xAF
        4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, // 0xB0 - 0xBF
        8, 12, 12, 16, 12, 16, 8, 16, 8, 16, 12, 4, 12, 24, 8, 16, // 0xC0 - 0xCF
        8, 12, 12, 0,  12, 16, 8, 16, 8, 16, 12, 0, 12, 0,  8, 16, // 0xD0 - 0xDF
        12, 12, 8, 0,  0,  16, 8, 16, 16, 4, 16, 0, 0,  0,  8, 16, // 0xE0 - 0xEF
        12, 12, 8, 4,  0,  16, 8, 16, 12, 8, 16, 4, 0,  0,  8, 16  // 0xF0 - 0xFF
    };

    const uint8_t cbCycles[256] = {
        // 0x00 - 0x3F:
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        // 0x40 - 0x7F:
        8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8,
        8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8,
        8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8,
        8, 8, 8, 8, 8, 8, 12, 8, 8, 8, 8, 8, 8, 8, 12, 8,
        // 0x80 - 0xFF:
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
        8, 8, 8, 8, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8
    };


public:
    CPU(MMU* mmuPtr);

    void reset();
    void runStep();

    uint16_t getAF();
    void setAF(uint16_t value);

    uint16_t getBC();
    void setBC(uint16_t value);

    uint16_t getDE();
    void setDE(uint16_t value);

    uint16_t getHL();
    void setHL(uint16_t value);
};
