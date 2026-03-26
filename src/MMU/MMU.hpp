#pragma once
#include "ROM/ROM.hpp"
#include <array>
#include <cstdint>

constexpr int WRAM_SIZE = 8192;

class MMU {
private:
    ROM* loadedRom;
    std::array<uint8_t, WRAM_SIZE> wram;

public:
    MMU(ROM* rom);

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
};
