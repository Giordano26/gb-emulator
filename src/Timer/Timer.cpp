#include "Timer/Timer.hpp"
#include <cstdint>

Timer::Timer(){
    this->div = 0;
    this->tima = 0;
    this->tma = 0;
    this->tac = 0;
    this->divCounter = 0;
    this->timaCounter = 0;
}

uint8_t Timer::read(uint16_t address) {
    switch (address) {
        case 0xFF04: return this->div;
        case 0xFF05: return this->tima;
        case 0xFF06: return this->tma;
        case 0xFF07: return this->tac;
        default: return 0xFF;
    }
}

void Timer::write(uint16_t address, uint8_t value) {
    switch (address) {
        case 0xFF04:
            this->div = 0;
            this->divCounter = 0;
            break;
        case 0xFF05:
            this->tima = value;
            break;
        case 0xFF06:
            this->tma = value;
            break;
        case 0xFF07:
            this->tac = value & 0x07;
            break;
    }
}

bool Timer::update(uint8_t cycles) {
    bool requestInterrupt = false;
    this->divCounter += cycles;

    while (this->divCounter >= 256) {
        this->divCounter -= 256;
        this->div++;
    }

    bool timaDisabled = (this->tac & (1 << 2)) == 0;
    if(timaDisabled){
        return requestInterrupt;
    }

    this->timaCounter += cycles;

    uint8_t clockSelect = this->tac & 0x03;
    uint16_t limit = 1024;

    switch (clockSelect) {
        case 0: limit = 1024; break;
        case 1: limit = 16; break;
        case 2: limit = 64; break;
        case 3: limit = 256; break;
    }

    while(this->timaCounter >= limit){
        this->timaCounter -= limit;

        if(this->tima == 0xFF){
            this->tima = this->tma;
            requestInterrupt = true;
        } else {
            this->tima++;
        }
    }

    return requestInterrupt;
}
