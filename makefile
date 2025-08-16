CC = gcc
CFLAGS = -Wall -Werror -Iinclude $(shell pkg-config --cflags sdl3)
LDFLAGS = $(shell pkg-config --libs sdl3)


SRC_DIR = src
SRC = $(SRC_DIR)/main.c $(SRC_DIR)/chip8.c $(SRC_DIR)/functions.c
OBJ_DIR = obj
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

