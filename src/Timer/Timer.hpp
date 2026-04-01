#pragma once
#include <cstdint>


class Timer {

private:
    uint8_t div;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;

    uint16_t divCounter;
    uint16_t timaCounter;

public:
    Timer();

    bool update(uint8_t cycles);

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

};
