# Makefile for texgenpack.

# Target directory when installing.
INSTALL_DIR = /usr/bin

CFLAGS = -std=gnu99 -Ofast
LFLAGS = -O
# CFLAGS = -std=gnu99 -ggdb
# LFLAGS = -ggdb
PKG_CONFIG_CFLAGS = `pkg-config --cflags gtk+-3.0`
PKG_CONFIG_LFLAGS = `pkg-config --libs gtk+-3.0`
# For MinGW with GTK installed, uncomment the following line.
#PNG_LIB_LOCATION = `pkg-config --libs gtk+-3.0`
SHARED_MODULE_OBJECTS = image.o compress.o mipmap.o file.o texture.o etc2.o dxtc.o astc.o bptc.o half_float.o \
	compare.o rgtc.o
TEXGENPACK_MODULE_OBJECTS = texgenpack.o calibrate.o
TEXVIEW_MODULE_OBJECTS = viewer.o gtk.o

all : texgenpack texgenpack-gui

texgenpack : $(TEXGENPACK_MODULE_OBJECTS) $(SHARED_MODULE_OBJECTS)
	$(CC) $(LFLAGS) $(TEXGENPACK_MODULE_OBJECTS) $(SHARED_MODULE_OBJECTS) -o texgenpack -lm -lpng -lfgen -lpthread $(PNG_LIB_LOCATION)

texgenpack-gui : $(TEXVIEW_MODULE_OBJECTS) $(SHARED_MODULE_OBJECTS)
	$(CC) $(LFLAGS) $(TEXVIEW_MODULE_OBJECTS) $(SHARED_MODULE_OBJECTS) -o texgenpack-gui -lm -lpng -lfgen -lpthread $(PKG_CONFIG_LFLAGS)

install : texgenpack texgenpack-gui
	install -m 0755 texgenpack $(INSTALL_DIR)/texgenpack
	install -m 0755 texgenpack-gui $(INSTALL_DIR)/texgenpack-gui

clean :
	rm -f $(TEXGENPACK_MODULE_OBJECTS) $(TEXVIEW_MODULE_OBJECTS) $(SHARED_MODULE_OBJECTS)
	rm -f texgenpack
	rm -f texgenpack-gui

gtk.o : gtk.c
	$(CC) -c $(CFLAGS) $(PKG_CONFIG_CFLAGS) gtk.c -o gtk.o

file.o : file.c
	$(CC) -c $(CFLAGS) $(PKG_CONFIG_CFLAGS) file.c -o file.o

.c.o :
	$(CC) -c $(CFLAGS) $< -o $@

.c.s :
	$(CC) -S $(CFLAGS) $< -o $@

dep:
	rm -f .depend
	make .depend

.depend:
	echo '# Module dependencies' >>.depend
	$(CC) -MM $(patsubst %.o,%.c,$(TEXGENPACK_MODULE_OBJECTS)) >>.depend
	$(CC) -MM $(patsubst %.o,%.c,$(TEXVIEW_MODULE_OBJECTS)) >>.depend
	$(CC) -MM $(patsubst %.o,%.c,$(SHARED_MODULE_OBJECTS)) >>.depend

include .depend

