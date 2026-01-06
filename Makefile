# -----------------------------------------------------------------------------
# Validation
# -----------------------------------------------------------------------------

GOAL_COUNT := $(words $(MAKECMDGOALS))

ifneq ($(GOAL_COUNT),1)
    ifneq ($(GOAL_COUNT),0)
        $(error You cannot specify more than 1 target (got $(GOAL_COUNT): $(MAKECMDGOALS)))
    endif
endif

# -----------------------------------------------------------------------------
# Public Settings
# -----------------------------------------------------------------------------

LIB_TYPE ?= so

PREFIX ?= /usr/local
PC_PREFIX ?= $(PREFIX)/lib/pkgconfig
PC_WITH_PATH =

OPT ?= 3

CC = gcc
AR = ar
MAKE = make

# -----------------------------------------------------------------------------
# Private Settings
# -----------------------------------------------------------------------------

LIB_NAME = nuterm
LIB_PC = $(LIB_NAME).pc

C_SRC = $(shell find src -name "*.c")
C_OBJ = $(patsubst src/%.c,build/%.o,$(C_SRC))

INSTALL_INCLUDE = include/nt.h include/nt_gfx.h include/nt_event.h include/nt_status.h 

OPT_FLAG = -O$(OPT)

DEBUG ?= 1
ifeq ($(DEBUG),1)
    DEBUG_FLAG = -g
    OPT_FLAG = -O0
endif

# -----------------------------------------------------------------------------
# Build Flags
# -----------------------------------------------------------------------------

# ---------------------------------------------------------
# pkgconf
# ---------------------------------------------------------

PC_DEPS = uconv

_PC_PREFIX = $(PREFIX)
_PC_EXEC_PREFIX = $${prefix}
_PC_LIBDIR = $${exec_prefix}/lib
_PC_INCLUDEDIR = $${exec_prefix}/include
_PC_NAME = $(LIB_NAME)
_PC_DESCRIPTION = Terminal event detection, function abstraction.
_PC_VERSION = 1.0.0
_PC_LIBS = -L$${libdir} -l$(LIB_NAME) -pthread
_PC_CFLAGS = -I$${includedir}/$(LIB_NAME)
_PC_REQUIRES = $(PC_DEPS)
_PC_REQUIRES_PRIVATE =

ifneq ($(PC_DEPS),)
    DEP_CFLAGS = $(shell pkgconf --with-path=$(PC_WITH_PATH) --silence-errors --cflags $(PC_DEPS))
    DEP_LFLAGS = $(shell pkgconf --with-path=$(PC_WITH_PATH) --silence-errors --libs $(PC_DEPS))
endif

# ---------------------------------------------------------
# Source Flags
# ---------------------------------------------------------

SRC_CFLAGS_STD = -std=c99
SRC_CFLAGS_DEBUG = $(DEBUG_FLAG)
SRC_CFLAGS_OPTIMIZATION = $(OPT_FLAG)
SRC_CFLAGS_WARN = -Wall
SRC_CFLAGS_MAKE = -MMD -MP
SRC_CFLAGS_INCLUDE = -Iinclude $(DEP_CFLAGS)

SRC_CFLAGS = -c -fPIC -pthread $(SRC_CFLAGS_STD) $(SRC_CFLAGS_INCLUDE) $(SRC_CFLAGS_MAKE) \
$(SRC_CFLAGS_WARN) $(SRC_CFLAGS_DEBUG) $(SRC_CFLAGS_OPTIMIZATION)

# ---------------------------------------------------------
# Test Flags
# ---------------------------------------------------------

DEMO_CFLAGS_STD = -std=c99
DEMO_CFLAGS_DEBUG = $(DEBUG_FLAG)
DEMO_CFLAGS_OPTIMIZATION = -O0
DEMO_CFLAGS_WARN = -Wall
DEMO_CFLAGS_MAKE = -MMD -MP
DEMO_CFLAGS_INCLUDE = -Iinclude $(DEP_CFLAGS)

DEMO_CFLAGS = -c -pthread $(DEMO_CFLAGS_STD) $(DEMO_CFLAGS_INCLUDE) $(DEMO_CFLAGS_MAKE) \
$(DEMO_CFLAGS_WARN) $(DEMO_CFLAGS_DEBUG) $(DEMO_CFLAGS_OPTIMIZATION)

DEMO_LFLAGS = -L. -l$(LIB_NAME) $(DEP_LFLAGS) 

ifeq ($(LIB_TYPE),so)
    DEMO_LFLAGS += -Wl,-rpath,.
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

# demo -----------------------------------------------------

demo: $(C_OBJ) build/demo.o $(LIB_FILE)
	$(CC) build/demo.o -o $@ $(DEMO_LFLAGS)

build/demo.o: demo.c
	@mkdir -p $(dir $@)
	$(CC) $(DEMO_CFLAGS) demo.c -o $@

# install --------------------------------------------------

install: $(LIB_PC)
	@mkdir -p $(PREFIX)/lib # lib
	ln -f $(LIB_FILE) $(PREFIX)/lib
	@mkdir -p $(PREFIX)/include/$(LIB_NAME) # headers
	cp -r $(INSTALL_INCLUDE) $(PREFIX)/include/$(LIB_NAME)
	@mkdir -p $(PC_PREFIX) # pc file
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
	rm -f demo
	rm -f $(LIB_PC)
	rm -f compile_commands.json
