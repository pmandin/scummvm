MODULE := engines/reevengi

MODULE_OBJS := \
	detection.o \
	metaengine.o \
	reevengi.o \
	formats/adt.o \
	formats/ard.o \
	formats/bss.o \
	formats/bss_sld.o \
	formats/pak.o \
	formats/rofs.o \
	formats/sld.o \
	formats/tim.o \
	game/clock.o \
	game/room.o \
	gfx/gfx_base.o \
	gfx/gfx_opengl.o \
	gfx/gfx_tinygl.o \
	movie/avi.o \
	movie/movie.o \
	movie/mpeg.o \
	movie/psx.o \
	re1/re1.o \
	re1/room.o \
	re2/re2.o \
	re2/room.o \
	re3/re3.o \
	re3/room.o

# This module can be built as a plugin
ifeq ($(ENABLE_REEVENGI), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk
