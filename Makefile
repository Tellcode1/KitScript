CC=clang
CFLAGS ?= -std=c99 -Wall -Wpedantic -g3 -fPIC -fsanitize=address,undefined
LDFLAGS ?= -lm -Wall -Wpedantic -g3 -fsanitize=address,undefined
PREFIX?=/usr/bin/

SRC_DIR=src
BUILD_DIR?=build
COMPILER_SOURCES=compiler/codegraph/codegraph.arg.vector.move.coalesce.c compiler/codegraph/codegraph.c compiler/codegraph/codegraph.constant.folding.c compiler/codegraph/codegraph.constant.propogation.c compiler/codegraph/codegraph.copy.propogation.c compiler/codegraph/codegraph.dce.c compiler/codegraph/codegraph.domination.c compiler/codegraph/codegraph.dse.c compiler/codegraph/codegraph.function.inlining.c compiler/codegraph/codegraph.licm.c compiler/codegraph/codegraph.liveliness.c compiler/codegraph/codegraph.loop.analysis.c compiler/codegraph/codegraph.register.allocation.c compiler/ast.extract.info.c compiler/compile.accessors.c compiler/compile.assignments.c compiler/compile.containers.c compiler/compile.func.c compiler/compile.if.c compiler/compile.loops.c compiler/compile.operators.c compiler/compile.small.statements.c compiler/compile.struct.c compiler/compile.variable.decleration.c compiler/defer.c compiler/dump.asm.c compiler/kit.cc.c compiler/kit.lvalue.c compiler/peephole.optimizers.c compiler/scopes.c compiler/tables.c
LIB_SOURCES=$(COMPILER_SOURCES) kit.ast.c kit.ast.free.c kit.lex.c kit.var.c kit.list.c kit.list.sort.c kit.map.c builtins/kit.bfunc.c builtins/kit.bfunc.rt.c builtins/kit.bfunc.str.c builtins/kit.bfunc.list.c builtins/kit.bfunc.io.c builtins/kit.bfunc.sys.c builtins/kit.bfunc.math.c builtins/kit.bfunc.rand.c builtins/kit.bfunc.log.c builtins/kit.bfunc.time.c kit.pool.c kit.ldfile.c kit.arena.c kit.cvt.c kit.struct.c kit.exec.c kit.vm.c
CLI_SOURCES = kit.cli.c

OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(LIB_SOURCES))
CLI_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(CLI_SOURCES))

.PHONY: all clean

all: $(BUILD_DIR) $(BUILD_DIR)/kscript

$(BUILD_DIR)/libKScript.a: $(OBJ)
	ar rcs $@ $(OBJ)

$(BUILD_DIR)/kscript: $(CLI_OBJ) $(BUILD_DIR)/libKScript.a
	$(CC) $^ -o $@ $(LDFLAGS)

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

install: $(BUILD_DIR)/kscript
	ln -sf -T $(realpath $(BUILD_DIR)/kscript) $(PREFIX)/kscript
