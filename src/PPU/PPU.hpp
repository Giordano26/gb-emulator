#pragma once
#include <cstdint>
#include <array>
#include <sys/types.h>


constexpr int SCREEN_HEIGHT = 144;
constexpr int SCREEN_WIDTH = 160;
constexpr int V_BLANK = 10;


class PPU {

private:
    uint8_t ly;
    uint16_t scanlineCounter;
    uint8_t lcdc;
    uint8_t stat;
    uint8_t lyc;

    uint8_t bgp;
    uint8_t scy;
    uint8_t scx;

    uint8_t obp0;
    uint8_t obp1;

    std::array<uint8_t, 8192> vram;
    std::array<uint8_t, 160> oam;


    uint8_t returnNewStatMode();
    bool updateStat();

    void drawScanline();

    uint32_t colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

public:
    PPU();

    std::array<uint32_t, SCREEN_HEIGHT * SCREEN_WIDTH> frameBuffer;

    uint8_t tick(uint8_t cycles);

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
};
