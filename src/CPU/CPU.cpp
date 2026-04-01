#include "CPU.hpp"
#include "MMU/MMU.hpp"
#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include <cstdlib>
#include <iomanip>

CPU::CPU(MMU* mmuPtr): mmu(mmuPtr){
    this->isHalted = false;
    this->ime = false;
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

void CPU::handleInterrupts(){
    if (!this->ime) return;

    uint8_t req = this->mmu->read(0xFF0F);
    uint8_t enabled = this->mmu->read(0xFFFF);

    uint8_t pending = req & enabled & 0x1F;

    if (pending == 0) return;

    for (int i = 0; i <= 4; i++) {
        if (pending & (1 << i)) {

            this->ime = false;

            req = req & ~(1 << i);
            this->mmu->write(0xFF0F, req);

            this->push(this->pc);

            switch (i) {
                case 0: this->pc = 0x0040; break;
                case 1: this->pc = 0x0048; break;
                case 2: this->pc = 0x0050; break;
                case 3: this->pc = 0x0058; break;
                case 4: this->pc = 0x0060; break;
            }

            this->mmu->tick(20);

            return;
        }
    }
}

void CPU::runStep(){
    uint8_t ie = this->mmu->read(0xFFFF);
    uint8_t if_flag = this->mmu->read(0xFF0F);

    if ((ie & if_flag & 0x1F) != 0) {
        this->isHalted = false;
    }

    if (this->isHalted) {
        this->mmu->tick(4);
        return;
    }

    this->handleInterrupts();

    uint8_t opCode = getNextByte();
    uint8_t cycles = this->decode(opCode);

    this->mmu->tick(cycles);
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

void CPU::addHL16(uint16_t& firstPair, uint16_t secondPair){
    this->setHalfCarryFlag((((firstPair & 0x0FFF) + (secondPair & 0x0FFF)) > 0x0FFF));

    uint32_t pairSum = firstPair + secondPair;
    this->setCarryFlag(pairSum > 0xFFFF);

    this->setSubtractFlag(false);

    setHL(pairSum & 0xFFFF);
}

void CPU::add8(uint8_t& firstReg, uint8_t secondReg){
    uint16_t sum = firstReg + secondReg;

    this->setHalfCarryFlag(((firstReg & 0x0F) + (secondReg & 0x0F)) > 0x0F);
    this->setCarryFlag(sum > 0xFF);
    this->setSubtractFlag(false);
    this->setZeroFlag((sum & 0xFF) == 0x00);

    firstReg = sum & 0xFF;
}

void CPU::sub8(uint8_t& firstReg, uint8_t secondReg){
    uint16_t sub = firstReg - secondReg;

    this->setHalfCarryFlag((firstReg & 0x0F) < (secondReg & 0x0F));
    this->setCarryFlag(firstReg < secondReg);
    this->setSubtractFlag(true);
    this->setZeroFlag((sub & 0xFF) == 0x00);

    firstReg = sub & 0xFF;
}

void CPU::adc8(uint8_t& firstReg, uint8_t secondReg){
    uint8_t carry = this->getCarryFlag() ? 1 : 0;
    uint16_t sum = firstReg + secondReg + carry;

    this->setHalfCarryFlag(((firstReg & 0x0F) + (secondReg & 0x0F) + carry) > 0x0F);
    this->setCarryFlag(sum > 0xFF);
    this->setSubtractFlag(false);
    this->setZeroFlag((sum & 0xFF) == 0x00);

    firstReg = sum & 0xFF;
}

void CPU::sbc8(uint8_t& firstReg, uint8_t secondReg){
    uint8_t carry = this->getCarryFlag() ? 1 : 0;
    uint16_t sub = firstReg - secondReg - carry;

    this->setHalfCarryFlag((firstReg & 0x0F) < ((secondReg & 0x0F) + carry));
    this->setCarryFlag((uint16_t)firstReg < ((uint16_t)secondReg + carry));
    this->setSubtractFlag(true);
    this->setZeroFlag((sub & 0xFF) == 0x00);

    firstReg = sub & 0xFF;
}

void CPU::and8(uint8_t& firstReg, uint8_t secondReg){
    firstReg = firstReg & secondReg;

    this->setZeroFlag(firstReg == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(true);
    this->setCarryFlag(false);

}

void CPU::xor8(uint8_t& firstReg, uint8_t secondReg){
    firstReg = firstReg ^ secondReg;

    this->setZeroFlag(firstReg == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
    this->setCarryFlag(false);

}

void CPU::or8(uint8_t& firstReg, uint8_t secondReg){
    firstReg = firstReg | secondReg;

    this->setZeroFlag(firstReg == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
    this->setCarryFlag(false);

}

void CPU::cp8(uint8_t firstReg, uint8_t secondReg){
    uint16_t sub = firstReg - secondReg;

    this->setHalfCarryFlag((firstReg & 0x0F) < (secondReg & 0x0F));
    this->setCarryFlag(firstReg < secondReg);
    this->setSubtractFlag(true);
    this->setZeroFlag((sub & 0xFF) == 0x00);
}


uint16_t CPU::pop(){
    uint8_t low = this->mmu->read(this->sp);
    this->sp++;

    uint8_t high = this->mmu->read(this->sp);
    this->sp++;

    return (high << 8) | low;
}

void CPU::push(uint16_t value){
    this->sp--;
    this->mmu->write(this->sp, (value >> 8) & 0xFF);

    this->sp--;
    this->mmu->write(this->sp, value & 0xFF);
}

void CPU::rst(uint16_t address){
    this->push(this->pc);

    this->pc = address;
}

void CPU::rlc(uint8_t& reg){
    uint8_t bit7 = (reg >> 7) & 0x01;

    reg = (reg << 1) | bit7;

    this->setCarryFlag(bit7);
    this->setZeroFlag(reg == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
}

void CPU::rrc(uint8_t& reg){
    uint8_t bit0 = reg & 0x01;

    this->setCarryFlag(bit0);

    reg = (reg >> 1) | (bit0 << 7);

    this->setZeroFlag(reg == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
}

void CPU::rl(uint8_t& reg){
    uint8_t carry = this->getCarryFlag() ? 1 : 0;
    uint8_t bit7 = (reg >> 7) & 0x01;

    this->setCarryFlag(bit7);

    reg = (reg << 1) | carry;

    this->setZeroFlag(reg  == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
}

void CPU::rr(uint8_t& reg){
    uint8_t carry = this->getCarryFlag() ? 1 : 0;
    uint8_t bit0 = reg & 0x01;

    this->setCarryFlag(bit0);

    reg = (reg >> 1) | (carry << 7);

    this->setZeroFlag(reg == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
}

void CPU::sla(uint8_t& reg){
    uint8_t bit7 = (reg >> 7) & 0x01;
    this->setCarryFlag(bit7);

    reg = reg << 1;

    this->setZeroFlag(reg  == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
}

void CPU::sra(uint8_t& reg){
    uint8_t bit0 = reg & 0x01;
    uint8_t bit7 = reg & 0x80;

    this->setCarryFlag(bit0);

    reg = (reg >> 1) | bit7;

    this->setZeroFlag(reg  == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
}

void CPU::swap(uint8_t& reg) {
    reg = ((reg & 0x0F) << 4) | ((reg & 0xF0) >> 4);

    this->setZeroFlag(reg == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
    this->setCarryFlag(false);
}

void CPU::srl(uint8_t& reg){
    uint8_t bit0 = reg & 0x01;

    this->setCarryFlag(bit0);

    reg = reg >> 1;

    this->setZeroFlag(reg  == 0x00);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(false);
}

void CPU::bit(uint8_t bitPosition, uint8_t reg){

    uint8_t bitValue = reg & (1 << bitPosition);

    this->setZeroFlag(bitValue == 0);
    this->setSubtractFlag(false);
    this->setHalfCarryFlag(true);
}

void CPU::res(uint8_t bitPosition, uint8_t& reg){
    reg = reg & ~(1 << bitPosition);
}

void CPU::set(uint8_t bitPosition, uint8_t& reg){
    reg = reg | (1 << bitPosition);
}

uint8_t CPU::decode(uint8_t opCode){
    uint8_t cycles = this->opCycles[opCode];

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

            this->setZeroFlag(false);
            this->setSubtractFlag(false);
            this->setHalfCarryFlag(false);
            break;
        }

        case 0x08: {
            this->mmu->write16(getNextd16(),this->sp);
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
                cycles += 4;
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
                cycles += 4;
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
                cycles += 4;
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
                cycles += 4;
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

        case 0x80:{
            this->add8(this->a, this->b);
            break;
        }

        case 0x81: {
            this->add8(this->a, this->c);
            break;
        }

        case 0x82: {
            this->add8(this->a, this->d);
            break;
        }

        case 0x83: {
            this->add8(this->a, this->e);
            break;
        }

        case 0x84: {
            this->add8(this->a, this->h);
            break;
        }

        case 0x85: {
            this->add8(this->a, this->l);
            break;
        }

        case 0x86: {
            uint8_t hlValue = this->mmu->read(this->getHL());
            this->add8(this->a, hlValue);
            break;
        }

        case 0x87: {
            this->add8(this->a, this->a);
            break;
        }

        case 0x88: {
            this->adc8(this->a, this->b);
            break;
        }

        case 0x89: {
            this->adc8(this->a, this->c);
            break;
        }

        case 0x8A: {
            this->adc8(this->a, this->d);
            break;
        }

        case 0x8B: {
            this->adc8(this->a, this->e);
            break;
        }

        case 0x8C: {
            this->adc8(this->a, this->h);
            break;
        }

        case 0x8D: {
            this->adc8(this->a, this->l);
            break;
        }

        case 0x8E: {
            uint8_t hlValue = this->mmu->read(this->getHL());
            this->adc8(this->a, hlValue);
            break;
        }

        case 0x8F: {
            this->adc8(this->a, this->a);
            break;
        }

        case 0x90: {
            this->sub8(this->a, this->b);
            break;
        }

        case 0x91: {
            this->sub8(this->a, this->c);
            break;
        }

        case 0x92: {
            this->sub8(this->a, this->d);
            break;
        }

        case 0x93: {
            this->sub8(this->a, this->e);
            break;
        }

        case 0x94: {
            this->sub8(this->a, this->h);
            break;
        }

        case 0x95: {
            this->sub8(this->a, this->l);
            break;
        }

        case 0x96: {
             uint8_t hlValue = this->mmu->read(this->getHL());
            this->sub8(this->a, hlValue);
            break;
        }

        case 0x97: {
            this->sub8(this->a, this->a);
            break;
        }

        case 0x98: {
            this->sbc8(this->a, this->b);
            break;
        }

        case 0x99: {
            this->sbc8(this->a, this->c);
            break;
        }

        case 0x9A: {
            this->sbc8(this->a, this->d);
            break;
        }

        case 0x9B: {
            this->sbc8(this->a, this->e);
            break;
        }

        case 0x9C: {
            this->sbc8(this->a, this->h);
            break;
        }

        case 0x9D: {
            this->sbc8(this->a, this->l);
            break;
        }

        case 0x9E: {
            uint8_t hlValue = this->mmu->read(this->getHL());
            this->sbc8(this->a, hlValue);
            break;
        }

        case 0x9F: {
            this->sbc8(this->a, this->a);
            break;
        }

        case 0xA0: {
            this->and8(this->a, this->b);
            break;
        }

        case 0xA1: {
            this->and8(this->a, this->c);
            break;
        }

        case 0xA2: {
            this->and8(this->a, this->d);
            break;
        }

        case 0xA3: {
            this->and8(this->a, this->e);
            break;
        }

        case 0xA4: {
            this->and8(this->a, this->h);
            break;
        }

        case 0xA5: {
            this->and8(this->a, this->l);
            break;
        }

        case 0xA6: {
            this->and8(this->a, this->mmu->read(this->getHL()));
            break;
        }

        case 0xA7: {
            this->and8(this->a, this->a);
            break;
        }

        case 0xA8: {
            this->xor8(this->a, this->b);
            break;
        }

        case 0xA9: {
            this->xor8(this->a, this->c);
            break;
        }

        case 0xAA: {
            this->xor8(this->a, this->d);
            break;
        }

        case 0xAB: {
            this->xor8(this->a, this->e);
            break;
        }

        case 0xAC: {
            this->xor8(this->a, this->h);
            break;
        }

        case 0xAD: {
            this->xor8(this->a, this->l);
            break;
        }

        case 0xAE: {
            this->xor8(this->a, this->mmu->read(this->getHL()));
            break;
        }

        case 0xAF: {
            this->xor8(this->a,this->a);
            break;
        }

        case 0xB0: {
            this->or8(this->a, this->b);
            break;
        }

        case 0xB1: {
            this->or8(this->a, this->c);
            break;
        }

        case 0xB2: {
            this->or8(this->a, this->d);
            break;
        }

        case 0xB3: {
            this->or8(this->a, this->e);
            break;
        }

        case 0xB4: {
            this->or8(this->a, this->h);
            break;
        }

        case 0xB5: {
            this->or8(this->a, this->l);
            break;
        }

        case 0xB6: {
            this->or8(this->a, this->mmu->read(this->getHL()));
            break;
        }

        case 0xB7: {
            this->or8(this->a, this->a);
            break;
        }

        case 0xB8: {
            this->cp8(this->a, this->b);
            break;
        }

        case 0xB9: {
            this->cp8(this->a, this->c);
            break;
        }

        case 0xBA: {
            this->cp8(this->a, this->d);
            break;
        }

        case 0xBB: {
            this->cp8(this->a, this->e);
            break;
        }

        case 0xBC: {
            this->cp8(this->a, this->h);
            break;
        }

        case 0xBD: {
            this->cp8(this->a, this->l);
            break;
        }

        case 0xBE: {
            this->cp8(this->a, this->mmu->read(this->getHL()));
            break;
        }

        case 0xBF: {
            this->cp8(this->a, this->a);
            break;
        }

        case 0xC0: {
            if(!this->getZeroFlag()){
                this->pc = this->pop();
                cycles += 12;
            }

            break;
        }

        case 0xC1: {
            this->setBC(this->pop());
            break;
        }

        case 0xC2: {
            uint16_t offset = this->getNextd16();

            if(!this->getZeroFlag()){
                this->pc = offset;
                cycles += 4;
            }

            break;
        }

        case 0xC3: {
            this->pc = getNextd16();
            break;
        }

        case 0xC4: {
            uint16_t destination = this->getNextd16();

            if(!this->getZeroFlag()){
                this->push(this->pc);
                this->pc = destination;
                cycles += 12;
            }

            break;
        }

        case 0xC5: {
            this->push(this->getBC());
            break;
        }

        case 0xC6: {
            this->add8(this->a, this->getNextByte());
            break;
        }

        case 0xC7: {
            this->rst(0x0000);
            break;
        }

        case 0xC8: {
            if(this->getZeroFlag()){
                this->pc = this->pop();
                cycles += 12;

            }

            break;
        }

        case 0xC9: {
            this->pc = this->pop();
            break;
        }

        case 0xCA: {
            uint16_t offset = this->getNextd16();

            if(this->getZeroFlag()){
                this->pc = offset;
                cycles += 4;
            }

            break;
        }

        case 0xCB: {
            uint8_t cbOpcode = this->getNextByte();

            cycles = 4 + this->decodePrefix(cbOpcode);
            break;
        }

        case 0xCC: {
            uint16_t destination = this->getNextd16();

            if(this->getZeroFlag()){
                this->push(this->pc);
                this->pc = destination;
                cycles += 12;
            }

            break;
        }

        case 0xCD: {
            uint16_t destination = this->getNextd16();
            this->push(this->pc);
            this->pc = destination;

            break;
        }

        case 0xCE: {
            this->adc8(this->a, this->getNextByte());
            break;
        }

        case 0xCF: {
            this->rst(0x0008);
            break;
        }

        case 0xD0: {
            if(!this->getCarryFlag()){
                this->pc = this->pop();
                cycles += 12;
            }

            break;
        }

        case 0xD1: {
            this->setDE(this->pop());
            break;
        }

        case 0xD2: {
            uint16_t offset = this->getNextd16();

            if(!this->getCarryFlag()){
                this->pc = offset;
                cycles += 4;
            }

            break;
        }

        case 0xD3: {
            break;
        }

        case 0xD4: {
            uint16_t destination = this->getNextd16();

            if(!this->getCarryFlag()){
                this->push(this->pc);
                this->pc = destination;
                cycles += 12;
            }

            break;
        }

        case 0xD5: {
            this->push(this->getDE());
            break;
        }

        case 0xD6: {
            this->sub8(this->a, this->getNextByte());
            break;
        }

        case 0xD7: {
            this->rst(0x0010);
            break;
        }

        case 0xD8: {
            if(this->getCarryFlag()){
                this->pc = this->pop();
                cycles += 12;
            }

            break;
        }

        case 0xD9: {
            this->pc = this->pop();

            this->ime = true;

            break;
        }

        case 0xDA: {
            uint16_t offset = this->getNextd16();

            if(this->getCarryFlag()){
                this->pc = offset;
                cycles += 4;
            }

            break;
        }

        case 0xDB: {break;}

        case 0xDC: {
            uint16_t destination = this->getNextd16();

            if(this->getCarryFlag()){
                this->push(this->pc);
                this->pc = destination;
                cycles += 12;
            }

            break;
        }

        case 0xDD: {break;}

        case 0xDE: {
            this->sbc8(this->a, getNextByte());
            break;
        }

        case 0xDF: {
            this->rst(0x0018);
            break;
        }

        case 0xE0: {
            uint8_t offset = this->getNextByte();

            uint16_t address = 0xFF00 + offset;

            this->mmu->write(address, this->a);
            break;
        }

        case 0xE1: {
            this->setHL(this->pop());
            break;
        }

        case 0xE2: {
            uint8_t offset = this->c;

            uint16_t address = 0xFF00 + offset;

            this->mmu->write(address, this->a);
            break;
        }

        case 0xE3: {break;}
        case 0xE4: {break;}

        case 0xE5: {
            this->push(this->getHL());
            break;
        }

        case 0xE6: {
            this->and8(this->a, this->getNextByte());
            break;
        }

        case 0xE7: {
            this->rst(0x0020);
            break;
        }

        case 0xE8: {
            int8_t offset = (int8_t)this->getNextByte();
            uint8_t unsignedOffset = (uint8_t)offset;

            uint16_t oldSP = this->sp;

            this->setHalfCarryFlag(((oldSP & 0x0F) + (unsignedOffset & 0x0F)) > 0x0F);
            this->setCarryFlag(((oldSP & 0xFF) + unsignedOffset) > 0xFF);
            this->setZeroFlag(false);
            this->setSubtractFlag(false);

            this->sp = this->sp + offset;
            break;
        }

        case 0xE9: {
            this->pc = this->getHL();
            break;
        }

        case 0xEA: {
            uint16_t address = this->getNextd16();
            this->mmu->write(address, this->a);
            break;
        }

        case 0xEB: {break;}
        case 0xEC: {break;}
        case 0xED: {break;}

        case 0xEE:{
            this->xor8(this->a, this->getNextByte());
            break;
        }

        case 0xEF: {
            this->rst(0x0028);
            break;
        }

        case 0xF0:{
            uint8_t offset = this->getNextByte();

            uint16_t address = 0xFF00 + offset;

            this->a = this->mmu->read(address);
            break;
        }

        case 0xF1: {
            this->setAF(this->pop());
            break;
        }

        case 0xF2: {
            uint8_t offset = this->c;

            uint16_t address = 0xFF00 + offset;

            this->a = this->mmu->read(address);
            break;
        }

        case 0xF3: {
            this->ime = false;
            break;
        }

        case 0xF4: {break;}

        case 0xF5: {
            this->push(this->getAF());
            break;
        }

        case 0xF6: {
            this->or8(this->a, this->getNextByte());
            break;
        }

        case 0xF7: {
            this->rst(0x0030);
            break;
        }

        case 0xF8: {
            int8_t offset = (int8_t)this->getNextByte();
            uint16_t oldSP = this->sp;

            uint8_t unsignedOffset = (uint8_t)offset;

            this->setHalfCarryFlag(((oldSP & 0x0F) + (unsignedOffset & 0x0F)) > 0x0F);
            this->setCarryFlag(((oldSP & 0xFF) + unsignedOffset) > 0xFF);

            this->setZeroFlag(false);
            this->setSubtractFlag(false);

            this->setHL(oldSP + offset);

            break;
        }

        case 0xF9: {
            this->sp = this->getHL();
            break;
        }

        case 0xFA: {
            uint16_t address = this->getNextd16();
            this->a = this->mmu->read(address);
            break;
        }

        case 0xFB: {
            this->ime = true;
            break;
        }

        case 0xFC: {break;}
        case 0xFD: {break;}

        case 0xFE: {
            this->cp8(this->a, this->getNextByte());
            break;
        }

        case 0xFF: {
            this->rst(0x0038);
            break;
        }

        default:
            std::cout << "Unrecognized Opcode: 0x"
            << std::hex << std::setfill('0') << std::setw(2)
            << static_cast<int>(opCode) << std::endl;
            exit(1);
    }

    return cycles;
}

uint8_t CPU::decodePrefix(uint8_t cbOpcode){
    uint8_t cycles = this->cbCycles[cbOpcode];

    switch (cbOpcode) {
        case 0x00: { this->rlc(this->b); break; }
        case 0x01: {this->rlc(this->c); break; }
        case 0x02: {this->rlc(this->d); break; }
        case 0x03: {this->rlc(this->e); break; }
        case 0x04: {this->rlc(this->h); break; }
        case 0x05: {this->rlc(this->l); break; }
        case 0x06: {
            uint8_t value = this->mmu->read(this->getHL());
            this->rlc(value);
            this->mmu->write(this->getHL(), value);
            break;
        }
        case 0x07: { this->rlc(this->a); break; }

        case 0x08: { this->rrc(this->b); break; }
        case 0x09: { this->rrc(this->c); break; }
        case 0x0A: { this->rrc(this->d); break; }
        case 0x0B: { this->rrc(this->e); break; }
        case 0x0C: { this->rrc(this->h); break; }
        case 0x0D: { this->rrc(this->l); break; }
        case 0x0E:{
            uint8_t value = this->mmu->read(this->getHL());
            this->rrc(value);
            this->mmu->write(this->getHL(), value);
            break;
        }
        case 0x0F: { this->rrc(this->a); break; }

        case 0x10: { this->rl(this->b); break; }
        case 0x11: { this->rl(this->c); break; }
        case 0x12: { this->rl(this->d); break; }
        case 0x13: { this->rl(this->e); break; }
        case 0x14: { this->rl(this->h); break; }
        case 0x15: { this->rl(this->l); break; }
        case 0x16: {
            uint8_t value = this->mmu->read(this->getHL());
            this->rl(value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0x17: { this->rl(this->a); break; }

        case 0x18: { this->rr(this->b); break; }
        case 0x19: { this->rr(this->c); break; }
        case 0x1A: { this->rr(this->d); break; }
        case 0x1B: { this->rr(this->e); break; }
        case 0x1C: { this->rr(this->h); break; }
        case 0x1D: { this->rr(this->l); break; }
        case 0x1E:{
            uint8_t value = this->mmu->read(this->getHL());
            this->rr(value);
            this->mmu->write(this->getHL(), value);
            break;
        }
        case 0x1F: { this->rr(this->a); break; }

        case 0x20: { this->sla(this->b); break; }
        case 0x21: { this->sla(this->c); break; }
        case 0x22: { this->sla(this->d); break; }
        case 0x23: { this->sla(this->e); break; }
        case 0x24: { this->sla(this->h); break; }
        case 0x25: { this->sla(this->l); break; }
        case 0x26: {
            uint8_t value = this->mmu->read(this->getHL());
            this->sla(value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0x27: { this->sla(this->a); break; }

        case 0x28: { this->sra(this->b); break; }
        case 0x29: { this->sra(this->c); break; }
        case 0x2A: { this->sra(this->d); break; }
        case 0x2B: { this->sra(this->e); break; }
        case 0x2C: { this->sra(this->h); break; }
        case 0x2D: { this->sra(this->l); break; }
        case 0x2E:{
            uint8_t value = this->mmu->read(this->getHL());
            this->sra(value);
            this->mmu->write(this->getHL(), value);
            break;
        }
        case 0x2F: { this->sra(this->a); break; }

        case 0x30: { this->swap(this->b); break; }
        case 0x31: { this->swap(this->c); break; }
        case 0x32: { this->swap(this->d); break; }
        case 0x33: { this->swap(this->e); break; }
        case 0x34: { this->swap(this->h); break; }
        case 0x35: { this->swap(this->l); break; }
        case 0x36: {
            uint8_t value = this->mmu->read(this->getHL());
            this->swap(value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0x37: { this->swap(this->a); break; }

        case 0x38: { this->srl(this->b); break; }
        case 0x39: { this->srl(this->c); break; }
        case 0x3A: { this->srl(this->d); break; }
        case 0x3B: { this->srl(this->e); break; }
        case 0x3C: { this->srl(this->h); break; }
        case 0x3D: { this->srl(this->l); break; }
        case 0x3E:{
            uint8_t value = this->mmu->read(this->getHL());
            this->srl(value);
            this->mmu->write(this->getHL(), value);
            break;
        }
        case 0x3F: { this->srl(this->a); break; }

        case 0x40: { this->bit(0, this->b); break; }
        case 0x41: { this->bit(0, this->c); break; }
        case 0x42: { this->bit(0, this->d); break; }
        case 0x43: { this->bit(0, this->e); break; }
        case 0x44: { this->bit(0, this->h); break; }
        case 0x45: { this->bit(0, this->l); break; }
        case 0x46: {
            uint8_t value = this->mmu->read(this->getHL());
            this->bit(0, value);
            break;
        }
        case 0x47: { this->bit(0, this->a); break; }

        case 0x48: { this->bit(1, this->b); break; }
        case 0x49: { this->bit(1, this->c); break; }
        case 0x4A: { this->bit(1, this->d); break; }
        case 0x4B: { this->bit(1, this->e); break; }
        case 0x4C: { this->bit(1, this->h); break; }
        case 0x4D: { this->bit(1, this->l); break; }
        case 0x4E: {
            uint8_t value = this->mmu->read(this->getHL());
            this->bit(1, value);
            break;
        }
        case 0x4F: { this->bit(1, this->a); break; }

        case 0x50: { this->bit(2, this->b); break; }
        case 0x51: { this->bit(2, this->c); break; }
        case 0x52: { this->bit(2, this->d); break; }
        case 0x53: { this->bit(2, this->e); break; }
        case 0x54: { this->bit(2, this->h); break; }
        case 0x55: { this->bit(2, this->l); break; }
        case 0x56: {
            uint8_t value = this->mmu->read(this->getHL());
            this->bit(2, value);
            break;
        }
        case 0x57: { this->bit(2, this->a); break; }

        case 0x58: { this->bit(3, this->b); break; }
        case 0x59: { this->bit(3, this->c); break; }
        case 0x5A: { this->bit(3, this->d); break; }
        case 0x5B: { this->bit(3, this->e); break; }
        case 0x5C: { this->bit(3, this->h); break; }
        case 0x5D: { this->bit(3, this->l); break; }
        case 0x5E: {
            uint8_t value = this->mmu->read(this->getHL());
            this->bit(3, value);
            break;
        }
        case 0x5F: { this->bit(3, this->a); break; }

        case 0x60: { this->bit(4, this->b); break; }
        case 0x61: { this->bit(4, this->c); break; }
        case 0x62: { this->bit(4, this->d); break; }
        case 0x63: { this->bit(4, this->e); break; }
        case 0x64: { this->bit(4, this->h); break; }
        case 0x65: { this->bit(4, this->l); break; }
        case 0x66: {
            uint8_t value = this->mmu->read(this->getHL());
            this->bit(4, value);
            break;
        }
        case 0x67: { this->bit(4, this->a); break; }

        case 0x68: { this->bit(5, this->b); break; }
        case 0x69: { this->bit(5, this->c); break; }
        case 0x6A: { this->bit(5, this->d); break; }
        case 0x6B: { this->bit(5, this->e); break; }
        case 0x6C: { this->bit(5, this->h); break; }
        case 0x6D: { this->bit(5, this->l); break; }
        case 0x6E: {
            uint8_t value = this->mmu->read(this->getHL());
            this->bit(5, value);
            break;
        }
        case 0x6F: { this->bit(5, this->a); break; }

        case 0x70: { this->bit(6, this->b); break; }
        case 0x71: { this->bit(6, this->c); break; }
        case 0x72: { this->bit(6, this->d); break; }
        case 0x73: { this->bit(6, this->e); break; }
        case 0x74: { this->bit(6, this->h); break; }
        case 0x75: { this->bit(6, this->l); break; }
        case 0x76: {
            uint8_t value = this->mmu->read(this->getHL());
            this->bit(6, value);
            break;
        }
        case 0x77: { this->bit(6, this->a); break; }

        case 0x78: { this->bit(7, this->b); break; }
        case 0x79: { this->bit(7, this->c); break; }
        case 0x7A: { this->bit(7, this->d); break; }
        case 0x7B: { this->bit(7, this->e); break; }
        case 0x7C: { this->bit(7, this->h); break; }
        case 0x7D: { this->bit(7, this->l); break; }
        case 0x7E: {
            uint8_t value = this->mmu->read(this->getHL());
            this->bit(7, value);
            break;
        }
        case 0x7F: { this->bit(7, this->a); break; }

        case 0x80: { this->res(0, this->b); break; }
        case 0x81: { this->res(0, this->c); break; }
        case 0x82: { this->res(0, this->d); break; }
        case 0x83: { this->res(0, this->e); break; }
        case 0x84: { this->res(0, this->h); break; }
        case 0x85: { this->res(0, this->l); break; }
        case 0x86: {
            uint8_t value = this->mmu->read(this->getHL());
            this->res(0, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0x87: { this->res(0, this->a); break; }

        case 0x88: { this->res(1, this->b); break; }
        case 0x89: { this->res(1, this->c); break; }
        case 0x8A: { this->res(1, this->d); break; }
        case 0x8B: { this->res(1, this->e); break; }
        case 0x8C: { this->res(1, this->h); break; }
        case 0x8D: { this->res(1, this->l); break; }
        case 0x8E: {
            uint8_t value = this->mmu->read(this->getHL());
            this->res(1, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0x8F: { this->res(1, this->a); break; }

        case 0x90: { this->res(2, this->b); break; }
        case 0x91: { this->res(2, this->c); break; }
        case 0x92: { this->res(2, this->d); break; }
        case 0x93: { this->res(2, this->e); break; }
        case 0x94: { this->res(2, this->h); break; }
        case 0x95: { this->res(2, this->l); break; }
        case 0x96: {
            uint8_t value = this->mmu->read(this->getHL());
            this->res(2, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0x97: { this->res(2, this->a); break; }

        case 0x98: { this->res(3, this->b); break; }
        case 0x99: { this->res(3, this->c); break; }
        case 0x9A: { this->res(3, this->d); break; }
        case 0x9B: { this->res(3, this->e); break; }
        case 0x9C: { this->res(3, this->h); break; }
        case 0x9D: { this->res(3, this->l); break; }
        case 0x9E: {
            uint8_t value = this->mmu->read(this->getHL());
            this->res(3, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0x9F: { this->res(3, this->a); break; }

        case 0xA0: { this->res(4, this->b); break; }
        case 0xA1: { this->res(4, this->c); break; }
        case 0xA2: { this->res(4, this->d); break; }
        case 0xA3: { this->res(4, this->e); break; }
        case 0xA4: { this->res(4, this->h); break; }
        case 0xA5: { this->res(4, this->l); break; }
        case 0xA6: {
            uint8_t value = this->mmu->read(this->getHL());
            this->res(4, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xA7: { this->res(4, this->a); break; }

        case 0xA8: { this->res(5, this->b); break; }
        case 0xA9: { this->res(5, this->c); break; }
        case 0xAA: { this->res(5, this->d); break; }
        case 0xAB: { this->res(5, this->e); break; }
        case 0xAC: { this->res(5, this->h); break; }
        case 0xAD: { this->res(5, this->l); break; }
        case 0xAE: {
            uint8_t value = this->mmu->read(this->getHL());
            this->res(5, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xAF: { this->res(5, this->a); break; }

        case 0xB0: { this->res(6, this->b); break; }
        case 0xB1: { this->res(6, this->c); break; }
        case 0xB2: { this->res(6, this->d); break; }
        case 0xB3: { this->res(6, this->e); break; }
        case 0xB4: { this->res(6, this->h); break; }
        case 0xB5: { this->res(6, this->l); break; }
        case 0xB6: {
            uint8_t value = this->mmu->read(this->getHL());
            this->res(6, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xB7: { this->res(6, this->a); break; }

        case 0xB8: { this->res(7, this->b); break; }
        case 0xB9: { this->res(7, this->c); break; }
        case 0xBA: { this->res(7, this->d); break; }
        case 0xBB: { this->res(7, this->e); break; }
        case 0xBC: { this->res(7, this->h); break; }
        case 0xBD: { this->res(7, this->l); break; }
        case 0xBE: {
            uint8_t value = this->mmu->read(this->getHL());
            this->res(7, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xBF: { this->res(7, this->a); break; }

        case 0xC0: { this->set(0, this->b); break; }
        case 0xC1: { this->set(0, this->c); break; }
        case 0xC2: { this->set(0, this->d); break; }
        case 0xC3: { this->set(0, this->e); break; }
        case 0xC4: { this->set(0, this->h); break; }
        case 0xC5: { this->set(0, this->l); break; }
        case 0xC6: {
            uint8_t value = this->mmu->read(this->getHL());
            this->set(0, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xC7: { this->set(0, this->a); break; }

        case 0xC8: { this->set(1, this->b); break; }
        case 0xC9: { this->set(1, this->c); break; }
        case 0xCA: { this->set(1, this->d); break; }
        case 0xCB: { this->set(1, this->e); break; }
        case 0xCC: { this->set(1, this->h); break; }
        case 0xCD: { this->set(1, this->l); break; }
        case 0xCE: {
            uint8_t value = this->mmu->read(this->getHL());
            this->set(1, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xCF: { this->set(1, this->a); break; }

        case 0xD0: { this->set(2, this->b); break; }
        case 0xD1: { this->set(2, this->c); break; }
        case 0xD2: { this->set(2, this->d); break; }
        case 0xD3: { this->set(2, this->e); break; }
        case 0xD4: { this->set(2, this->h); break; }
        case 0xD5: { this->set(2, this->l); break; }
        case 0xD6: {
            uint8_t value = this->mmu->read(this->getHL());
            this->set(2, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xD7: { this->set(2, this->a); break; }

        case 0xD8: { this->set(3, this->b); break; }
        case 0xD9: { this->set(3, this->c); break; }
        case 0xDA: { this->set(3, this->d); break; }
        case 0xDB: { this->set(3, this->e); break; }
        case 0xDC: { this->set(3, this->h); break; }
        case 0xDD: { this->set(3, this->l); break; }
        case 0xDE: {
            uint8_t value = this->mmu->read(this->getHL());
            this->set(3, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xDF: { this->set(3, this->a); break; }

        case 0xE0: { this->set(4, this->b); break; }
        case 0xE1: { this->set(4, this->c); break; }
        case 0xE2: { this->set(4, this->d); break; }
        case 0xE3: { this->set(4, this->e); break; }
        case 0xE4: { this->set(4, this->h); break; }
        case 0xE5: { this->set(4, this->l); break; }
        case 0xE6: {
            uint8_t value = this->mmu->read(this->getHL());
            this->set(4, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xE7: { this->set(4, this->a); break; }

        case 0xE8: { this->set(5, this->b); break; }
        case 0xE9: { this->set(5, this->c); break; }
        case 0xEA: { this->set(5, this->d); break; }
        case 0xEB: { this->set(5, this->e); break; }
        case 0xEC: { this->set(5, this->h); break; }
        case 0xED: { this->set(5, this->l); break; }
        case 0xEE: {
            uint8_t value = this->mmu->read(this->getHL());
            this->set(5, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xEF: { this->set(5, this->a); break; }

        case 0xF0: { this->set(6, this->b); break; }
        case 0xF1: { this->set(6, this->c); break; }
        case 0xF2: { this->set(6, this->d); break; }
        case 0xF3: { this->set(6, this->e); break; }
        case 0xF4: { this->set(6, this->h); break; }
        case 0xF5: { this->set(6, this->l); break; }
        case 0xF6: {
            uint8_t value = this->mmu->read(this->getHL());
            this->set(6, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xF7: { this->set(6, this->a); break; }

        case 0xF8: { this->set(7, this->b); break; }
        case 0xF9: { this->set(7, this->c); break; }
        case 0xFA: { this->set(7, this->d); break; }
        case 0xFB: { this->set(7, this->e); break; }
        case 0xFC: { this->set(7, this->h); break; }
        case 0xFD: { this->set(7, this->l); break; }
        case 0xFE: {
            uint8_t value = this->mmu->read(this->getHL());
            this->set(7, value);
            this->mmu->write(this->getHL(),value);
            break;
        }
        case 0xFF: { this->set(7, this->a); break; }


        default:
            std::cout << "Unrecognized Opcode: CB 0x"
            << std::hex << std::setfill('0') << std::setw(4)
            << static_cast<int>(cbOpcode) << std::endl;
            exit(1);
    }

    return cycles;
}
