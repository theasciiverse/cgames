#define	__POSIX_SOURCE
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	<string.h>
#include	<time.h>
#include	<signal.h>
#include	<ctype.h>
#include	<errno.h>
#include	<unistd.h>
#include	<ncurses.h>
#include	"cmines.h"
#include	"userio.h"


/* The attributes used to display all the different kinds of cells.
 */
static chtype		coveredcell, minecell, boomcell, flagcell, badflagcell;
static chtype		numbercell[9];

/* The emoticons used to indicate the current status.
 */
static char const *smiley[] = { "   ", ":-)", "8-|", ":-D", "B-)" };

/* starttime either is zero if the timer is not running, is negative
 * if the timer has been turned off, or else holds the time at which
 * the timer began running.
 */
static time_t		starttime = -1;

/* The timer's current value.
 */
static int		timer = 0;

/* The index of the bottommost line.
 */
static int		lastline;

/* The index of the column where the timer is displayed.
 */
static int		timercolumn;

/* TRUE if the timer should be updated asynchronously.
 */
static int		updatetimer = FALSE;

/* TRUE if a mouse click to the right of a cell is accepted.
 */
static int		allowoffclicks = FALSE;

/* TRUE if the current status should be indicated with emoticons.
 */
static int		showsmileys = FALSE;

/* FALSE if the program is allowed to ring the bell.
 */
static int		silence = FALSE;

/* The name of the program (for use in error messages).
 */
char const	       *programname = "";

/* The screen row and column of the field's top left corner (its
 * frame), worked out at each drawing so that the field and its counter
 * and timer are centred together.
 */
static int		oy, ox;

/* The size of the field last drawn, to ignore clicks outside it.
 */
static int		fieldrows, fieldcols;

/* TRUE while the help is on the screen, with what it shows, to draw it
 * again when the terminal is resized.
 */
static int		showinghelp = FALSE;
static int		lastkeycount, lastsetupcount;
static char const     **lastkeys, **lastsetups;
static int	       *lastbesttimes;

/*
 * Timer functions
 */

/* Return the current value of the timer.
 */
int gettimer(void)
{
    return timer;
}

/* Set the timer's current state. If action is positive, the timer
 * begins counting. If action is zero, the timer stops counting. If
 * action is negative, the timer is turned off and removed from the
 * display.
 */
void settimer(int action)
{
    if (action < 0) {
	timer = 0;
	starttime = -1;
    } else if (action > 0)
	starttime = time(NULL) - timer;
    else
	starttime = 0;
}

/*
 * Output functions
 */

/* Retrieve the current screen size and compute the size and placement
 * of the various parts. FALSE is returned if the screen is too small
 * to display anything.
 */
static int measurescreen(void)
{
    int	y, x;

    getmaxyx(stdscr, y, x);

    lastline = y - 1;
    timercolumn = x - 7;
    if (timercolumn < 1)
	return FALSE;
    return TRUE;
}

/* Update the timer display without altering the cursor position.
 * Output is flushed whether or not the timer is currently being
 * displayed.
 */
static void displaytimer(void)
{
    int	y, x;

    if (starttime > 0)
	timer = time(NULL) - starttime;
    if (timer > 0 && !showinghelp) {
	getyx(stdscr, y, x);
	mvprintw(oy, timercolumn, "%7d", timer);
	move(y, x);
    }
    refresh();
}

/* Display the game state on the console. The field is placed in the
 * upper left corner, and status information appears in the upper
 * right corner.
 */
