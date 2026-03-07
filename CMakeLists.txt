cmake_minimum_required(VERSION 4.1.2)

project(GameBoyEmu)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wextra -std=c23")

add_executable(${PROJECT_NAME}
	src/main.c
	src/cart.c
	src/bus.c
	src/cpu.c
	src/instructions.c
	src/stack.c
	src/io.c
	src/dbg.c
	src/emu.c
	src/ppu.c
	src/oam.c

	src/cJSON.c
)

find_package(SDL3 REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC SDL3)
