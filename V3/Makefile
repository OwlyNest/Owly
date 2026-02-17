CC      := gcc
CFLAGS  := -Wall -Wextra -Iinclude -Iinclude/front
LDFLAGS := -lcjson -lstdx

SRC_DIR := src
OBJ_DIR := build
BIN     := owlyc3

SRCS := \
	$(SRC_DIR)/owlyc3.c \
	$(SRC_DIR)/memutils.c \
	$(SRC_DIR)/SA.c \
	$(SRC_DIR)/SA_to_json.c \
	$(SRC_DIR)/ir.c \
	$(SRC_DIR)/front/lexer.c \
	$(SRC_DIR)/front/parser.c \
	$(SRC_DIR)/front/expressions.c \
	$(SRC_DIR)/front/ast.c \
	$(SRC_DIR)/front/ast_to_json.c

OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR) $(OBJ_DIR)/front

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	
clean:
	rm -rf $(OBJ_DIR) $(BIN)