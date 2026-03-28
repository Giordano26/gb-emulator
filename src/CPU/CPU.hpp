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


public:
    CPU(MMU* mmuPtr);

    void reset();
    uint8_t getNextByte();
    void runStep();
    void decode(uint8_t opCode);

    uint16_t getAF();
    void setAF(uint16_t value);

    uint16_t getBC();
    void setBC(uint16_t value);

    uint16_t getDE();
    void setDE(uint16_t value);

    uint16_t getHL();
    void setHL(uint16_t value);
};
