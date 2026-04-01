#include "MMU.hpp"
#include <cstdint>

MMU::MMU(ROM* rom):loadedRom(rom){
    wram.fill(0);
}

uint8_t MMU::read(uint16_t address){
    if(address < 0x8000){
        return loadedRom->read(address);
    }

    if(address >= 0xC000 && address <= 0xDFFF){
        return wram[address - 0xC000];
    }

    if (address >= 0xFF04 && address <= 0xFF07) {
        return this->timer.read(address);
    }

    return 0xFF;
}

void MMU::write(uint16_t address, uint8_t value){
    if (address >= 0xC000 && address <= 0xDFFF) {
        wram[address - 0xC000] = value;
    }
}

uint16_t MMU::read16(uint16_t address){
    uint8_t low = this->read(address);
    uint8_t high = this->read(address + 1);
    return (high << 8) | low;
}

void MMU::write16(uint16_t address, uint16_t value){
    this->write(address, value & 0xFF);
    this->write(address+1, (value >> 8) & 0xFF);
}

void MMU::tick(uint8_t cycles){
    bool timerOverflowed = this->timer.update(cycles);

    if (timerOverflowed) {
        this->requestInterrupt(2);
    }
}

void MMU::requestInterrupt(uint8_t interruptBit){
    uint8_t currentIF = this->read(0xFF0F);

    currentIF = currentIF | (1 << interruptBit);

    this->write(0xFF0F, currentIF);
}
