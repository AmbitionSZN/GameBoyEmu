cmake_minimum_required(VERSION 4.1.2)

project(GameBoyEmu)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -lSDL3")

add_executable(${PROJECT_NAME}
	src/main.c
	src/cart.c
	src/bus.c
	src/cpu.c
	src/instructions.c
	src/stack.c

	src/cJSON.c
)
