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
    //ROM AND BOOT ROM
    if(address < 0x8000){
        if(this->bootromEnabled && address < 0x0100){
            return this->bootRom[address];
        }

        return this->loadedRom->read(address);
    }

    //VRAM AREA
    if (address >= 0x8000 && address <= 0x9FFF) {
        return this->ppu.read(address);
    }

    //CARTRIDGE RAM AREA
    if (address >= 0xA000 && address <= 0xBFFF) {
        return this->loadedRom->read(address);
    }

    //WORKING RAM AREA
    if (address >= 0xC000 && address <= 0xDFFF) {
        return this->wram[address - 0xC000];
    }

    //ECHO RAM AREA
    if (address >= 0xE000 && address <= 0xFDFF) {
        return this->wram[address - 0xE000];
    }

    //OAM AREA
    if (address >= 0xFE00 && address <= 0xFE9F) {
        return this->ppu.read(address);
    }

    //I/O AREA
    if (address >= 0xFF00 && address <= 0xFF7F) {
        //Joypad
        if (address == 0xFF00) return this->joypad.read();
        //Timer
        if (address >= 0xFF04 && address <= 0xFF07) return this->timer.read(address);

        //Interrupt Flag
        if (address == 0xFF0F) return this->interruptFlag;

        //PPU Output
        if (address >= 0xFF40 && address <= 0xFF4B) return this->ppu.read(address);

        return 0xFF;
    }

    //HIGH RAM AREA
    if (address >= 0xFF80 && address <= 0xFFFE) {
        return this->hram[address - 0xFF80];
    }

    if (address == 0xFFFF) {
        return this->interruptEnable;
    }

    return 0xFF;
}

void MMU::write(uint16_t address, uint8_t value){
    //MBC area
    if (address < 0x8000) {
        this->loadedRom->write(address, value);
        return;
    }

    //VRAM area
    if (address >= 0x8000 && address <= 0x9FFF) {
        this->ppu.write(address, value);
        return;
    }

    //CARTDRIGE RAM AREA
    if (address >= 0xA000 && address <= 0xBFFF) {
        this->loadedRom->write(address, value);
        return;
    }

    //WORKING RAM AREA
    if (address >= 0xC000 && address <= 0xDFFF) {
        this->wram[address - 0xC000] = value;
        return;
    }

    //Echo RAM
    if (address >= 0xE000 && address <= 0xFDFF) {
        this->wram[address - 0xE000] = value;
        return;
    }

    //OAM AREA
    if (address >= 0xFE00 && address <= 0xFE9F) {
        this->ppu.write(address, value);
        return;
    }

    //I/O AREA
    if (address >= 0xFF00 && address <= 0xFF7F) {
        //Joypad
        if (address == 0xFF00) {
            this->joypad.write(value);
            return;
        }

        // Timer
        if (address >= 0xFF04 && address <= 0xFF07) {
            this->timer.write(address, value);
            return;
        }

        // Interrupt Flag
        if (address == 0xFF0F) {
            this->interruptFlag = value;
            return;
        }

        // DMA Transfer
        if (address == 0xFF46) {
            uint16_t source = value << 8;
            for (int i = 0; i < 160; i++) {
                this->write(0xFE00 + i, this->read(source + i));
            }
            return;
        }

        // BOOT ROM OFF
        if (address == 0xFF50 && value != 0) {
            this->bootromEnabled = false;
            return;
        }

        //PPU OUTPUT
        if (address >= 0xFF40 && address <= 0xFF4B) {
            this->ppu.write(address, value);
            return;
        }

        return;
    }

    //HIGH RAM
    if (address >= 0xFF80 && address <= 0xFFFE) {
        this->hram[address - 0xFF80] = value;
        return;
    }

    //IE
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
