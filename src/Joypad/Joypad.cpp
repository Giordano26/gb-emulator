#include "Joypad/Joypad.hpp"

void Joypad::write(uint8_t value) {
    this->joypadSelect = value & 0x30;
}

uint8_t Joypad::read() {
    uint8_t state = 0xC0 | this->joypadSelect | 0x0F;

    if ((this->joypadSelect & (1 << 4)) == 0) {
        state &= this->dirButtons;
    }

    if ((this->joypadSelect & (1 << 5)) == 0) {
        state &= this->actionButtons;
    }

    return state;
}
