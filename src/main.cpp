#include <SDL.h>
#include <iostream>
#include "CPU/CPU.hpp"
#include "MMU/MMU.hpp"
#include "ROM/ROM.hpp"

const int GB_WIDTH = 160;
const int GB_HEIGHT = 144;
const int SCALE = 4;

void handleKeyboard(SDL_Event& event, MMU& motherboard) {
    bool isDown = (event.type == SDL_KEYDOWN);
    uint8_t bit = 255;
    bool isDir = false;

    switch (event.key.keysym.sym) {
        case SDLK_z:     bit = 0; isDir = false; break; // A
        case SDLK_x:     bit = 1; isDir = false; break; // B
        case SDLK_c:     bit = 2; isDir = false; break; // Select
        case SDLK_v:     bit = 3; isDir = false; break; // Start

        case SDLK_RIGHT: bit = 0; isDir = true; break;  // Right
        case SDLK_LEFT:  bit = 1; isDir = true; break;  // Left
        case SDLK_UP:    bit = 2; isDir = true; break;  // Up
        case SDLK_DOWN:  bit = 3; isDir = true; break;  // Down
    }

    if (bit != 255) {
        uint8_t& buttons = isDir ? motherboard.joypad.dirButtons : motherboard.joypad.actionButtons;

        bool wasPressed = ((buttons & (1 << bit)) == 0);

        if (isDown) {
            buttons &= ~(1 << bit);
            if (!wasPressed) {
                motherboard.requestInterrupt(4);
            }
        } else {
            buttons |= (1 << bit);
        }
    }
}

int main(int argc, char* argv[]) {
    std::string filePath = (argc > 1) ? argv[1] : "tetris.gb";
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

            if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                handleKeyboard(event, motherboard);
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
