#pragma once
#include <cstdint>
#include <vector>
#include <string>

class ROM {
private:
    std::vector<uint8_t> romData;

public:
    ROM();

    bool load(const std::string& filePath);
    uint8_t read(uint16_t address);
};
