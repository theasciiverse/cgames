/* paths.h: where the Asciiverse builds of the cgames keep their files.
 *
 * Copyright (C) 2026 Asciiverse, under the GNU General Public License,
 * version 2 or later, as the games. No warranty.
 */

#ifndef	_paths_h_
#define	_paths_h_

#include	<stddef.h>

extern int av_exedir(char *buf, size_t n);
extern int av_besideexe(char *buf, size_t n, char const *sub);
extern int av_playerdir(char *buf, size_t n, char const *game,
			char const *legacy);

#endif
