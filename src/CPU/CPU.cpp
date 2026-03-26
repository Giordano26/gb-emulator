#include "CPU.hpp"
#include "MMU/MMU.hpp"
#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include <cstdlib>
#include <iomanip>

CPU::CPU(MMU* mmuPtr): mmu(mmuPtr){}

uint16_t CPU::getAF(){
    return (this->a << 8) | this->f;
}

void CPU::setAF(uint16_t value){
    this->a = (value >> 8) & 0xFF;
    this->f = value & 0xFF;
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

void CPU::setSubtractFlag(bool state){
    if(state){
        this->f = this->f | (1 << 6);
        return;
    }

    this->f = this->f & ~(1 << 6);
}

void CPU::setHalfCarryFlag(bool state){
    if(state){
        this->f = this->f | (1 << 5);
        return;
    }

    this->f = this->f & ~(1 << 5);
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
            uint16_t bc = getBC();
            this->setBC(bc + 1);
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
            uint16_t address = getNextd16();
            uint8_t lowByte = this->sp & 0xFF;
            uint8_t highByte = (this->sp >> 8) & 0xFF;

            this->mmu->write(address, lowByte);
            this->mmu->write(address + 1, highByte);
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
            uint16_t bc = this->getBC();
            this->setBC(bc - 1);
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
            uint16_t de = this->getDE();
            this->setDE(de + 1);
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
            uint16_t de = getDE();
            this->setDE(de - 1);
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

        case 0x21: {
            this->setHL(getNextd16());
            break;
        }

        case 0x32: {
            this->mmu->write(this->getHL(), this->a);
            this->setHL(this->getHL() - 1);
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
