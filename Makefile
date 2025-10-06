# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread -Iinclude
DEBUG = -g
RELEASE = -O2

# Diretórios
SRC_DIR = src
TEST_DIR = tests
BIN_DIR = bin

# Alvos principais
.PHONY: all clean rebuild debug

all: $(BIN_DIR)/servidor_chat $(BIN_DIR)/cliente_chat $(BIN_DIR)/log_test

# Garante que a pasta bin exista
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compilação dos programas principais
$(BIN_DIR)/servidor_chat: $(SRC_DIR)/servidor_chat.c $(SRC_DIR)/libtslog.c $(SRC_DIR)/thread_safe_queue.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(RELEASE) -o $@ $^

$(BIN_DIR)/cliente_chat: $(SRC_DIR)/cliente_chat.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(RELEASE) -o $@ $^

# Compilação do teste
$(BIN_DIR)/log_test: $(TEST_DIR)/log_test.c $(SRC_DIR)/libtslog.c $(SRC_DIR)/thread_safe_queue.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(DEBUG) -o $@ $^

# Limpeza
clean:
	rm -rf $(BIN_DIR)
	rm -f ./saida.log ./servidor_chat.log

# Rebuild
rebuild: clean all

# Debug build
debug: CFLAGS += $(DEBUG)
debug: rebuild
