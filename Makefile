CC      ?= cc
AR      ?= ar
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
PREFIX  ?= /usr/local
DESTDIR ?=

BUILD      := build
LIB        := $(BUILD)/libask.a
OBJ        := $(BUILD)/ask.o
TEST_BIN   := $(BUILD)/test_ask
ALLOC_BIN  := $(BUILD)/test_alloc
EXAMPLE    := $(BUILD)/basic

.PHONY: all lib example test check clean install

all: lib example

lib: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(OBJ): src/ask.c src/ask.h | $(BUILD)
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

example: $(EXAMPLE)

$(EXAMPLE): examples/basic.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) -Isrc $< $(LIB) -o $@

test: $(TEST_BIN) $(ALLOC_BIN)
	./$(TEST_BIN)
	./$(ALLOC_BIN)

$(TEST_BIN): tests/test_ask.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) -Isrc $< $(LIB) -o $@

# Compile ask.c with failure-injecting allocators to exercise OOM paths.
$(ALLOC_BIN): tests/test_alloc.c tests/test_alloc.h src/ask.c src/ask.h | $(BUILD)
	$(CC) $(CFLAGS) -Isrc -include tests/test_alloc.h \
		-DASK_MALLOC=ask_test_malloc -DASK_REALLOC=ask_test_realloc \
		-DASK_FREE=ask_test_free src/ask.c $< -o $@

check: test

$(BUILD):
	mkdir -p $(BUILD)

install: lib
	install -d $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	install -m 644 src/ask.h $(DESTDIR)$(PREFIX)/include/ask.h
	install -m 644 $(LIB) $(DESTDIR)$(PREFIX)/lib/libask.a

clean:
	rm -rf $(BUILD)
