CXX = g++
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LDFLAGS = $(shell sdl2-config --libs)

CXXFLAGS = -Wall -Wextra -std=c++17 -Isrc $(SDL_CFLAGS)
TARGET = gabs

OBJ_DIR = build

SRCS = src/main.cpp src/ROM/ROM.cpp src/MMU/MMU.cpp src/CPU/CPU.cpp src/Timer/Timer.cpp src/PPU/PPU.cpp

OBJS = $(patsubst src/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(SDL_LDFLAGS)

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
