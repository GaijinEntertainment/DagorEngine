
# Do not edit normally. Configuration settings are in Makefile.conf.

TARGET_MACHINE := $(shell gcc -dumpmachine)
include Makefile.conf

SHORT_LIBRARY_NAME = detex
LIBRARY_NAME = lib$(SHORT_LIBRARY_NAME)
VERSION = 0.1.2
SO_VERSION = 0.1
MAJOR_VERSION = 0

ifeq ($(LIBRARY_CONFIGURATION), DEBUG)
OPTCFLAGS += -ggdb
else
OPTCFLAGS += -Ofast -ffast-math
endif
# Use the 1999 ISO C standard with POSIX.1-2008 definitions.
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wno-maybe-uninitialized -pipe -I. $(OPTCFLAGS)

ifeq ($(LIBRARY_CONFIGURATION), SHARED)
# Shared library.
LIBRARY_OBJECT = $(LIBRARY_NAME).so.$(VERSION)
INSTALL_TARGET = install_shared
LIBRARY_DEPENDENCY =
TEST_PROGRAM_LFLAGS = -l$(SHORT_LIBRARY_NAME)
CFLAGS_LIB = $(CFLAGS) -fPIC -fvisibility=hidden -DDST_SHARED -DDST_SHARED_EXPORTS
CFLAGS_TEST = $(CFLAGS)
else
# Static or static debug version.
ifeq ($(LIBRARY_CONFIGURATION), DEBUG)
LIBRARY_OBJECT = $(LIBRARY_NAME)_dbg.a
else
LIBRARY_OBJECT = $(LIBRARY_NAME).a
endif
# install_static also works for debugging library
INSTALL_TARGET = install_static
LIBRARY_DEPENDENCY = $(LIBRARY_OBJECT)
TEST_PROGRAM_LFLAGS = $(LIBRARY_OBJECT)
CFLAGS_LIB = $(CFLAGS)
CFLAGS_TEST = $(CFLAGS)
endif
CFLAGS_TEST += -DDETEX_VERSION=\"v$(VERSION)\"
LIBRARY_LIBS = -lm

LIBRARY_MODULE_OBJECTS = bptc-tables.o bits.o clamp.o convert.o dds.o decompress-bc.o decompress-bptc.o \
	decompress-bptc-float.o decompress-etc.o decompress-eac.o decompress-rgtc.o division-tables.o \
	file-info.o half-float.o hdr.o ktx.o misc.o raw.o texture.o
LIBRARY_HEADER_FILES = detex.h
TEST_PROGRAMS = detex-validate detex-view detex-convert

default : library

library : $(LIBRARY_OBJECT)

programs : $(TEST_PROGRAMS)

$(LIBRARY_NAME).so.$(VERSION) : $(LIBRARY_MODULE_OBJECTS) $(LIBRARY_HEADER_FILES)
	g++ -shared -Wl,-soname,$(LIBRARY_NAME).so.$(SO_VERSION) -fPIC -o $(LIBRARY_OBJECT) \
$(LIBRARY_MODULE_OBJECTS) $(LIBRARY_LIBS)
	@echo Run '(sudo) make install to install.'

$(LIBRARY_NAME).a : $(LIBRARY_MODULE_OBJECTS) $(LIBRARY_HEADER_FILES)
	ar r $(LIBRARY_OBJECT) $(LIBRARY_MODULE_OBJECTS)
	@echo 'Run (sudo) make install to install, or make programs to build the test programs.'

$(LIBRARY_NAME)_dbg.a : $(LIBRARY_MODULE_OBJECTS) $(LIBRARY_HEADER_FILES)
	ar r $(LIBRARY_OBJECT) $(LIBRARY_MODULE_OBJECTS)
	@echo 'The library is compiled with debugging enabled.'

install : $(INSTALL_TARGET) install_headers

install_headers : $(LIBRARY_HEADER_FILES)
	@for x in $(LIBRARY_HEADER_FILES); do \
	echo Installing $(HEADER_FILE_INSTALL_DIR)/$$x.; \
	install -m 0644 $$x $(HEADER_FILE_INSTALL_DIR)/$$x; done

install_shared : $(LIBRARY_OBJECT)
	install -m 0644 $(LIBRARY_OBJECT) $(SHARED_LIB_DIR)/$(LIBRARY_OBJECT)
	ln -sf $(SHARED_LIB_DIR)/$(LIBRARY_OBJECT) $(SHARED_LIB_DIR)/$(LIBRARY_NAME).so

install_static : $(LIBRARY_OBJECT)
	install -m 0644 $(LIBRARY_OBJECT) $(STATIC_LIB_DIR)/$(LIBRARY_OBJECT)

install-programs : detex-view detex-convert
	install -m 0755 detex-view $(PROGRAM_INSTALL_DIR)/detex-view
	install -m 0755 detex-convert $(PROGRAM_INSTALL_DIR)/detex-convert

detex-validate : validate.o $(LIBRARY_OBJECT)
	gcc validate.o -o detex-validate $(LIBRARY_OBJECT) $(LIBRARY_LIBS) `pkg-config --libs gtk+-3.0`

detex-view : detex-view.o $(LIBRARY_OBJECT)
	gcc detex-view.o -o detex-view $(LIBRARY_OBJECT) $(LIBRARY_LIBS) `pkg-config --libs gtk+-3.0`

detex-convert : detex-convert.o png.o $(LIBRARY_OBJECT)
	gcc detex-convert.o png.o -o detex-convert $(LIBRARY_OBJECT) $(LIBRARY_LIBS) `pkg-config --libs libpng`

clean :
	rm -f $(LIBRARY_MODULE_OBJECTS)
	rm -f $(TEST_PROGRAMS)
	rm -f validate.o
	rm -f detex-view.o
	rm -f detex-convert.o
	rm -f png.o
	rm -f $(LIBRARY_NAME).so.$(VERSION)
	rm -f $(LIBRARY_NAME).a
	rm -f $(LIBRARY_NAME)_dbg.a

.c.o :
	gcc -c $(CFLAGS_LIB) $< -o $@

validate.o : validate.c
	gcc -c $(CFLAGS_TEST) $< -o $@ `pkg-config --cflags --libs gtk+-3.0`

detex-view.o : detex-view.c
	gcc -c $(CFLAGS_TEST) $< -o $@ `pkg-config --cflags --libs gtk+-3.0`

detex-convert.o : detex-convert.c
	gcc -c $(CFLAGS_TEST) $< -o $@

png.o : png.c
	gcc -c $(CFLAGS_TEST) $< -o $@

dep :
	rm -f .depend
	make .depend

.depend: Makefile.conf Makefile
	gcc -MM $(CFLAGS_LIB) $(patsubst %.o,%.c,$(LIBRARY_MODULE_OBJECTS)) > .depend
        # Make sure Makefile.conf and Makefile are dependency for all modules.
	for x in $(LIBRARY_MODULE_OBJECTS); do \
	echo $$x : Makefile.conf Makefile >> .depend; done
	gcc -MM $(CFLAGS_TEST) validate.c >> .depend
	gcc -MM $(CFLAGS_TEST) detex-view.c >> .depend
	gcc -MM $(CFLAGS_TEST) detex-convert.c png.c >> .depend

include .depend

