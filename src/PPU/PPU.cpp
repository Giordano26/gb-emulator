#include "PPU/PPU.hpp"
#include <cstdint>

PPU::PPU(){
    this->oam.fill(0);
    this->vram.fill(0);

    this->scanlineCounter = 0;
    this->ly = 0;
    this->lcdc = 0x91;
    this->stat = 0x85;
    this->lyc = 0x00;
    this->bgp = 0xFC;
    this->obp0 = 0xFF;
    this->obp1 = 0xFF;
    this->scy = 0;
    this->scx = 0;
}

uint8_t PPU::returnNewStatMode(){
    if(this->ly >= 144) return 1;

    if(this->scanlineCounter >= 252) return 0;

    if(this->scanlineCounter >= 80) return 3;

    if(this->scanlineCounter < 80 && this->scanlineCounter >= 0) return 2;

    return 255;
}

bool PPU::updateStat(){
    bool requestStatInterrupt = false;
    uint8_t currentMode = this->stat & 0x03;
    uint8_t newMode = this->returnNewStatMode();

    if(currentMode != newMode){
        this->stat = (this->stat & 0xFC) | newMode;
        if(newMode == 3){
            this->drawScanline();
        }

        if (newMode == 0 && (this->stat & (1 << 3))) requestStatInterrupt = true;
        if (newMode == 1 && (this->stat & (1 << 4))) requestStatInterrupt = true;
        if (newMode == 2 && (this->stat & (1 << 5))) requestStatInterrupt = true;
    }


    bool currentCoincidence = (this->stat & (1 << 2)) != 0;
    bool newCoincidence = (this->ly == this->lyc);

    if(newCoincidence){
        this->stat = this->stat | (1 << 2);

        if (!currentCoincidence && (this->stat & (1 << 6))) requestStatInterrupt = true;
    } else {
        this->stat = this->stat & ~(1 << 2);
    }

    return requestStatInterrupt;
}

uint8_t PPU::tick(uint8_t cycles){
    uint8_t interrupts = 0;
    bool lcdEnabled = (this->lcdc & (1 << 7)) != 0;

    if(!lcdEnabled){
        this->scanlineCounter = 0;
        this->ly = 0;
        this->stat = (this->stat & 0xFC) | 0;
        return interrupts;
    }

    this->scanlineCounter += cycles;

    if(this->updateStat()) interrupts |= (1<<1);

    if(this->scanlineCounter >= 456){
        this->scanlineCounter -= 456;
        this->ly++;

        if(this->ly == SCREEN_HEIGHT){
            interrupts |= (1 << 0);
        } else if (this->ly >= (SCREEN_HEIGHT + V_BLANK)){
            this->ly = 0;
        }
    }
    return interrupts;
}

void PPU::drawScanline() {
    bool backgroundEnabled = (this->lcdc & (1 << 0)) != 0;

    if (backgroundEnabled) {
        this->drawBackground();
    } else {
        for (int pixel = 0; pixel < SCREEN_WIDTH; pixel++) {
            this->frameBuffer[this->ly * SCREEN_WIDTH + pixel] = this->colors[0];
        }
    }

    bool spritesEnabled = (this->lcdc & (1 << 1)) != 0;
    if (spritesEnabled) {
        this->drawSprites();
    }
}

void PPU::drawBackground() {
    uint16_t tileMap = (this->lcdc & (1 << 3)) != 0 ? 0x9C00 : 0x9800;
    uint16_t tileData = (this->lcdc & (1 << 4)) != 0 ? 0x8000 : 0x8800;
    bool isUnsigned = (tileData == 0x8000);

    uint8_t yPos = this->ly + this->scy;
    uint16_t tileRow = (yPos / 8) * 32;

    for (int pixel = 0; pixel < SCREEN_WIDTH; pixel++) {
        uint8_t xPos = pixel + this->scx;
        uint16_t tileCol = (xPos / 8);

        uint16_t tileAddress = tileMap + tileRow + tileCol;
        uint8_t tileID = this->vram[tileAddress - 0x8000];

        uint16_t tileDataLocation;
        if (isUnsigned) {
            tileDataLocation = tileData + (tileID * 16);
        } else {
            int8_t signedID = (int8_t)tileID;
            tileDataLocation = 0x9000 + (signedID * 16);
        }

        uint8_t line = yPos % 8;
        uint8_t data1 = this->vram[(tileDataLocation + (line * 2)) - 0x8000];
        uint8_t data2 = this->vram[(tileDataLocation + (line * 2) + 1) - 0x8000];

        int colorBit = 7 - (xPos % 8);
        uint8_t colorBit1 = (data1 >> colorBit) & 0x01;
        uint8_t colorBit2 = (data2 >> colorBit) & 0x01;
        uint8_t colorID = (colorBit2 << 1) | colorBit1;

        uint8_t realColor = (this->bgp >> (colorID * 2)) & 0x03;
        this->frameBuffer[this->ly * SCREEN_WIDTH + pixel] = this->colors[realColor];
    }
}

