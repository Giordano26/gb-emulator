#include "ROM.hpp"
#include <cstdio>
#include <exception>
#include <iostream>
#include <fstream>
#include <regex>

ROM::ROM(std::string filePath): file(filePath){
    this->romBank = 1;
    this->ramBank = 0;
    this->ramEnabled = false;
    this->bankingMode = false;
    this->cartType = 0;

    std::regex re(R"(^([^.]+)\.gb$)");
    this->saveFilePath = std::regex_replace(filePath, re, "$1.sav");

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

    if (this->romData.size() > 0x0147) {
        this->cartType = this->romData[0x0147];
    }


    this->ramData.resize(0x8000, 0);
    std::ifstream saveFile(this->saveFilePath, std::ios::binary);
    if(saveFile){
        std::cout << " Save file: " << this->saveFilePath  << " was found." << std::endl;

        saveFile.seekg(0, std::ios::end);
        std::streampos saveSize = saveFile.tellg();
        ramData.resize(static_cast<std::size_t>(saveSize));

        saveFile.seekg(0, std::ios::beg);
        saveFile.read(reinterpret_cast<char*>(ramData.data()), saveSize);
    }

    return true;
}

uint8_t ROM::read(uint16_t address) {
    //Bank 0
    if (address < 0x4000) {
        return this->romData[address];
    }

    //Bank 1 - 127
    if (address >= 0x4000 && address < 0x8000) {
        uint32_t newAddress = (address - 0x4000) + (this->romBank * 0x4000);
        return this->romData[newAddress % this->romData.size()];
    }

    //Swap Bank (Cartridge RAM)
    if (address >= 0xA000 && address <= 0xBFFF) {
        if (this->ramEnabled) {
            uint32_t newAddress = (address - 0xA000) + (this->ramBank * 0x2000);
            return this->ramData[newAddress];
        }
        return 0xFF;
    }

    return 0xFF;
}

void ROM::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        bool turningOff = ((value & 0x0F) != 0x0A);
        if(this->ramEnabled && turningOff){
            std::ofstream saveFile(this->saveFilePath, std::ios::binary);

            if(!saveFile.is_open()){
                std::cerr << "Error when opening the savefile for writing" << std::endl;
            }

            try{
                saveFile.write(reinterpret_cast<char*>(this->ramData.data()), this->ramData.size());
                saveFile.close();
            }catch(std::exception& e){
                std::cerr << "Could not save game data: " << e.what() << std::endl;
            }
        }

        this->ramEnabled = ((value & 0x0F) == 0x0A);
        return;
    }

    if (address >= 0x2000 && address < 0x4000) {
        if (this->cartType >= 0x01 && this->cartType <= 0x03) {
            //MBC1
            uint8_t lower5 = value & 0x1F;
            if (lower5 == 0) lower5 = 1;
            this->romBank = (this->romBank & 0xE0) | lower5;
        }
        else if (this->cartType >= 0x0F && this->cartType <= 0x13) {
            //MBC3
            uint8_t lower7 = value & 0x7F;
            if (lower7 == 0) lower7 = 1;
            this->romBank = lower7;
        }

        return;
    }

    if (address >= 0x4000 && address < 0x6000) {
        if (this->cartType >= 0x01 && this->cartType <= 0x03) {
            // MBC1
            if (this->bankingMode) {
                this->ramBank = value & 0x03;
            } else {
                this->romBank = (this->romBank & 0x1F) | ((value & 0x03) << 5);
            }
        }
        else if (this->cartType >= 0x0F && this->cartType <= 0x13) {
            // MBC3
            if (value <= 0x03) {
                this->ramBank = value;
            }
        }
        return;
    }

    if (address >= 0x6000 && address < 0x8000) {
        if (this->cartType >= 0x01 && this->cartType <= 0x03) {
            this->bankingMode = (value & 0x01) != 0;
        }
        return;
    }

    if (address >= 0xA000 && address <= 0xBFFF) {
        if (this->ramEnabled) {
            uint32_t newAddress = (address - 0xA000) + (this->ramBank * 0x2000);
            this->ramData[newAddress] = value;
        }
        return;
    }

    return;
}
