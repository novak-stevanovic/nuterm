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

LIB_TYPE ?= so

PREFIX ?= /usr/local

PC_PREFIX ?= /usr/local/lib/pkgconfig

OPT ?= 2
OPT_FLAG = -O$(OPT)

DEBUG ?= 0
ifeq ($(DEBUG),1)
    DEBUG_FLAG = -g
    OPT_FLAG = -O0
endif

# ---------------------------------------------------------

LIB_NAME = nuterm
LIB_PC = $(LIB_NAME).pc

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
# pkgconf
# ---------------------------------------------------------

_PC_PREFIX = $(PREFIX)
_PC_EXEC_PREFIX = $${prefix}
_PC_LIBDIR = $${exec_prefix}/lib
_PC_INCLUDEDIR = $${exec_prefix}/include
_PC_NAME = $(LIB_NAME)
_PC_DESCRIPTION = Terminal event detection, function abstraction.
_PC_VERSION = 1.0.0
_PC_LIBS = -L$${libdir} -l$(LIB_NAME)
_PC_CFLAGS = -I$${includedir}/$(LIB_NAME)
_PC_REQUIRES =
_PC_REQUIRES_PRIVATE =

PC_DEPS = $(_PC_REQUIRES)
ifneq ($(PC_DEPS),)
    PC_CFLAGS = $(shell pkgconf --silence-errors --cflags $(PC_DEPS))
    PC_LFLAGS = $(shell pkgconf --silence-errors --libs $(PC_DEPS))
endif

DEP_CFLAGS = $(PC_CFLAGS)
DEP_LFLAGS = $(PC_LFLAGS)

# ---------------------------------------------------------
# Source Flags
# ---------------------------------------------------------

SRC_CFLAGS_DEBUG = $(DEBUG_FLAG)
SRC_CFLAGS_OPTIMIZATION = $(OPT_FLAG)
SRC_CFLAGS_WARN = -Wall
SRC_CFLAGS_MAKE = -MMD -MP
SRC_CFLAGS_INCLUDE = -Iinclude $(DEP_CFLAGS)

SRC_CFLAGS = -c -fPIC $(SRC_CFLAGS_INCLUDE) $(SRC_CFLAGS_MAKE) \
$(SRC_CFLAGS_WARN) $(SRC_CFLAGS_DEBUG) $(SRC_CFLAGS_OPTIMIZATION)

# ---------------------------------------------------------
# Test Flags
# ---------------------------------------------------------

TEST_CFLAGS_DEBUG = $(DEBUG_FLAG)
TEST_CFLAGS_OPTIMIZATION = -O0
TEST_CFLAGS_WARN = -Wall
TEST_CFLAGS_MAKE = -MMD -MP
TEST_CFLAGS_INCLUDE = -Iinclude $(DEP_CFLAGS)

TEST_CFLAGS = -c $(TEST_CFLAGS_INCLUDE) $(TEST_CFLAGS_MAKE) \
$(TEST_CFLAGS_WARN) $(TEST_CFLAGS_DEBUG) $(TEST_CFLAGS_OPTIMIZATION)

TEST_LFLAGS = -L. -l$(LIB_NAME) $(DEP_LFLAGS) 

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

install: $(LIB_PC)
	@mkdir -p $(PREFIX)/lib # lib
	ln -f $(LIB_FILE) $(PREFIX)/lib
	@mkdir -p $(PREFIX)/include/$(LIB_NAME) # headers
	cp -r $(INSTALL_INCLUDE) $(PREFIX)/include/$(LIB_NAME)
	mkdir -p $(PC_PREFIX) # pc file
	mv $(LIB_PC) $(PC_PREFIX)

$(LIB_PC):
	@echo 'prefix=$(_PC_PREFIX)' > $@
	@echo 'exec_prefix=$(_PC_EXEC_PREFIX)' >> $@
	@echo 'libdir=$(_PC_LIBDIR)' >> $@
	@echo 'includedir=$(_PC_INCLUDEDIR)' >> $@
	@echo '' >> $@
	@echo 'Name: $(_PC_NAME)' >> $@
	@echo 'Description: $(_PC_DESCRIPTION)' >> $@
	@echo 'Version: $(_PC_VERSION)' >> $@
	@echo 'Libs: $(_PC_LIBS)' >> $@
	@echo 'Cflags: $(_PC_CFLAGS)' >> $@
	@echo 'Requires: $(_PC_REQUIRES)' >> $@
	@echo 'Requires.private: $(_PC_REQUIRES_PRIVATE)' >> $@

# uninstall ------------------------------------------------

uninstall:
	rm -f $(PREFIX)/lib/$(LIB_FILE)
	rm -rf $(PREFIX)/include/$(LIB_NAME)
	rm -f $(PC_PREFIX)/$(LIB_PC)

# clean ----------------------------------------------------

clean:
	rm -rf build
	rm -f $(LIB_AR_FILE)
	rm -f $(LIB_SO_FILE)
	rm -f test
	rm -f $(LIB_PC)
	rm -f compile_commands.json
