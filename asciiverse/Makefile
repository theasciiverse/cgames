# Builds the three cgames with curses, without configure:
#
#   make -f asciiverse/Makefile             # out/cmines, out/csokoban, out/cblocks
#
#   CC, CFLAGS, LDFLAGS as usual; CURSES_LIBS (default -lncurses)
#   EXE=.exe for Windows; BUILD, the folder for objects (default build)
#
# Each game uses its curses interface (cursesio.c, which configure
# --with-ncurses links as userio.c), the curses mouse, and finds its
# files through asciiverse/paths.c (AV_PATHS): levels in series/ beside
# the program, the player's files in their data folder.

CC ?= cc
CFLAGS ?= -O2
CURSES_LIBS ?= -lncurses
EXE ?=
BUILD ?= build
DEFS = -DAV_PATHS -Iasciiverse

GAMES = cmines csokoban cblocks
cmines_SRCS = cmines/cmines.c cmines/cursesio.c
csokoban_SRCS = csokoban/csokoban.c csokoban/answers.c csokoban/dirio.c \
	csokoban/fileread.c csokoban/movelist.c csokoban/play.c \
	csokoban/cursesio.c
cblocks_SRCS = cblocks/cblocks.c cblocks/answers.c cblocks/dirio.c \
	cblocks/fileread.c cblocks/movelist.c cblocks/parse.c cblocks/play.c \
	cblocks/cursesio.c

all: $(GAMES:%=out/%$(EXE))

define game
$(1)_OBJS = $$($(1)_SRCS:%.c=$(BUILD)/%.o) $(BUILD)/$(1)/paths.o
out/$(1)$(EXE): $$($(1)_OBJS)
	@mkdir -p out
	$$(CC) $$(LDFLAGS) -o $$@ $$($(1)_OBJS) $$(CURSES_LIBS)
$(BUILD)/$(1)/paths.o: asciiverse/paths.c asciiverse/paths.h
	@mkdir -p $$(dir $$@)
	$$(CC) $$(CFLAGS) $$(CPPFLAGS) $$(DEFS) -MMD -MP -c -o $$@ $$<
endef
$(foreach g,$(GAMES),$(eval $(call game,$(g))))

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEFS) -MMD -MP -c -o $@ $<

-include $(shell find $(BUILD) -name '*.d' 2>/dev/null)

clean:
	rm -rf $(BUILD) out
