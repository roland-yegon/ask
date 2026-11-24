CC      ?= cc
AR      ?= ar
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2
PREFIX  ?= /usr/local
DESTDIR ?=

BUILD      := build
LIB        := $(BUILD)/libask.a
OBJ        := $(BUILD)/ask.o
TEST_BIN   := $(BUILD)/test_ask
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

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): tests/test_ask.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) -Isrc $< $(LIB) -o $@

check: test

$(BUILD):
	mkdir -p $(BUILD)

install: lib
	install -d $(DESTDIR)$(PREFIX)/include $(DESTDIR)$(PREFIX)/lib
	install -m 644 src/ask.h $(DESTDIR)$(PREFIX)/include/ask.h
	install -m 644 $(LIB) $(DESTDIR)$(PREFIX)/lib/libask.a

clean:
	rm -rf $(BUILD)