void displaygame(cell const *field, int ysize, int xsize,
		 int minecount, int flagcount, int status)
{
    cell const *p;
    chtype	ch;
    int		y, x;

    int		w;

    showinghelp = FALSE;
    erase();
    measurescreen();

    /* the field's frame, then the counter, emoticon and timer */
    w = xsize * 2 + 3 + 5 + (showsmileys ? 3 : 0) + 1 + 7;
    if (ysize + 2 > lastline + 1 || w > COLS) {
	oy = ox = 0;
	mvaddstr(0, 0, "The screen is too small to show the game.");
	refresh();
	return;
    }
    oy = (lastline + 1 - (ysize + 2)) / 2;
    ox = (COLS - w) / 2;
    fieldrows = ysize;
    fieldcols = xsize;
    timercolumn = ox + w - 7;

    mvaddch(oy + 0, ox + 0, ACS_ULCORNER);
    mvaddch(oy + 0, ox + 1, ACS_HLINE);
    mvaddch(oy + 0, ox + xsize * 2 + 2, ACS_URCORNER);
    mvaddch(oy + ysize + 1, ox + 0, ACS_LLCORNER);
    mvaddch(oy + ysize + 1, ox + 1, ACS_HLINE);
    mvaddch(oy + ysize + 1, ox + xsize * 2 + 2, ACS_LRCORNER);
    for (x = 0 ; x < xsize ; ++x) {
	mvaddch(oy + 0, ox + x * 2 + 2, ACS_HLINE);
	mvaddch(oy + 0, ox + x * 2 + 3, ACS_HLINE);
	mvaddch(oy + ysize + 1, ox + x * 2 + 2, ACS_HLINE);
	mvaddch(oy + ysize + 1, ox + x * 2 + 3, ACS_HLINE);
    }
    for (y = 0, p = field ; y < ysize ; ++y, p += XSIZE) {
	mvaddch(oy + y + 1, ox + 0, ACS_VLINE);
	mvaddch(oy + y + 1, ox + xsize * 2 + 2, ACS_VLINE);
	for (x = 0 ; x < xsize ; ++x) {
	    if (p[x] & MINED) {
		if (p[x] & FLAGGED)
		    ch = flagcell;
		else if (p[x] & EXPOSED)
		    ch = boomcell;
		else
		    ch = status == status_normal ? coveredcell : minecell;
	    } else if (p[x] & FLAGGED) {
		ch = status == status_normal ? flagcell : badflagcell;
	    } else if (p[x] & EXPOSED) {
		ch = p[x] & NEIGHBOR_MASK;
		ch = numbercell[ch];
	    } else
		ch = coveredcell;
	    mvaddch(oy + y + 1, ox + x * 2 + 2, ch);
	}
    }

    mvprintw(oy, ox + xsize * 2 + 4, "%-4d", minecount - flagcount);
    if (showsmileys && status != status_ignore)
	addstr(smiley[status]);

    move(oy + ysize + 1, ox + xsize * 2 + 4);
    displaytimer();
}

/* Display information about the various key commands. Each element in
 * the keys array contains two sequential NUL-terminated strings,
 * describing the key and the associated command respectively. The
 * strings in the setups array describe the different configurations
 * available. The besttimes array give the best times on record for
 * each configuration. The information is arranged into columns.
 */
void displayhelp(int keycount, char const *keys[],
		 int setupcount, char const *setups[], int besttimes[])
{
    int keywidth, descwidth;
    int	i, n;

    keywidth = 1;
    descwidth = 0;
    for (i = 0 ; i < setupcount ; ++i) {
	n = strlen(setups[i]);
	if (n > descwidth)
	    descwidth = n;
    }
    for (i = 0 ; i < keycount ; ++i) {
	n = strlen(keys[i]);
	if (n > keywidth)
	    keywidth = n;
	n = strlen(keys[i] + n + 1);
	if (n > descwidth)
	    descwidth = n;
    }

    showinghelp = TRUE;
    lastkeycount = keycount;
    lastkeys = keys;
    lastsetupcount = setupcount;
    lastsetups = setups;
    lastbesttimes = besttimes;

    erase();
    measurescreen();
    /* the help, centred */
    oy = (lastline + 1 - (keycount + setupcount + 3)) / 2;
    ox = (COLS - (keywidth + descwidth + 16)) / 2;
    if (oy < 0)
	oy = 0;
    if (ox < 0)
	ox = 0;
    for (i = 0 ; i < keycount ; ++i) {
	n = strlen(keys[i]);
	mvprintw(oy + i, ox, "%-*s  %-*s", keywidth, keys[i],
				  descwidth, keys[i] + n + 1);
	if (i == keycount - 1)
	    addstr("    Best Times");
    }
    move(oy + i, ox + keywidth + descwidth + 6);
    for (i = 0 ; i < 10 ; ++i)
	addch(ACS_HLINE);
    for (i = 0 ; i < setupcount ; ++i) {
	n = strlen(setups[i]);
	mvprintw(oy + keycount + 1 + i, ox, "%-*c  %-*s", keywidth,
		 tolower(setups[i][0]), descwidth, setups[i]);
	if (besttimes[i])
	    printw("%14d", besttimes[i]);
    }
    mvaddstr(oy + keycount + setupcount + 2, ox, "Press any key to return.");
    refresh();
}

/* Move the cursor to the given field cell.
 */
void setcursorpos(int pos)
{
    int	y, x;

    if (pos >= 0) {
	y = pos / XSIZE;
	x = pos - y * XSIZE;
	y = oy + y + 1;
	x = ox + x * 2 + 2;
	if (y <= lastline && x <= timercolumn) {
	    move(y, x);
	    refresh();
	}
    }
}

/*
 * Input functions
 */

/* Handle mouse activity. The screen coordinates are translated into a
 * field's cell position and mousecallback() is called. If the left
 * and right mouse buttons are both depressed, it is automatically
 * translated into a middle button event. The return value supplies
 * the keystroke to translate the mouse event into, or zero if no
 * keystroke should be generated. Because the program is only
 * monitoring mouse clicks, this function simulates separate pressing
 * and releasing events.
 */
