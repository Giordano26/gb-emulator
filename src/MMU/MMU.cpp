#include "MMU.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>

MMU::MMU(ROM* rom):loadedRom(rom){
    wram.fill(0);
    hram.fill(0);
    this->interruptFlag = 0;
    this->interruptEnable = 0;

    std::ifstream bootFile("dmg_boot.bin", std::ios::binary);
    if (bootFile.is_open()) {
        bootFile.read(reinterpret_cast<char*>(this->bootRom.data()), 256);
        this->bootromEnabled = true;
        std::cout << "BIOS file found." << std::endl;
    } else {
        this->bootromEnabled = false;
        std::cout << "BIOS file missing" << std::endl;
    }
}

uint8_t MMU::read(uint16_t address){
    //refact a hashmap to each addres for each instruction maybe??
    if(address < 0x8000){ return this->loadedRom->read(address); }

    if(address >= 0xC000 && address <= 0xDFFF){ return this->wram[address - 0xC000]; }


    if(address >= 0xFF04 && address <= 0xFF07) { return this->timer.read(address); }

    if(address >= 0x8000 && address <= 0x9FFF){ return this->ppu.read(address); }
    if(address >= 0xFE00 && address <= 0xFE9F){ return this->ppu.read(address); }
    if(address == 0xFF40){ return this->ppu.read(address); }
    if(address == 0xFF41){ return this->ppu.read(address); }
    if(address == 0xFF42){ return this->ppu.read(address); }
    if(address == 0xFF43){ return this->ppu.read(address); }
    if(address == 0xFF44){ return this->ppu.read(address); }
    if(address == 0xFF45){ return this->ppu.read(address); }
    if(address == 0xFF47){ return this->ppu.read(address); }
    if(address == 0xFF48){ return this->ppu.read(address); }
    if(address == 0xFF49){ return this->ppu.read(address); }



    if(address >= 0xFF80 && address <= 0xFFFE){return this->hram[address - 0xFF80];}

    if(address == 0xFF0F){ return this->interruptFlag; }

    if(address == 0xFFFF){ return this->interruptEnable; }


    return 0xFF;
}

void MMU::write(uint16_t address, uint8_t value){
    if (address >= 0xC000 && address <= 0xDFFF) {
        this->wram[address - 0xC000] = value;
        return;
    }

    if(address >= 0x8000 && address <= 0x9FFF){
        this->ppu.write(address, value);
        return;
    }
    if(address >= 0xFE00 && address <= 0xFE9F){
        this->ppu.write(address, value);
        return;
    }
    if (address == 0xFF40){
        this->ppu.write(address, value);
        return;
    }
    if (address == 0xFF41){
        this->ppu.write(address, value);
        return;
    }
    if (address == 0xFF42){
        this->ppu.write(address, value);
        return;
    }
    if (address == 0xFF43){
        this->ppu.write(address, value);
        return;
    }
    if (address == 0xFF44) {
        this->ppu.write(address, value);
        return;
    }
    if (address == 0xFF45){
        this->ppu.write(address, value);
        return;
    }
    if(address == 0xFF47){
        this->ppu.write(address, value);
        return;
    }
    if(address == 0xFF48){
        this->ppu.write(address, value);
        return;
    }
    if(address == 0xFF49){
        this->ppu.write(address, value);
        return;
    }

    if(address >= 0xFF80 && address <= 0xFFFE){
        this->hram[address - 0xFF80] = value;
        return;
    }

    if (address == 0xFF46) {
        uint16_t source = value << 8;
        for (int i = 0; i < 160; i++) {
            uint8_t byteToCopy = this->read(source + i);
            this->write(0xFE00 + i, byteToCopy);
        }
        return;
    }

    if (address >= 0xFF04 && address <= 0xFF07) {
        this->timer.write(address, value);
        return;
    }

    if (address == 0xFF0F) {
        this->interruptFlag = value;
        return;
    }

    if (address == 0xFFFF) {
        this->interruptEnable = value;
        return;
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

    uint8_t ppuInterrupts = this->ppu.tick(cycles);

    if(ppuInterrupts & (1 << 0)){
        this->requestInterrupt(0);
    }

    if(ppuInterrupts & (1 << 1)){
        this->requestInterrupt(1);
    }
}

void MMU::requestInterrupt(uint8_t interruptBit){
    uint8_t currentIF = this->read(0xFF0F);

    currentIF = currentIF | (1 << interruptBit);

    this->write(0xFF0F, currentIF);
}
