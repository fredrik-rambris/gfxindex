all: gfxindex

install: gfxindex
	install gfxindex /usr/bin

uninstall:
	rm -f /usr/bin/gfxindex

gfxindex: gfxindex.c config.h
	cc -o gfxindex gfxindex.c `imlib-config --libs-gdk --cflags-gdk` -lpopt

clean:
	rm -f *.o *~ gfxindex core
