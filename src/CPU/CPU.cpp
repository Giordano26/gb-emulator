#include "CPU.hpp"
#include "MMU/MMU.hpp"
#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include <cstdlib>
#include <iomanip>

CPU::CPU(MMU* mmuPtr): mmu(mmuPtr){
    this->isHalted = false;
}

uint16_t CPU::getAF(){
    return (this->a << 8) | this->f;
}

void CPU::setAF(uint16_t value){
    this->a = (value >> 8) & 0xFF;
    this->f = value & 0xF0;
}

uint16_t CPU::getBC(){
    return (this->b << 8) | this->c;
}

void CPU::setBC(uint16_t value){
    this->b = (value >> 8) & 0xFF;
    this->c = value & 0xFF;
}

uint16_t CPU::getDE(){
    return (this->d << 8) | this->e;
}

void CPU::setDE(uint16_t value){
    this->d = (value >> 8) & 0xFF;
    this->e = value & 0xFF;
}

uint16_t CPU::getHL(){
    return (this->h << 8) | this->l;
}

void CPU::setHL(uint16_t value){
    this->h = (value >> 8) & 0xFF;
    this->l = value & 0xFF;
}

void CPU::reset(){
    this->pc = 0x0100;
}

uint8_t CPU::getNextByte(){
    uint8_t nextByte = this->mmu->read(this->pc);

    std::cout << "0x" <<
    std::hex << std::setfill('0') << std::setw(2)
    << static_cast<int>(nextByte) << std::endl;

    this->pc++;

    return nextByte;
}

void CPU::runStep(){
    if(this->isHalted) return;

    uint8_t opCode = getNextByte();
    this->decode(opCode);
}

void CPU::setZeroFlag(bool state){
    if(state){
        this->f = this->f | (1 << 7);
        return;
    }

    this->f = this->f & ~(1 << 7);
}

bool CPU::getZeroFlag(){
    return (this->f & (1 << 7)) != 0;
}

void CPU::setSubtractFlag(bool state){
    if(state){
        this->f = this->f | (1 << 6);
        return;
    }

    this->f = this->f & ~(1 << 6);
}

bool CPU::getSubtractFlag(){
    return (this->f & (1 << 6)) != 0;
}

void CPU::setHalfCarryFlag(bool state){
    if(state){
        this->f = this->f | (1 << 5);
        return;
    }

    this->f = this->f & ~(1 << 5);
}

bool CPU::getHalfCarryFlag(){
    return (this->f & (1 << 5)) != 0;
}

void CPU::setCarryFlag(bool state){
    if(state){
        this->f = this->f | (1 << 4);
        return;
    }

    this->f = this->f & ~(1 << 4);
}

bool CPU::getCarryFlag(){
    return (this->f & 0x10) != 0;
}

uint16_t CPU::getNextd16(){
    uint8_t lowByte = getNextByte();
    uint8_t highByte = getNextByte();
    return (highByte << 8) | lowByte;
}

void CPU::inc8(uint8_t& reg){
    this->setHalfCarryFlag((reg & 0x0F) == 0x0F);

    reg++;

    this->setZeroFlag(reg == 0x00);
    this->setSubtractFlag(false);
}

void CPU::dec8(uint8_t& reg){
    this->setHalfCarryFlag((reg & 0x0F) == 0x00);

    reg--;

    this->setZeroFlag(reg == 0x00);
    this->setSubtractFlag(true);
}

void CPU::addHL16(uint16_t& firstPair, uint16_t& secondPair){
    this->setHalfCarryFlag((((firstPair & 0x0FFF) + (secondPair & 0x0FFF)) > 0x0FFF));

    uint32_t pairSum = firstPair + secondPair;
    this->setCarryFlag(pairSum > 0xFFFF);

    this->setSubtractFlag(false);

    setHL(pairSum & 0xFFFF);
}

