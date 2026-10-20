# Changes from upstream

This is the Asciiverse edition of cgames, Brian Raiter's three games for
the terminal (C, 2000 to 2020): cmines (minesweeper), csokoban (sokoban)
and cblocks (sliding-block puzzles). They were written for the Linux
console, with a curses version of each beside it; this edition builds
the curses versions. Each game is its own title in the catalog, in the
collection "cgames".

## History

cgames has no public repository, so this one is built from every release
on Brian Raiter's site (muppetlabs.com/~breadbox/pub/software/), each
imported unchanged as one commit in his name, dated by its tarball and
tagged:

| tag | sha256 of the tarball |
|---|---|
| `1.0` (2000-09-02, csokoban only) | `0acbe988bb7ebd3d7f2b2238956baa5e02d984ef9c31fc10e9eabce0e0d0db7c3` |
| `2.0` (2000-09-10) | `aa7d760bbb72efb915d5f8a8ca82a2baf1f0aa93598792f00813237af21d32ea` |
| `2.1` (2000-09-18) | `255a2292cb1c9bc134c8931a5e24b1f66d249f3f512672a57eed8a7a86e8a5e4` |
| `2.2-ibiblio` (2000-11-12, the first upload of 2.2, from ibiblio) | `0e1c3ec332077613283ea2c3fdba5b4776c07f73c2749e7659a5980024ab4640` |
| `2.2` (2000-11-12, re-rolled that afternoon: the clean rules, the Changelog credit to Ben Olmstead, Conway's puzzle moved last) | `8641f915f6333914b113df12cd97aff507c66559ae905fb6b337270b0206ed8e` |
| `2.2a` (2013-04-21) | `3629b1a6c0a99dfb00c7387687b27ee9868b072cfddcae3944f997ce78b0947c` |
| `2.2b` (2020-01-28, the latest) | `6ace6157198077ffb544a6e9bbf63a5823a09e408055c02045630cc35841f474` |

No distribution packages cgames (not in Debian), so there are no
distribution patches to bring in. The `asciiverse` branch carries the
changes below.

## Licence

GPL-2.0-or-later ("version 2 (or, at your option, any later version)",
README); our changes are under the same terms. csokoban's levels are
David W. Skinner's (below).

## Levels

csokoban shipped four level sets: the 50 original levels (Thinking
Rabbit, Hiroyuki Imabayashi, 1982; the rights are now Falcon's), the
Xsokoban extras, J. Franklin Mentzer's "Still More!" and Yoshio Murase's
sets. No licence that allows passing them on was found for any of them,
so they are left out (they stay in the history, in the release tags).

In their place are David W. Skinner's fifteen finished sets, 1,043
levels: Microban, Mas Microban, Microban III and IV, and Sasquatch I to
XI. His page said in 2001 that his sets "may be freely distributed
provided they remain unchanged and credited with my name and email
address", and from 2008 "provided they remain properly credited". The
files are his last versions as kept by the Internet Archive (2012), with
Unix line endings and a header naming the set and crediting him by name
and address; the sha256 of each file as downloaded is in the commit that
adds them. His unfinished sets (Microban V, Sasquatch XII) are left out.
The archive carries LEVELS.txt with the credits.

cblocks' twelve Classic puzzles (Dudeney, Loyd, Conway and others) are
Brian Raiter's own file, unchanged.

## Code

- **The colour pair is read into a short.** attr_get() stores the pair
  through a short pointer, but cmines and cblocks passed an int's
  address, leaving half of it as it was (GCC 14 refuses to compile it).
- **cmines' mine shuffle stays within the cards still to shuffle.** It
  picked rand() * (n + 1) / RAND_MAX, which is n + 1 when rand() returns
  RAND_MAX: a slot that can hold an entry from an earlier game.
- **Files** (under `AV_PATHS`, asciiverse/paths.c, so other builds are
  unchanged): csokoban and cblocks read their levels from `series/`
  beside the program; saved positions and solutions go in
  `$XDG_DATA_HOME/<game>`, else `~/.local/share/<game>` (`%APPDATA%` on
  Windows), unless `~/.csokoban` or `~/.cblocks` already exists; cmines'
  setups and best times go in `cminesrc` there, unless `~/.cminesrc`
  exists. The environment variables and options of the originals still
  override.
- **Centred, and following a resized terminal.** Each game's curses
  layer places the level and its sidebar (cmines: the field, its counter
  and timer, the timer now beside the counter instead of in the screen's
  corner) at every drawing, centred together, and the help centred too.
  On a resize the layer draws again what it last showed, so the game code
  is unchanged. Mouse clicks are taken relative to the field; in cmines a
  click outside the field is ignored (far to the right of a narrow field
  it gave a column past the row's width, which landed on a cell of the
  next row).
- **csokoban: levels up to 64x64** (32x32 with the border before; many
  of Skinner's are larger, up to 49x44). Positions are offsets in a
  short and moves are 14 bits, ample at 64x64, and saved solutions are
  move letters, so existing saves read as before.
- **csokoban: a level too big for the terminal** said "won't fit" and
  went straight on to the end-of-level prompt, reading a variable that
  was never set on that path. Now the screen says how much room the
  level needs and how much there is, and the game waits: a bigger
  terminal shows the level, N or Return goes to the next level, P to
  the previous one, q quits.
- **csokoban hides the terminal's cursor**, which stood at the bottom of
  the sidebar.
- **An empty move list is copied without memcpy() from a null
  pointer** (csokoban and cblocks; found by UBSan).
- **Windows** (MinGW and PDCurses): `<curses.h>` and a one-argument
  mkdir() under `_WIN32`; paths.c finds the program's folder with
  GetModuleFileName and the player's under `%APPDATA%`.

## Packaging

- `asciiverse/Makefile` builds the three into `out/` without configure;
  `asciiverse/package.sh` packs one archive per game:
  `<game>-<target>` with the program, `series/` (csokoban, cblocks),
  COPYING, PATCHES.md, the game's README (and cgames' as
  README-cgames), its man page, and for csokoban LEVELS.txt.
- `.github/workflows/release.yml`: static musl Linux (Alpine, x86-64 and
  ARM64), macOS (Intel and Apple silicon, system ncurses) and Windows
  x86-64 (MinGW, PDCurses 3.9 wincon, wide and UTF-8), on our tags
  (`*-av.*`).
- `asciiverse/record/` records the previews: `record.sh` runs a game
  under asciinema in a 100x30 tmux pane while its driver plays, then
  stops it so the cast ends on the game. csokoban.py solves Microban's
  levels by breadth-first search; cmines.py plays as a careful player
  (the take kept ends on a won Intermediate game, a best time of 46 s,
  as the game itself recorded);
  cblocks.py solves the Classic puzzles and selects blocks with the
  arrow keys.

## Checked

Linux, native and static (Alpine): all three played; every one of the
1,043 levels loads in csokoban; levels found beside the program, player
files in the data folder; resizing in play and in the help; the
too-big-level wait. AddressSanitizer and UBSan clean over random-key,
mouse and resize soaks of four minutes per game after the fixes (about
280 restarts in all). Windows: cross-built and run under Wine (levels
found beside the program, cmines' setups saved in `%APPDATA%`); Wine's
console shows the line drawing as question marks with either PDCurses
build, so the drawing on real Windows is unchecked. macOS: not built
here.
