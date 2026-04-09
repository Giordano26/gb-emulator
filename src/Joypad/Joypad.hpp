#pragma once
#include <cstdint>

class Joypad {
public:
    uint8_t actionButtons = 0x0F; // Start, Select, B, A
    uint8_t dirButtons = 0x0F;    // Down, Up, Left, Right

    uint8_t joypadSelect = 0x30;  //Tells which bit to read | 4-> dir && 5 ->action

    uint8_t read();
    void write(uint8_t value);
};
