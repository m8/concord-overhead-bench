TARGET = VOLREND
OBJS = adaptive.o file.o main.o map.o normal.o octree.o opacity.o option.o raytrace.o render.o view.o

include ../../Makefile.config

CFLAGS := $(CFLAGS) -I./libtiff
LDFLAGS := -L./libtiff $(LDFLAGS) -ltiff -lpng -ljpeg -lz -lm -ljbig -llzma

main.c:	main.C incl.h anl.h
file.c:	file.C incl.h
option.c: option.C incl.h
map.c: map.C incl.h
normal.c: normal.C incl.h anl.h address.h
opacity.c: opacity.C incl.h anl.h
octree.c: octree.C incl.h anl.h
view.c:	view.C incl.h
render.c: render.C incl.h
adaptive.c: adaptive.C incl.h anl.h
raytrace.c: raytrace.C incl.h address.h

incl.h:	user_options.h const.h my_types.h global.h macros.h address.h
