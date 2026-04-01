#pragma once
#include "ROM/ROM.hpp"
#include "TIMER/Timer.hpp"
#include <array>
#include <cstdint>

constexpr int WRAM_SIZE = 8192;

class MMU {
private:
    ROM* loadedRom;
    std::array<uint8_t, WRAM_SIZE> wram;

    Timer timer;

public:
    MMU(ROM* rom);

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    uint16_t read16(uint16_t address);
    void write16(uint16_t address, uint16_t value);

    void tick(uint8_t cycles);
    void requestInterrupt(uint8_t interruptBit);
};