void PPU::drawSprites() {
    int spriteHeight = (this->lcdc & (1 << 2)) != 0 ? 16 : 8;

    for (int i = 0; i < 40; i++) {
        uint8_t index = i * 4;

        int yPos = this->oam[index] - 16;
        int xPos = this->oam[index + 1] - 8;
        uint8_t tileLocation = this->oam[index + 2];
        uint8_t attributes = this->oam[index + 3];

        bool yFlip = (attributes & (1 << 6)) != 0;
        bool xFlip = (attributes & (1 << 5)) != 0;

        if (this->ly >= yPos && this->ly < (yPos + spriteHeight)) {
            int line = this->ly - yPos;

            if (yFlip) {
                line = (spriteHeight - 1) - line;
            }

            if (spriteHeight == 16) {
                tileLocation = tileLocation & 0xFE;
            }

            uint16_t dataAddress = 0x8000 + (tileLocation * 16) + (line * 2);
            uint8_t data1 = this->vram[dataAddress - 0x8000];
            uint8_t data2 = this->vram[dataAddress + 1 - 0x8000];

            for (int tilePixel = 7; tilePixel >= 0; tilePixel--) {
                int colorBit = tilePixel;

                if (xFlip) {
                    colorBit = 7 - colorBit;
                }

                uint8_t colorBit1 = (data1 >> colorBit) & 0x01;
                uint8_t colorBit2 = (data2 >> colorBit) & 0x01;
                uint8_t colorID = (colorBit2 << 1) | colorBit1;

                if (colorID == 0) continue;

                uint8_t palette = (attributes & (1 << 4)) != 0 ? this->obp1 : this->obp0;
                uint8_t realColor = (palette >> (colorID * 2)) & 0x03;

                int xPix = xPos + (7 - tilePixel);

                if (xPix >= 0 && xPix < SCREEN_WIDTH) {
                    this->frameBuffer[this->ly * SCREEN_WIDTH + xPix] = this->colors[realColor];
                }
            }
        }
    }
}

uint8_t PPU::read(uint16_t address) {

    if(address >= 0x8000 && address <= 0x9FFF){
        return this->vram[address - 0x8000];
    }

    if(address >= 0xFE00 && address <= 0xFE9F){
        return this->oam[address - 0xFE00];
    }

    switch (address) {
        case 0xFF40: return this->lcdc;
        case 0xFF41: return this->stat;
        case 0xFF42: return this->scy;
        case 0xFF43: return this->scx;
        case 0xFF44: return this->ly;
        case 0xFF45: return this->lyc;
        case 0xFF47: return this->bgp;
        case 0xFF48: return this->obp0;
        case 0xFF49: return this->obp1;
    }

    return 0xFF;
}

void PPU::write(uint16_t address, uint8_t value) {
    if(address >= 0x8000 && address <= 0x9FFF){
        this->vram[address - 0x8000] = value;
        return;
    }

    if(address >= 0xFE00 && address <= 0xFE9F){
        this->oam[address - 0xFE00] = value;
        return;
    }

    switch (address) {
        case 0xFF40: this->lcdc = value; break;
        case 0xFF41:
            this->stat = (value & 0xF8) | (this->stat & 0x07);
            break;
        case 0xFF42: this->scy = value; break;
        case 0xFF43: this->scx = value; break;
        case 0xFF44: break; //ly read only
        case 0xFF45: this->lyc = value; break;
        case 0xFF47: this->bgp = value; break;
        case 0xFF48: this->obp0 = value; break;
        case 0xFF49: this->obp1 = value; break;
    }

}
