cmake_minimum_required(VERSION 4.1.2)

project(GameBoyEmu)

add_executable(${PROJECT_NAME}
	src/main.c
	src/cart.c
	src/bus.c
	src/cpu.c
)
