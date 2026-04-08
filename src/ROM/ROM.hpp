#pragma once
#include <cstdint>
#include <vector>
#include <string>

class ROM {
private:
    std::vector<uint8_t> romData;
    std::string file;

public:
    ROM(std::string filePath);

    bool load();
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
};
