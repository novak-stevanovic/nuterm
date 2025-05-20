# -----------------------------------------------------------------------------
# Validaton & Global Settings
# -----------------------------------------------------------------------------

GOAL_COUNT := $(words $(MAKECMDGOALS))

ifneq ($(GOAL_COUNT),1)
    ifneq ($(GOAL_COUNT),0)
        $(error You cannot specify more than 1 target (got $(GOAL_COUNT): $(MAKECMDGOALS)))
    endif
endif

ifndef LIB_TYPE
    LIB_TYPE = shared
endif

ifndef PREFIX
    PREFIX = /usr/local
endif

ifneq ($(LIB_TYPE),shared)
    ifneq ($(LIB_TYPE),archive)
        $(error Invalid LIB_TYPE. USAGE: make [TARGET] [LIB_TYPE=shared/archive])
    endif
endif

LIB_NAME = uconv

CC = gcc

C_SRC = $(shell find src -name "*.c")
C_OBJ = $(patsubst src/%.c,build/%.o,$(C_SRC))

# -----------------------------------------------------------------------------
# Build Flags
# -----------------------------------------------------------------------------

# ---------------------------------------------------------
# Base Flags
# ---------------------------------------------------------

BASE_CFLAGS_DEBUG = -g
BASE_CFLAGS_OPTIMIZATION = -O0
BASE_CFLAGS_WARN = -Wall
BASE_CFLAGS_MAKE = -MMD -MP
BASE_CFLAGS_INCLUDE = -Iinclude

BASE_CFLAGS = -c -fPIC $(BASE_CFLAGS_INCLUDE) $(BASE_CFLAGS_MAKE) \
$(BASE_CFLAGS_WARN) $(BASE_CFLAGS_DEBUG) $(BASE_CFLAGS_OPTIMIZATION)

# ---------------------------------------------------------
# C Source Flags
# ---------------------------------------------------------

SRC_CFLAGS = $(BASE_CFLAGS)

# ---------------------------------------------------------
# Test Flags
# ---------------------------------------------------------

TEST_CFLAGS = $(BASE_CFLAGS)

TEST_LFLAGS = -L. -l$(LIB_NAME)

ifeq ($(LIB_TYPE),shared)
TEST_LFLAGS += -Wl,-rpath,.
endif

# ---------------------------------------------------------
# Lib Make
# ---------------------------------------------------------

LIB_AR_FILE = lib$(LIB_NAME).a
LIB_SO_FILE = lib$(LIB_NAME).so

ifeq ($(LIB_TYPE), archive)
LIB_FILE = $(LIB_AR_FILE)
LIB_MAKE = ar rcs $(LIB_FILE) $(C_OBJ)
else 
LIB_FILE = $(LIB_SO_FILE)
LIB_MAKE = $(CC) -shared $(C_OBJ) -o $(LIB_FILE)
endif

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

.PHONY: all clean install uninstall 

all: $(LIB_FILE)

$(LIB_FILE): $(C_OBJ)
	$(LIB_MAKE)

$(C_OBJ): build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(SRC_CFLAGS) $< -o $@

# test -----------------------------------------------------

test: $(C_OBJ) build/tests.o $(LIB_FILE)
	$(CC) build/tests.o -o $@ $(TEST_LFLAGS)

build/tests.o: tests.c
	@mkdir -p $(dir $@)
	$(CC) $(TEST_CFLAGS) tests.c -o $@

# install --------------------------------------------------

install:
	@mkdir -p $(PREFIX)/lib
	cp $(LIB_FILE) $(PREFIX)/lib

	@mkdir -p $(PREFIX)/include/$(LIB_NAME)
	cp -r include/* $(PREFIX)/include/$(LIB_NAME)

# uninstall ------------------------------------------------

uninstall:
	rm -f $(PREFIX)/lib/$(LIB_FILE)
	rm -rf $(PREFIX)/include/$(LIB_NAME)

# clean ----------------------------------------------------

clean:
	rm -rf build
	rm -f $(LIB_AR_FILE)
	rm -f $(LIB_SO_FILE)
	rm -f test
	rm -f compile_commands.json
