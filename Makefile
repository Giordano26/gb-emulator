CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Isrc
TARGET = emulador

OBJ_DIR = build

SRCS = src/main.cpp src/ROM/ROM.cpp src/MMU/MMU.cpp src/CPU/CPU.cpp src/Timer/Timer.cpp

OBJS = $(patsubst src/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
