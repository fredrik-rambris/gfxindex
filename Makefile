gfxindex : gfxindex.o
	cc -o gfxindex gfxindex.o

gfxindex.o : gfxindex.c
	cc -c gfxindex.c 

install:
	install gfxindex /usr/bin
	install makethumbs /usr/bin
	install anytopnm /usr/bin