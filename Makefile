# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu11 -pthread
INCLUDE = -Iinclude
LIBS = -Lbin -ltslog

# Diretórios
SRC_DIR = src
TEST_DIR = tests
BIN_DIR = bin

# Alvos principais
.PHONY: all clean libtslog

all: $(BIN_DIR)/libtslog.a $(BIN_DIR)/servidor_chat $(BIN_DIR)/cliente_chat $(BIN_DIR)/log_test

# Garante que a pasta bin exista
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compilação da biblioteca estática
$(BIN_DIR)/libtslog.a: $(SRC_DIR)/libtslog.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $(BIN_DIR)/libtslog.o
	ar rcs $@ $(BIN_DIR)/libtslog.o

# Compilação dos programas principais
$(BIN_DIR)/servidor_chat: $(SRC_DIR)/servidor_chat.c $(BIN_DIR)/libtslog.a | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $< $(LIBS)

$(BIN_DIR)/cliente_chat: $(SRC_DIR)/cliente_chat.c $(BIN_DIR)/libtslog.a | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $< $(LIBS)

# Compilação do teste
$(BIN_DIR)/log_test: $(SRC_DIR)/libtslog.c $(TEST_DIR)/log_test.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) $^ -o $@

# Limpeza
clean:
	rm -rf $(BIN_DIR)
	rm -rf ./saida.log
	rm -rf ./servidor_chat.log
