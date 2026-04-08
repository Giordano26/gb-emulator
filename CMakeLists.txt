cmake_minimum_required(VERSION 3.14)

project(gabs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_package(SDL2 REQUIRED)

set(SOURCES
    src/main.cpp
    src/ROM/ROM.cpp
    src/MMU/MMU.cpp
    src/CPU/CPU.cpp
    src/Timer/Timer.cpp
    src/PPU/PPU.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

if(TARGET SDL2::SDL2)
  target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
else()
  target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
endif()
