CC ?= cc
CFLAGS ?= -std=c99 -g3 -Wall -Wpedantic -fsanitize=undefined
LDFLAGS ?= -g3 -lm -fsanitize=undefined

SRC_DIR=.
BUILD_DIR?=build

SOURCES=ast.c ast.free.c lex.c var.c list.c map.c bfunc.c bfunc.str.c bfunc.list.c bfunc.io.c bfunc.sys.c bfunc.math.c bfunc.rand.c bfunc.time.c pool.c ldfile.c validate.c stackemu.c stack.c arena.c cfold.c
COMPILER_SOURCES=cc.c frontend.c $(SOURCES)
DECOMPILER_SOURCES=dc.c $(SOURCES)
RUNTIME_SOURCES=exec.c execprog.c $(SOURCES)

OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

COMPILER_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(COMPILER_SOURCES))
DECOMPILER_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(DECOMPILER_SOURCES))
RUNTIME_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(RUNTIME_SOURCES))
SCRIPTS = $(wildcard tests/*.e)
COMPILED_SCRIPTS = $(patsubst tests/%.e,$(BUILD_DIR)/%.eb,$(SCRIPTS))

.PHONY: all clean clean_scripts

all: $(BUILD_DIR) $(BUILD_DIR)/libesl.a $(BUILD_DIR)/ec $(BUILD_DIR)/dc $(BUILD_DIR)/eexec

$(BUILD_DIR)/libesl.a: $(OBJ) $(BUILD_DIR)/cc.o $(BUILD_DIR)/exec.o
	ar rcs $@ $^	

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

$(BUILD_DIR)/%.eb: %.e $(BUILD_DIR)/ec | $(BUILD_DIR)
	$(BUILD_DIR)/ec $< -o $@

clean:
	rm -rf $(BUILD_DIR)