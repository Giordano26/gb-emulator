#include <cstdint>
#include <iostream>
#include "ROM/ROM.hpp"
#include "MMU/MMU.hpp"
#include "CPU/CPU.hpp"

int main() {
    ROM meuCartucho;

    if (!meuCartucho.load("tetris.gb")) {
        std::cerr << "Erro ao carregar a ROM!" << std::endl;
        return 1;
    }

    MMU mmu(&meuCartucho);
    CPU cpu(&mmu);

    cpu.reset();
    while(1){
        cpu.runStep();
    }

    return 0;
}
