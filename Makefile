CC = clang
AR ?= ar
CFLAGS ?= -std=c99 -g -Wall -Wpedantic -fsanitize=address,undefined
LDFLAGS ?= -g -lm -Wall -Wpedantic -fsanitize=address,undefined
PREFIX?=/usr/bin/

SRC_DIR=src
BUILD_DIR?=build

SOURCES=ast.c ast.free.c lex.c var.c list.c map.c builtins/bfunc.c builtins/bfunc.rt.c builtins/bfunc.str.c builtins/bfunc.list.c builtins/bfunc.io.c builtins/bfunc.sys.c builtins/bfunc.math.c builtins/bfunc.rand.c builtins/bfunc.log.c builtins/bfunc.time.c pool.c ldfile.c arena.c cvt.c struct.c regalloc.c
COMPILER_SOURCES=frontend.c
DECOMPILER_SOURCES=dc.c
RUNTIME_SOURCES=execprog.c

OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

COMPILER_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(COMPILER_SOURCES))
DECOMPILER_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(DECOMPILER_SOURCES))
RUNTIME_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(RUNTIME_SOURCES))

.PHONY: all clean

all: $(BUILD_DIR) $(BUILD_DIR)/libesl.a $(BUILD_DIR)/ec $(BUILD_DIR)/dc $(BUILD_DIR)/eexec

$(BUILD_DIR)/libesl.a: $(OBJ) $(BUILD_DIR)/cc.o $(BUILD_DIR)/exec.o
	$(AR) rcs $@ $^	

$(BUILD_DIR)/ec: $(COMPILER_OBJ) $(BUILD_DIR)/libesl.a
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/dc: $(DECOMPILER_OBJ) $(BUILD_DIR)/libesl.a
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/eexec: $(RUNTIME_OBJ) $(BUILD_DIR)/libesl.a
	$(CC) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/rt/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(RUNTIME_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/cc/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/builtins

$(BUILD_DIR)/%.eb: %.e $(BUILD_DIR)/ec | $(BUILD_DIR)
	$(BUILD_DIR)/ec $< -o $@

clean:
	rm -rf $(BUILD_DIR)

install: $(BUILD_DIR)/ec $(BUILD_DIR)/eexec
	ln -sf -T $(realpath $(BUILD_DIR)/ec) $(PREFIX)/ec
	ln -sf -T $(realpath $(BUILD_DIR)/eexec) $(PREFIX)/eexec
	ln -sf -T $(realpath ./erun.sh) $(PREFIX)/erun.sh
