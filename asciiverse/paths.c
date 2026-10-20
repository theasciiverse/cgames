/* paths.c: where the Asciiverse builds of the cgames keep their files
 * (compiled in with -DAV_PATHS; see PATCHES.md).
 *
 * Copyright (C) 2026 Asciiverse, under the GNU General Public License,
 * version 2 or later, as the games. No warranty.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#ifdef _WIN32
#include	<windows.h>
#elif defined(__APPLE__)
#include	<stdint.h>
#include	<mach-o/dyld.h>
#else
#include	<unistd.h>
#endif
#include	"paths.h"

#ifdef _WIN32
#define	mkdir(path, mode)	mkdir(path)
#endif

/* Copy the folder holding the running program into buf (of size n).
 * Return 0 if it cannot be found.
 */
int av_exedir(char *buf, size_t n)
{
    char       *p;
    long	len;

#ifdef _WIN32
    len = GetModuleFileNameA(NULL, buf, (DWORD)n);
    if (len <= 0 || (size_t)len >= n)
	return 0;
    for (p = buf ; *p ; ++p)
	if (*p == '\\')
	    *p = '/';
#elif defined(__APPLE__)
    uint32_t	size = (uint32_t)n;

    if (_NSGetExecutablePath(buf, &size) != 0)
	return 0;
    len = strlen(buf);
#else
    len = readlink("/proc/self/exe", buf, n - 1);
    if (len <= 0)
	return 0;
    buf[len] = '\0';
#endif
    if (!(p = strrchr(buf, '/')))
	return 0;
    *p = '\0';
    return 1;
}

/* Copy the folder named sub beside the running program into buf, if
 * there is one. Return 0 otherwise (buf unchanged).
 */
int av_besideexe(char *buf, size_t n, char const *sub)
{
    char	dir[4096];
    struct stat	st;

    if (!av_exedir(dir, sizeof dir) || strlen(dir) + 1 + strlen(sub) >= n)
	return 0;
    sprintf(dir + strlen(dir), "/%s", sub);
    if (stat(dir, &st) || !S_ISDIR(st.st_mode))
	return 0;
    strcpy(buf, dir);
    return 1;
}

/* Make the folder path and any missing parents, as "mkdir -p" does.
 */
static void makedirs(char *path)
{
    char       *p;

    for (p = path + 1 ; *p ; ++p) {
	if (*p == '/' || *p == '\\') {
	    char c = *p;
	    *p = '\0';
	    mkdir(path, 0755);
	    *p = c;
	}
    }
    mkdir(path, 0755);
}

/* Copy the player's folder for the game into buf (of size n), made if
 * missing: $XDG_DATA_HOME/game, else ~/.local/share/game (on Windows
 * %APPDATA%\game). Empty variables count as unset. If legacy names a
 * folder in the home folder that already exists (a player's files from
 * before), that is used instead. Return 0 if there is no
 * home folder to go by.
 */
int av_playerdir(char *buf, size_t n, char const *game, char const *legacy)
{
    char const *home, *base;
    struct stat	st;

#ifdef _WIN32
    home = getenv("USERPROFILE");
    base = getenv("APPDATA");
#else
    home = getenv("HOME");
    base = getenv("XDG_DATA_HOME");
    if (base && *base != '/')
	base = NULL;
#endif
    if (home && !*home)
	home = NULL;
    if (base && !*base)
	base = NULL;

    if (legacy && home
	       && strlen(home) + 1 + strlen(legacy) < n) {
	sprintf(buf, "%s/%s", home, legacy);
	if (!stat(buf, &st) && S_ISDIR(st.st_mode))
	    return 1;
    }

    if (base) {
	if (strlen(base) + 1 + strlen(game) >= n)
	    return 0;
	sprintf(buf, "%s/%s", base, game);
    } else if (home) {
	if (strlen(home) + 14 + strlen(game) >= n)
	    return 0;
	sprintf(buf, "%s/.local/share/%s", home, game);
    } else
	return 0;
    makedirs(buf);
    return 1;
}
