CC = gcc
CFLAGS = -Wall -Werror -Iinclude $(shell pkg-config --cflags sdl3)
LDFLAGS = $(shell pkg-config --libs sdl3)


SRC_DIR = src
SRC = $(SRC_DIR)/main.c $(SRC_DIR)/chip8.c $(SRC_DIR)/functions.c
OBJ_DIR = obj
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

TARGET = emul8 

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -rf $(OBJ_DIR) $(TARGET)


remake:
	make clean
	make

