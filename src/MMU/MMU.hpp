#pragma once
#include "ROM/ROM.hpp"
#include "PPU/PPU.hpp"
#include "Timer/Timer.hpp"
#include <array>
#include <cstdint>

constexpr int WRAM_SIZE = 8192;

class MMU {
private:
    ROM* loadedRom;
    Timer timer;

    uint8_t interruptFlag;
    uint8_t interruptEnable;

    std::array<uint8_t, 256> bootRom;
    std::array<uint8_t, WRAM_SIZE> wram;
    std::array<uint8_t, 127> hram;

    bool bootromEnabled;


public:
    MMU(ROM* rom);

    PPU ppu;

    bool hasBootRom() const {return bootromEnabled; }

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    uint16_t read16(uint16_t address);
    void write16(uint16_t address, uint16_t value);

    void tick(uint8_t cycles);
    void requestInterrupt(uint8_t interruptBit);
};
