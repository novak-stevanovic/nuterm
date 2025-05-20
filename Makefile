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

LIB_NAME = nuterm

CC = gcc
AR = ar
MAKE = make

C_SRC = $(shell find src -name "*.c")
C_OBJ = $(patsubst src/%.c,build/%.o,$(C_SRC))

INSTALL_INCLUDE = include/nuterm.h include/nt_shared.h include/nt_esc.h

# -----------------------------------------------------------------------------
# Build Flags
# -----------------------------------------------------------------------------

# ---------------------------------------------------------
# Thirdparty
# ---------------------------------------------------------

_UCONV_DIR = thirdparty/uconv
_UTHASH_DIR = thirdparty/uthash

_UCONV_LIB = $(_UCONV_DIR)/libuconv.a

_UCONV_CFLAGS = -I$(_UCONV_DIR)/include
_UTHASH_CFLAGS = -I$(_UTHASH_DIR)/include

_UCONV_LFLAGS = -L$(_UCONV_DIR) -luconv

# ---------------------------------------------------------
# Base Flags
# ---------------------------------------------------------

SRC_CFLAGS_DEBUG = -g
SRC_CFLAGS_OPTIMIZATION = -O2
SRC_CFLAGS_WARN = -Wall
SRC_CFLAGS_MAKE = -MMD -MP
SRC_CFLAGS_INCLUDE = -Iinclude $(_UCONV_CFLAGS) $(_UTHASH_CFLAGS)

SRC_CFLAGS = -c -fPIC $(SRC_CFLAGS_INCLUDE) $(SRC_CFLAGS_MAKE) \
$(SRC_CFLAGS_WARN) $(SRC_CFLAGS_DEBUG) $(SRC_CFLAGS_OPTIMIZATION)

# ---------------------------------------------------------
# Test Flags
# ---------------------------------------------------------

TEST_CFLAGS_DEBUG = -g
TEST_CFLAGS_OPTIMIZATION = -O0
TEST_CFLAGS_WARN = -Wall
TEST_CFLAGS_MAKE = -MMD -MP
TEST_CFLAGS_INCLUDE = -Iinclude

TEST_CFLAGS = -c -fPIC $(TEST_CFLAGS_INCLUDE) $(TEST_CFLAGS_MAKE) \
$(TEST_CFLAGS_WARN) $(TEST_CFLAGS_DEBUG) $(TEST_CFLAGS_OPTIMIZATION)

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
else 
LIB_FILE = $(LIB_SO_FILE)
endif

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

.PHONY: all clean install uninstall

all: $(LIB_FILE)

$(LIB_AR_FILE): $(_UCONV_LIB) $(C_OBJ)
	@mkdir -p build/uconv
	@cd build/uconv && ar -x ../../$(_UCONV_LIB)
	$(AR) rcs $@ $(C_OBJ) build/uconv/*.o

$(LIB_SO_FILE): $(_UCONV_LIB) $(C_OBJ)
	$(CC) -shared $(C_OBJ) $(_UCONV_LIB) -o $@

$(C_OBJ): build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(SRC_CFLAGS) $< -o $@

$(_UCONV_LIB): $(_UCONV_DIR)
	make -C $(_UCONV_DIR) LIB_TYPE=archive

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
	cp -r $(INSTALL_INCLUDE) $(PREFIX)/include/$(LIB_NAME)

# uninstall ------------------------------------------------

uninstall:
	rm -f $(PREFIX)/lib/$(LIB_FILE)
	rm -rf $(PREFIX)/include/$(LIB_NAME)

# clean ----------------------------------------------------

clean:
	@$(MAKE) -C thirdparty/uconv clean
	rm -rf build
	rm -f $(LIB_AR_FILE)
	rm -f $(LIB_SO_FILE)
	rm -f test
	rm -f compile_commands.json
