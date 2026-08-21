# =============================================================================
# PUBLIC
# =============================================================================

LIB := nuterm

PREFIX := /usr/local
PC_PREFIX := $(PREFIX)/lib/pkgconfig
PC_WITH_PATH :=

CC := gcc
AR := ar

DEBUG ?= 0

# ---------------------------------------------------------
# Thirdparty dependencies
# ---------------------------------------------------------

# ---------------------------------------------------------
# RELEASE BUILD
# ---------------------------------------------------------

SRC_CFLAGS_REL := -Iinclude -std=c11 -D_POSIX_C_SOURCE=200809L -O3 -flto -Wall -Wfatal-errors -MMD -MP -pthread
SRC_CFLAGS_SO_REL := -fPIC
SRC_CFLAGS_AR_REL :=

SO_CFLAGS_REL := -flto -pthread
SO_LIBS_REL :=

AR_FLAGS_REL := rcs

# ---------------------------------------------------------
# DEBUG BUILD
# ---------------------------------------------------------

SRC_CFLAGS_DEB := -Iinclude -std=c99 -D_POSIX_C_SOURCE=200809L -O0 -Wall -Wextra -Wpedantic -g -MMD -MP -pthread # -fsanitize=address
SRC_CFLAGS_SO_DEB := -fPIC
SRC_CFLAGS_AR_DEB :=

SO_CFLAGS_DEB := -pthread
SO_LIBS_DEB :=

AR_FLAGS_DEB := rcs

# -----------------------------------------------------------------------------
# demo (links with .so)
# -----------------------------------------------------------------------------

DEMO_CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L -O0 -Wall -Wfatal-errors -Iinclude -pthread -g
DEMO_LIBS := -Wl,-rpath,'$$ORIGIN' -L. -l$(LIB) -pthread

# =============================================================================
# PRIVATE
# =============================================================================

ifeq ($(DEBUG),1)
    SRC_CFLAGS := $(SRC_CFLAGS_DEB)
    SRC_CFLAGS_SO := $(SRC_CFLAGS_SO_DEB)
    SRC_CFLAGS_AR := $(SRC_CFLAGS_AR_DEB)

    SO_CFLAGS := $(SO_CFLAGS_DEB)
    SO_LIBS := $(SO_LIBS_DEB)

    AR_FLAGS := $(AR_FLAGS_DEB)
else
    SRC_CFLAGS := $(SRC_CFLAGS_REL)
    SRC_CFLAGS_SO := $(SRC_CFLAGS_SO_REL)
    SRC_CFLAGS_AR := $(SRC_CFLAGS_AR_REL)

    SO_CFLAGS := $(SO_CFLAGS_REL)
    SO_LIBS := $(SO_LIBS_REL)

    AR_FLAGS := $(AR_FLAGS_REL)
endif

C_SRC := $(shell find src -name "*.c")
SO_OBJ := $(patsubst src/%.c,build/so/%.o,$(C_SRC))
AR_OBJ := $(patsubst src/%.c,build/ar/%.o,$(C_SRC))

SO_DEP := $(SO_OBJ:.o=.d)
AR_DEP := $(AR_OBJ:.o=.d)
C_DEP := $(SO_DEP) $(AR_DEP)

LIB_SO := lib$(LIB).so
LIB_AR := lib$(LIB).a
LIB_PC := $(LIB).pc

INSTALL_INCLUDE := include/nt_shared.h include/nt.h include/nt_event.h \
	include/nt_gfx.h include/nt_error.h

# ---------------------------------------------------------
# Pkgconf
# ---------------------------------------------------------

PC_EXEC_PREFIX := $${prefix}
PC_LIBDIR := $${exec_prefix}/lib
PC_INCLUDEDIR := $${prefix}/include

PC_NAME := $(LIB)
PC_DESCRIPTION := Terminal abstraction layer library
PC_VERSION := 1.0.0

PC_LIBS := -L$${libdir} -l$(LIB)
PC_LIBS_PRIVATE := -pthread
PC_CFLAGS := -I$${includedir}/$(LIB)

PC_REQUIRES :=
PC_REQUIRES_PRIVATE :=

# =============================================================================
# TARGETS
# =============================================================================

.PHONY: so ar demo clean install install-so install-ar install-common uninstall

# ---------------------------------------------------------
# SO
# ---------------------------------------------------------

so: $(LIB_SO)

$(LIB_SO): $(SO_OBJ)
	$(CC) -shared $(SO_CFLAGS) $(SO_OBJ) -o $@ $(SO_LIBS)

$(SO_OBJ): build/so/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(SRC_CFLAGS) $(SRC_CFLAGS_SO) $< -o $@

# ---------------------------------------------------------
# AR
# ---------------------------------------------------------

ar: $(LIB_AR)

$(LIB_AR): $(AR_OBJ)
	$(AR) $(AR_FLAGS) $@ $(AR_OBJ)

$(AR_OBJ): build/ar/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(SRC_CFLAGS) $(SRC_CFLAGS_AR) $< -o $@

# ---------------------------------------------------------
# demo
# ---------------------------------------------------------

demo: demo.c so
	$(CC) $(DEMO_CFLAGS) $< -o $@ $(DEMO_LIBS)

# ---------------------------------------------------------
# pkgconf
# ---------------------------------------------------------

$(LIB_PC):
	@echo 'prefix=$(PREFIX)' > $@
	@echo 'exec_prefix=$(PC_EXEC_PREFIX)' >> $@
	@echo 'libdir=$(PC_LIBDIR)' >> $@
	@echo 'includedir=$(PC_INCLUDEDIR)' >> $@
	@echo '' >> $@
	@echo 'Name: $(PC_NAME)' >> $@
	@echo 'Description: $(PC_DESCRIPTION)' >> $@
	@echo 'Version: $(PC_VERSION)' >> $@
	@echo 'Requires: $(PC_REQUIRES)' >> $@
	@echo 'Requires.private: $(PC_REQUIRES_PRIVATE)' >> $@
	@echo 'Libs: $(PC_LIBS)' >> $@
	@echo 'Libs.private: $(PC_LIBS_PRIVATE)' >> $@
	@echo 'Cflags: $(PC_CFLAGS)' >> $@

# ---------------------------------------------------------
# install
# ---------------------------------------------------------

install: install-so install-ar

install-common: $(LIB_PC)
	install -d $(PREFIX)/include/$(LIB)
	install -m 644 $(INSTALL_INCLUDE) $(PREFIX)/include/$(LIB)/
	install -d $(PC_PREFIX)
	install -m 644 $(LIB_PC) $(PC_PREFIX)/$(LIB_PC)

install-so: so install-common
	install -d $(PREFIX)/lib
	install -m 755 $(LIB_SO) $(PREFIX)/lib/$(LIB_SO)

install-ar: ar install-common
	install -d $(PREFIX)/lib
	install -m 644 $(LIB_AR) $(PREFIX)/lib/$(LIB_AR)

# ---------------------------------------------------------
# uninstall
# ---------------------------------------------------------

uninstall:
	rm -f $(PREFIX)/lib/$(LIB_SO)
	rm -f $(PREFIX)/lib/$(LIB_AR)
	rm -rf $(PREFIX)/include/$(LIB)
	rm -f $(PC_PREFIX)/$(LIB_PC)

# ---------------------------------------------------------
# clean
# ---------------------------------------------------------

clean:
	rm -rf build
	rm -f $(LIB_SO)
	rm -f $(LIB_AR)
	rm -f demo
	rm -f $(LIB_PC)
	rm -f compile_commands.json
	rm -f gdb.txt

-include $(C_DEP)