void CPU::decode(uint8_t opCode){
    switch (opCode) {
        case 0x00:
            break;

        case 0x01: {
            this->setBC(getNextd16());
            break;
        }

        case 0x02: {
            this->mmu->write(getBC(), this->a);
            break;
        }

        case 0x03: {
            this->setBC(this->getBC() + 1);
            break;
        }

        case 0x04: {
            this->inc8(this->b);
            break;
        }

        case 0x05: {
            this->dec8(this->b);
            break;
        }

        case 0x06: {
            this->b = getNextByte();
            break;
        }

        case 0x07: {
            uint8_t bit7 = (this->a >> 7);
            this->setCarryFlag(bit7);

            this->a = this->a << 1;

            this->a = this->a | bit7;

            setZeroFlag(false);
            setSubtractFlag(false);
            setHalfCarryFlag(false);
            break;
        }

        case 0x08: {
            this->mmu->write16(getNextd16(),this->pc);
            break;
        }

        case 0x09: {
            uint16_t hl = getHL();
            uint16_t bc = getBC();

            this->addHL16(hl, bc);
            break;
        }

        case 0x0A : {
            this->a = this->mmu->read(getBC());
            break;
        }

        case 0x0B : {
            this->setBC(this->getBC() - 1);
            break;
        }

        case 0x0C: {
            this->inc8(this->c);
            break;
        }

        case 0x0D: {
            this->dec8(this->c);
            break;
        }

        case 0x0E: {
            this->c = this->getNextByte();
            break;
        }

        case 0x0F: {
            uint8_t bit0 = this->a & 0x01;
            setCarryFlag(bit0);

            this->a = this->a >> 1;

            this->a = this->a | (bit0 << 7);

            setZeroFlag(false);
            setSubtractFlag(false);
            setHalfCarryFlag(false);

            break;
        }

        case 0x10: {
            this->getNextByte();
            break;
        }

        case 0x11: {
            this->setDE(this->getNextd16());
            break;
        }

        case 0x12: {
            this->mmu->write(getDE(), this->a);
            break;
        }

        case 0x13: {
            this->setDE(this->getDE() + 1);
            break;
        }

        case 0x14: {
            this->inc8(this->d);
            break;
        }

        case 0x15: {
            this->dec8(this->d);
            break;
        }

        case 0x16: {
            this->d = this->getNextByte();
            break;
        }

        case 0x17: {
            uint8_t bit7 = (this->a >> 7) & 0x01;
            uint8_t oldCarry = this->getCarryFlag() ? 1 : 0;

            this->a = this->a << 1;
            this->a = this->a | oldCarry;

            this->setCarryFlag(bit7);
            setZeroFlag(false);
            setSubtractFlag(false);
            setHalfCarryFlag(false);

            break;
        }

        case 0x18: {
            int8_t offset = (int8_t)this->getNextByte();

            this->pc += offset;
            break;
        }

        case 0x19: {
            uint16_t hl = getHL();
            uint16_t de = getDE();

            this->addHL16(hl, de);
            break;
        }

        case 0x1A: {
            this->a = this->mmu->read(getDE());
            break;
        }

        case 0x1B: {
            this->setDE(this->getDE() - 1);
            break;
        }

        case 0x1C: {
            inc8(this->e);
            break;
        }

        case 0x1D: {
            this->dec8(this->e);
            break;
        }

        case 0x1E: {
            this->e = getNextByte();
            break;
        }

        case 0x1F: {
            uint8_t bit0 = this->a & 0x01;
            uint8_t oldCarry = this->getCarryFlag() ? 1 : 0;

            this->a = this->a >> 1;
            this->a = this->a | (oldCarry << 7);

            this->setCarryFlag(bit0);
            this->setZeroFlag(false);
            this->setSubtractFlag(false);
            this->setHalfCarryFlag(false);

            break;
        }

        case 0x20: {
            int8_t offset = (int8_t)this->getNextByte();

            if(!this->getZeroFlag()){
                this->pc += offset;
            }

            break;
        }

        case 0x21: {
            this->setHL(getNextd16());
            break;
        }

        case 0x22: {
            this->mmu->write(this->getHL(), this->a);
            this->setHL(this->getHL() + 1);
            break;
        }

        case 0x23: {
            this->setHL(this->getHL() + 1);
            break;
        }

        case 0x24: {
            this->inc8(this->h);
            break;
        }

        case 0x25: {
            this->dec8(this->h);
            break;
        }

        case 0x26: {
            this->h = getNextByte();
            break;
        }

        case 0x27: {
            uint8_t offSet = 0x00;
            bool shouldCarry = false;

            bool lastWasADD = !getSubtractFlag();
            bool carry = getCarryFlag();
            bool halfCarry = getHalfCarryFlag();

           if(carry || (lastWasADD && this->a > 0x99)){
               offSet = offSet | 0x60;
               shouldCarry = true;
           }

           if(halfCarry || (lastWasADD && (this->a & 0x0F) > 0x09)){
               offSet = offSet | 0x06;
           }

           if(lastWasADD){
               this->a += offSet;
           } else {
               this->a -= offSet;
           }

            this->setZeroFlag(this->a == 0x00);
            this->setHalfCarryFlag(false);
            this->setCarryFlag(shouldCarry);

            break;
        }

        case 0x28: {
            int8_t offset = (int8_t)this->getNextByte();

            if(getZeroFlag()){
                this->pc += offset;
            }

            break;
        }

        case 0x29: {
            uint16_t hl = getHL();
            this->addHL16(hl, hl);
            break;
        }

        case 0x2A: {
            this->a = this->mmu->read(this->getHL());

            this->setHL(this->getHL() + 1);

            break;
        }

        case 0x2B: {
            this->setHL(this->getHL() - 1);
            break;
        }

        case 0x2C: {
            this->inc8(this->l);
            break;
        }

        case 0x2D: {
            this->dec8(this->l);
            break;
        }

        case 0x2E: {
            this->l = this->getNextByte();
            break;
        }

        case 0x2F: {
            this->a = ~this->a;

            this->setSubtractFlag(true);
            this->setHalfCarryFlag(true);
            break;
        }

        case 0x30: {
            int8_t offset = (int8_t)this->getNextByte();

            if(!this->getCarryFlag()){
                this->pc += offset;
            }

            break;
        }

        case 0x31: {
            this->sp = this->getNextd16();
            break;
        }

        case 0x32: {
            this->mmu->write(this->getHL(), this->a);
            this->setHL(this->getHL() - 1);
            break;
        }

        case 0x33: {
            this->sp += 1;
            break;
        }

        case 0x34: {
            uint16_t address = this->getHL();

            uint8_t value = this->mmu->read(address);

            this->setHalfCarryFlag((value & 0x0F) == 0x0F);

            value++;
            this->mmu->write(address, value);

            this->setSubtractFlag(false);
            this->setZeroFlag(value == 0x00);

            break;
        }

        case 0x35: {
            uint16_t address = this->getHL();

            uint8_t value = this->mmu->read(address);

            this->setHalfCarryFlag((value & 0x0F) == 0x00);

            value--;
            this->mmu->write(address, value);

            this->setSubtractFlag(true);
            this->setZeroFlag(value == 0x00);

            break;
        }

        case 0x36: {
            this->mmu->write(this->getHL(), this->getNextByte());
            break;
        }

        case 0x37: {
            this->setCarryFlag(true);
            this->setSubtractFlag(false);
            this->setHalfCarryFlag(false);
            break;
        }

        case 0x38: {
            int8_t offset = (int8_t)this->getNextByte();

            if(this->getCarryFlag()){
                this->pc += offset;
            }

            break;
        }

        case 0x39: {
            uint16_t hl =  this->getHL();
            this->addHL16(hl,this->sp);
            break;
        }

        case 0x3A: {
            this->a = this->mmu->read(this->getHL());

            this->setHL(this->getHL() - 1);

            break;
        }

        case 0x3B: {
            this->sp--;
            break;
        }

        case 0x3C: {
            this->inc8(this->a);
            break;
        }

        case 0x3D: {
            this->dec8(this->a);
            break;
        }

        case 0x3E: {
            this->a = this->getNextByte();
            break;
        }

        case 0x3F: {
            this->setCarryFlag(!this->getCarryFlag());
            this->setSubtractFlag(false);
            this->setHalfCarryFlag(false);
            break;
        }

        case 0x40: {
            this->b = this->b;
            break;
        }

        case 0x41: {
            this->b = this->c;
            break;
        }

        case 0x42:{
            this->b = this->d;
            break;
        }

        case 0x43: {
            this->b = this->e;
            break;
        }

        case 0x44: {
            this->b = this->h;
            break;
        }

        case 0x45: {
            this->b = this->l;
            break;
        }

        case 0x46: {
            this->b = this->mmu->read(this->getHL());
            break;
        }

        case 0x47:{
            this->b = this->a;
            break;
        }

        case 0x48: {
            this->c = this->b;
            break;
        }

        case 0x49: {
            this->c = this->c;
            break;
        }

        case 0x4A: {
            this->c = this->d;
            break;
        }

        case 0x4B: {
            this->c = this->e;
            break;
        }

        case 0x4C: {
            this->c = this->h;
            break;
        }

        case 0x4D: {
            this->c = this->l;
            break;
        }

        case 0x4E: {
            this->c = this->mmu->read(this->getHL());
            break;
        }

        case 0x4F: {
            this->c = this->a;
            break;
        }

        case 0x50: {
            this->d = this->b;
            break;
        }

        case 0x51: {
            this->d = this->c;
            break;
        }

        case 0x52: {
            this->d = this->d;
            break;
        }

        case 0x53: {
            this->d = this->e;
            break;
        }

        case 0x54: {
            this->d = this->h;
            break;
        }

        case 0x55: {
            this->d = this->l;
            break;
        }

        case 0x56: {
            this->d = this->mmu->read(this->getHL());
            break;
        }

        case 0x57: {
            this->d = this->a;
            break;
        }

        case 0x58: {
            this->e = this->b;
            break;
        }

        case 0x59: {
            this->e = this->c;
            break;
        }

        case 0x5A: {
            this->e = this->d;
            break;
        }

        case 0x5B: {
            this->e = this->e;
            break;
        }

        case 0x5C: {
            this->e = this->h;
            break;
        }

        case 0x5D: {
            this->e = this->l;
            break;
        }

        case 0x5E: {
            this->e = this->mmu->read(this->getHL());
            break;
        }

        case 0x5F: {
            this->e = this->a;
            break;
        }

        case 0x60: {
            this->h = this->b;
            break;
        }

        case 0x61: {
            this->h = this->c;
            break;
        }

        case 0x62: {
            this->h = this->d;
            break;
        }

        case 0x63: {
            this->h = this->e;
            break;
        }

        case 0x64: {
            this->h = this->h;
            break;
        }

        case 0x65: {
            this->h = this->l;
            break;
        }

        case 0x66: {
            this->h = this->mmu->read(this->getHL());
            break;
        }

        case 0x67: {
            this->h = this->a;
            break;
        }

        case 0x68: {
            this->l = this->b;
            break;
        }

        case 0x69: {
            this->l = this->c;
            break;
        }

        case 0x6A: {
            this->l = this->d;
            break;
        }

        case 0x6B: {
            this->l = this->e;
            break;
        }

        case 0x6C: {
            this->l = this->h;
            break;
        }

        case 0x6D: {
            this->l = this->l;
            break;
        }

        case 0x6E: {
            this->l = this->mmu->read(this->getHL());
            break;
        }

        case 0x6F: {
            this->l = this->a;
            break;
        }

        case 0x70: {
            this->mmu->write(this->getHL(),this->b);
            break;
        }

        case 0x71: {
            this->mmu->write(this->getHL(),this->c);
            break;
        }

        case 0x72: {
            this->mmu->write(this->getHL(),this->d);
            break;
        }

        case 0x73: {
            this->mmu->write(this->getHL(),this->e);
            break;
        }

        case 0x74: {
            this->mmu->write(this->getHL(),this->h);
            break;
        }

        case 0x75: {
            this->mmu->write(this->getHL(),this->l);
            break;
        }

        case 0x76: {
            this->isHalted = true;
            break;
        }

        case 0x77:{
            this->mmu->write(this->getHL(),this->a);
            break;
        }

        case 0x78: {
            this->a = this->b;
            break;
        }

        case 0x79: {
            this->a = this->c;
            break;
        }

        case 0x7A: {
            this->a = this->d;
            break;
        }

        case 0x7B: {
            this->a = this->e;
            break;
        }

        case 0x7C: {
            this->a = this->h;
            break;
        }

        case 0x7D: {
            this->a = this->l;
            break;
        }

        case 0x7E: {
            this->a = this->mmu->read(this->getHL());
            break;
        }

        case 0x7F: {
            this->a = this->a;
            break;
        }

        case 0xAF: {
            this->a = this->a ^ this-> a;
            setZeroFlag(true);
            setSubtractFlag(false);
            setHalfCarryFlag(false);
            setCarryFlag(false);
            break;
        }

        case 0xC3: {
            this->pc = getNextd16();
            break;
        }

        default:
            std::cout << "Unrecognized Opcode: 0x"
            << std::hex << std::setfill('0') << std::setw(2)
            << static_cast<int>(opCode) << std::endl;
            exit(1);
    }
}
