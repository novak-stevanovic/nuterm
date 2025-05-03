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

ifndef PC_PREFIX
    PC_PREFIX = /usr/local/lib/pkgconfig
endif

ifneq ($(LIB_TYPE),shared)
    ifneq ($(LIB_TYPE),archive)
        $(error Invalid LIB_TYPE. USAGE: make [TARGET] [LIB_TYPE=shared/archive])
    endif
endif

LIB_NAME = nuterm
_LIB_NAME = _$(LIB_NAME) # internal

PC_FILE = $(LIB_NAME).pc
_PC_FILE = _$(PC_FILE) # internal

CC = gcc
# CC = /opt/arm-gnu-toolchain-14.2.rel1-x86_64-aarch64_be-none-linux-gnu/bin/aarch64_be-none-linux-gnu-gcc

C_SRC = $(shell find src -name "*.c")
C_OBJ = $(patsubst src/%.c,build/%.o,$(C_SRC))

# -----------------------------------------------------------------------------
# Build Flags
# -----------------------------------------------------------------------------

# ---------------------------------------------------------
# PKGConfig Dependency Flags
# ---------------------------------------------------------

PC_DEPS = $(shell PKG_CONFIG_PATH=$(PWD):$PC_CONFIG_PATH \
pkg-config --print-requires $(_LIB_NAME))

PC_DEPS_CFLAGS = $(foreach dep,$(PC_DEPS),$(shell pkg-config --cflags $(dep)))

PC_DEPS_LIBS = $(foreach dep,$(PC_DEPS),$(shell pkg-config --libs $(dep)))

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

BASE_CFLAGS += $(PC_DEPS_CFLAGS)

# ---------------------------------------------------------
# C Source Flags
# ---------------------------------------------------------

SRC_CFLAGS = $(BASE_CFLAGS)

# ---------------------------------------------------------
# Test Flags
# ---------------------------------------------------------

TEST_CFLAGS = $(BASE_CFLAGS)

TEST_LFLAGS = -L. -l$(LIB_NAME)
TEST_LFLAGS += $(PC_DEPS_LIBS)

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

install: $(PC_FILE)
	@mkdir -p $(PREFIX)/lib
	cp $(LIB_FILE) $(PREFIX)/lib

	@mkdir -p $(PREFIX)/include/$(LIB_NAME)
	cp -r include/* $(PREFIX)/include/$(LIB_NAME)

	@mkdir -p $(PC_PREFIX)
	cp $(PC_FILE) $(PC_PREFIX)

$(PC_FILE): $(__PC_FILE)
	@sed 's|@prefix@|$(PREFIX)|g' $< > $@

# uninstall ------------------------------------------------

uninstall:
	rm -f $(PREFIX)/lib/$(LIB_SO_FILE)
	rm -f $(PREFIX)/lib/$(LIB_AR_FILE)
	rm -rf $(PREFIX)/include/$(LIB_NAME)
	rm -f $(PC_PREFIX)/$(PC_FILE)

# clean ----------------------------------------------------

clean:
	rm -rf build
	rm -f $(LIB_AR_FILE)
	rm -f $(LIB_SO_FILE)
	rm -f test
	rm -f compile_commands.json
	rm -f $(PC_FILE)
