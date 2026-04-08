#include "ROM.hpp"
#include <cstdio>
#include <iostream>
#include <fstream>

ROM::ROM(std::string filePath): file(filePath){
    if (!this->load()) {
        std::cerr << "Could not open ROM: " << filePath << std::endl;
    } else {
        std::cout << "ROM loaded successfully wit size: " << romData.size() << " bytes." << std::endl;
    }
}


bool ROM::load(){
    std::ifstream romFile(this->file, std::ios::binary);

    if(!romFile.is_open()) return false;

    romFile.seekg(0, std::ios::end);

    std::streampos romSize = romFile.tellg();
    romData.resize(static_cast<std::size_t>(romSize));

    romFile.seekg(0, std::ios::beg);

    romFile.read(reinterpret_cast<char*>(romData.data()), romSize);

    return true;
}

uint8_t ROM::read(uint16_t address) {
    if (address < this->romData.size()) {
        return this->romData[address];
    }

    return 0xFF;
}

void ROM::write(uint16_t address, uint8_t value) {
    //Future MBC/RAM impl
    return;
}
