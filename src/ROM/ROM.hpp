#pragma once
#include <cstdint>
#include <vector>
#include <string>

class ROM {
private:
    std::vector<uint8_t> romData;
    std::vector<uint8_t> ramData;
    std::string file;
    std::string saveFilePath;

    uint8_t cartType;
    uint32_t romBank;
    uint32_t ramBank;
    bool ramEnabled;
    bool bankingMode;

public:
    ROM(std::string filePath);

    bool load();
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
};
