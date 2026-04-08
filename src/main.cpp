#include <SDL.h>
#include <iostream>
#include "CPU/CPU.hpp"
#include "MMU/MMU.hpp"
#include "ROM/ROM.hpp"

const int GB_WIDTH = 160;
const int GB_HEIGHT = 144;
const int SCALE = 4;

int main(int argc, char* argv[]) {
    std::string filePath = "tetris.gb";
    ROM rom(filePath);
    MMU motherboard(&rom);
    CPU cpu(&motherboard);

    cpu.reset();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Error on SDL init" << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "G.A.B.S",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        GB_WIDTH * SCALE, GB_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        GB_WIDTH, GB_HEIGHT
    );

    bool isRunning = true;
    SDL_Event event;

    while (isRunning) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            }
        }

        int cyclesThisFrame = 0;
        const int MAX_CYCLES_PER_FRAME = 70224;

        while (cyclesThisFrame < MAX_CYCLES_PER_FRAME) {
            uint8_t cycles = cpu.runStep();
            cyclesThisFrame += cycles;
        }

        SDL_UpdateTexture(
            texture,
            nullptr,
            motherboard.ppu.frameBuffer.data(),
            GB_WIDTH * sizeof(uint32_t)
        );

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
