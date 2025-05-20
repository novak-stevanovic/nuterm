# -----------------------------------------------------------------------------
# Validation & Global Settings
# -----------------------------------------------------------------------------

GOAL_COUNT := $(words $(MAKECMDGOALS))

ifneq ($(GOAL_COUNT),1)
    ifneq ($(GOAL_COUNT),0)
        $(error You cannot specify more than 1 target (got $(GOAL_COUNT): $(MAKECMDGOALS)))
    endif
endif

# ---------------------------------------------------------

ifndef LIB_TYPE
    LIB_TYPE = so
endif

ifneq ($(LIB_TYPE),so)
    ifneq ($(LIB_TYPE),ar)
        $(error Invalid settings. USAGE: make [TARGET] [LIB_TYPE=so/ar] [OPT={0..3}])
    endif
endif

# ---------------------------------------------------------

ifndef PREFIX
    PREFIX = /usr/local
endif

# ---------------------------------------------------------

ifndef OPT
    OPT = 2
endif

ifneq ($(OPT),0)
    ifneq ($(OPT),1)
        ifneq ($(OPT),2)
            ifneq ($(OPT),3)
                $(error Invalid settings. USAGE: make [TARGET] [LIB_TYPE=so/ar] [OPT={0..3}])
            endif
        endif
    endif
endif

OPT_FLAG = -O$(OPT)

# ---------------------------------------------------------

LIB_NAME = uconv

CC = gcc
AR = ar
MAKE = make

C_SRC = $(shell find src -name "*.c")
C_OBJ = $(patsubst src/%.c,build/%.o,$(C_SRC))

INSTALL_INCLUDE = include/uconv.h

# -----------------------------------------------------------------------------
# Build Flags
# -----------------------------------------------------------------------------

# ---------------------------------------------------------
# Thirdparty
# ---------------------------------------------------------

THIRDPARTY_CFLAGS =

# ---------------------------------------------------------
# Base Flags
# ---------------------------------------------------------

SRC_CFLAGS_DEBUG = -g
SRC_CFLAGS_OPTIMIZATION = $(OPT_FLAG)
SRC_CFLAGS_WARN = -Wall
SRC_CFLAGS_MAKE = -MMD -MP
SRC_CFLAGS_INCLUDE = -Iinclude $(THIRDPARTY_CFLAGS)

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

TEST_CFLAGS = -c $(TEST_CFLAGS_INCLUDE) $(TEST_CFLAGS_MAKE) \
$(TEST_CFLAGS_WARN) $(TEST_CFLAGS_DEBUG) $(TEST_CFLAGS_OPTIMIZATION)

TEST_LFLAGS = -L. -l$(LIB_NAME)

ifeq ($(LIB_TYPE),so)
    TEST_LFLAGS += -Wl,-rpath,.
endif

# ---------------------------------------------------------
# Lib Make
# ---------------------------------------------------------

LIB_AR_FILE = lib$(LIB_NAME).a
LIB_SO_FILE = lib$(LIB_NAME).so

ifeq ($(LIB_TYPE), so)
    LIB_FILE = $(LIB_SO_FILE)
else 
    LIB_FILE = $(LIB_AR_FILE)
endif

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------

.PHONY: all clean install uninstall

all: $(LIB_FILE)

$(LIB_AR_FILE): $(C_OBJ)
	$(AR) rcs $@ $(C_OBJ)

$(LIB_SO_FILE): $(C_OBJ)
	$(CC) -shared $(C_OBJ) -o $@

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
	cp -r $(INSTALL_INCLUDE) $(PREFIX)/include/$(LIB_NAME)

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
