#include "ROM.hpp"
#include <cstdio>
#include <iostream>
#include <fstream>

ROM::ROM(){}


bool ROM::load(const std::string& filePath){
    std::ifstream romFile(filePath, std::ios::binary);

    if(!romFile.is_open()) return false;

    romFile.seekg(0, std::ios::end);

    std::streampos romSize = romFile.tellg();
    romData.resize(static_cast<std::size_t>(romSize));

    romFile.seekg(0, std::ios::beg);

    romFile.read(reinterpret_cast<char*>(romData.data()), romSize);

    return true;
}

uint8_t ROM::read(uint16_t address) {
    return romData[address];
}
