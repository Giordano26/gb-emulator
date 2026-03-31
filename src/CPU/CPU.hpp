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
    void decode(uint8_t opCode);
    void decodePrefix(uint8_t cbOpcode);

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
