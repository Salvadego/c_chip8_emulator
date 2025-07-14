CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c99 -I/usr/include/SDL2 -D_REENTRANT -lSDL2
TARGET_DIR = bin
TARGET = $(TARGET_DIR)/chip8
SRC = ./src/main.c
OBJ = $(SRC:.c=.o)
DEBUGFLAGS = -DDEBUG

.PHONY: all clean run

all: $(TARGET_DIR) $(TARGET)

$(TARGET_DIR):
	@mkdir -p $(TARGET_DIR)

$(TARGET): $(OBJ)
	@$(CC) -o $@ $< $(CFLAGS)

$(SRC:.c=.o): $(SRC)
	@$(CC) $(CFLAGS) -c $< -o $@

run: all
	@$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)
	rm -rf $(TARGET_DIR)

debug: CFLAGS += -DDEBUG
debug: clean all