static int mousehandler(void)
{
    MEVENT	event;
    int		mstate;
    int		key, pos;

    if (getmouse(&event) != OK)
	return 0;
    if (event.y < oy + 1 || event.x < ox + 2
			  || event.y > oy + fieldrows
			  || (event.x - ox - 2) / 2 >= fieldcols)
	return 0;
    if (!allowoffclicks && ((event.x - ox) & 1))
	return 0;

    switch (event.bstate) {
      case BUTTON1_CLICKED:
	mstate = 1;
	break;
      case BUTTON3_CLICKED:
      case BUTTON2_CLICKED | BUTTON3_CLICKED:
	mstate = 2;
	break;
      case BUTTON2_CLICKED:
      case BUTTON1_CLICKED | BUTTON3_CLICKED:
      case BUTTON1_CLICKED | BUTTON2_CLICKED | BUTTON3_CLICKED:
	mstate = 3;
	break;
      default:
	return 0;
    }

    pos = (event.y - oy - 1) * XSIZE + (event.x - ox - 2) / 2;
    key = mousecallback(pos, -mstate);
    if (!key || key == '\f')
	key = mousecallback(pos, mstate);
    else
	mousecallback(pos, mstate);

    return key;
}

/* Retrieve and return a single keystroke. Arrow keys and other inputs
 * are translated into ASCII equivalents. The timer is updated every
 * second.
 */
int input(void)
{
    int	key;

    for (;;) {
	if (starttime > 0 && updatetimer)
	    displaytimer();
	key = getch();
	switch (key) {
	  case KEY_UP:			return 'k';
	  case KEY_RIGHT:		return 'l';
	  case KEY_DOWN:		return 'j';
	  case KEY_LEFT:		return 'h';
	  case KEY_ENTER:		return '\n';
	  case '\r':			return '\n';
	  case '\f':
	    clearok(stdscr, TRUE);
	    return key;
#ifdef KEY_RESIZE
	  case KEY_RESIZE:
	    /* the game draws itself again on Ctrl-L; the help is drawn
	     * again here, to stay on the screen */
#ifdef PDCURSES
	    resize_term(0, 0);
#endif
	    if (!showinghelp)
		return '\f';
	    displayhelp(lastkeycount, lastkeys, lastsetupcount,
			lastsetups, lastbesttimes);
	    break;
#endif
	  case KEY_MOUSE:
	    if ((key = mousehandler()))
		return key;
	    break;
	  case ERR:
	    break;
	  default:
	    return key;
	}
    }
}

/*
 * Top-level functions
 */

/* Reset the terminal to its natural state.
 */
static void shutdown(void)
{
    if (!isendwin())
	endwin();
}

/* Initialize the terminal. The terminal's capabilities (such as color
 * and highlighting) are examined to decide how the elements of the
 * game should be displayed.
 */
int ioinitialize(int updatetimerflag, int showsmileysflag, int silenceflag,
		 int allowoffclicksflag)
{
    attr_t	attr;
    int		n;

    updatetimer = updatetimerflag;
    showsmileys = showsmileysflag;
    silence = silenceflag;
    allowoffclicks = allowoffclicksflag;

    atexit(shutdown);

    if (!initscr())
	return FALSE;

    measurescreen();

    nonl();
    halfdelay(10);
    noecho();
    keypad(stdscr, TRUE);

#ifndef NOMOUSE
    mousemask(BUTTON1_CLICKED | BUTTON2_CLICKED | BUTTON3_CLICKED, NULL);
#endif

    if (has_colors()) {
	short	pair, bkgnd;
	short	numbercolors[] = { 0, COLOR_BLUE, COLOR_GREEN, COLOR_RED,
				   COLOR_MAGENTA, COLOR_YELLOW, COLOR_CYAN };

	start_color();
	attr_get(&attr, &pair, NULL);
	pair_content(pair, &numbercolors[0], &bkgnd);
	numbercell[0] = ' ' | A_NORMAL;
	for (n = 1 ; n <= 6 ; ++n) {
	    init_pair(n, numbercolors[n], bkgnd);
	    numbercell[n] = ('0' + n) | COLOR_PAIR(n);
	}
	attr = COLOR_PAIR(3);
	numbercell[7] = '7' | A_BOLD;
	numbercell[8] = '8' | A_BOLD | attr;
    } else {
	numbercell[0] = ' ';
	for (n = 1 ; n <= 8 ; ++n)
	    numbercell[n] = ('0' + n) | A_NORMAL;
	attr = A_NORMAL;
    }
#ifdef USE_LATIN1
    coveredcell = 183 | A_NORMAL;
#else
    coveredcell = ACS_BULLET | A_NORMAL;
#endif
    minecell = '*' | attr;
    boomcell = minecell | A_BOLD;
    flagcell = '+' | A_NORMAL;
    badflagcell = 'x' | A_BOLD | attr;

    return TRUE;
}

/*
 * Miscellaneous interface functions.
 */

/* Ring the bell.
 */
void ding(void)
{
    if (!silence)
	beep();
}

/* Display a formatted message on stderr and exit cleanly.
 */
void die(char const *fmt, ...)
{
    va_list	args;

    shutdown();
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    exit(EXIT_FAILURE);
}
