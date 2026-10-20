"""cmines.py SESSION HOME: plays cmines for the catalog preview
(asciiverse/record/record.sh), started as "cmines Intermediate".

Reads the field from the pane (the frame's corner gives its place; a
covered cell is a middle dot, an opened one a digit or a space, a flag
+; tmux's capture may show the line drawing and the dot as the DEC
characters l q k x and ~), and plays as a careful player would: flags the cells a number shows
must be mines, opens the cells a number shows are safe (Return on a
satisfied number opens its neighbours), and guesses only when nothing
is certain, preferring corners and edges. It walks the cursor with h j
k l, following the cursor tmux reports. Plays new games (n) until it
wins one after 25 seconds, or SECONDS (default 75) have passed."""
import os, random, subprocess, sys, time

S, home = sys.argv[1], sys.argv[2]
secs = float(os.environ.get("SECONDS_LIMIT", "75"))
TMUX = ["tmux", "-L", os.environ["TMUX_SOCKET"]]
random.seed(7)


def key(*k):
    subprocess.run(TMUX + ["send-keys", "-t", S, *k])


def screen():
    out = subprocess.run(TMUX + ["capture-pane", "-p", "-t", S],
                         capture_output=True, text=True).stdout
    return [r.ljust(100) for r in (out.split("\n") + [""] * 30)[:30]]


def cursor():
    out = subprocess.run(TMUX + ["display-message", "-p", "-t", S,
                                 "#{cursor_x} #{cursor_y}"],
                         capture_output=True, text=True).stdout.split()
    return int(out[0]), int(out[1])


def field(rows):
    """(oy, ox, cells) with cells[y][x] one of '#' covered, '+' flag,
    0-8 opened, '*' mine shown; or None."""
    for y, r in enumerate(rows):
        x = r.find("┌") if "┌" in r else r.find("lq")
        if x >= 0:
            w = r.find("┐", x) if "┐" in r else r.find("qk", x) + 1
            if w < 0:
                continue
            cols = (w - x - 1) // 2
            cells = []
            yy = y + 1
            while yy < 30 and rows[yy][x] in "│x":
                line = []
                for c in range(cols):
                    ch = rows[yy][x + 2 + 2 * c]
                    if ch in "·~":
                        line.append("#")
                    elif ch == " ":
                        line.append(0)
                    elif ch.isdigit():
                        line.append(int(ch))
                    else:
                        line.append(ch)
                cells.append(line)
                yy += 1
            return y, x, cells
    return None


def neighbours(cells, y, x):
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if (dy or dx) and 0 <= y + dy < len(cells) and 0 <= x + dx < len(cells[0]):
                yield y + dy, x + dx


def decide(cells):
    """('open'|'flag'|'chord'|'guess', y, x)."""
    for y, line in enumerate(cells):
        for x, v in enumerate(line):
            if not isinstance(v, int) or v == 0:
                continue
            cov = [(a, b) for a, b in neighbours(cells, y, x) if cells[a][b] == "#"]
            flags = sum(1 for a, b in neighbours(cells, y, x) if cells[a][b] == "+")
            if cov and flags == v:
                return ("chord", y, x)
            if cov and flags + len(cov) == v:
                return ("flag",) + cov[0]
    covered = [(y, x) for y, line in enumerate(cells) for x, v in enumerate(line) if v == "#"]
    if not covered:
        return None
    h, w = len(cells), len(cells[0])
    edge = [c for c in covered if c[0] in (0, h - 1) or c[1] in (0, w - 1)]
    return ("guess",) + random.choice(edge or covered)


def goto(oy, ox, y, x):
    cx, cy = cursor()
    cy, cx = cy - oy - 1, (cx - ox - 2) // 2
    while (cy, cx) != (y, x):
        if cx < x: key("l"); cx += 1
        elif cx > x: key("h"); cx -= 1
        elif cy < y: key("j"); cy += 1
        else: key("k"); cy -= 1
        time.sleep(0.04)


time.sleep(2.0)
t0 = time.time()
while time.time() - t0 < secs:
    rows = screen()
    f = field(rows)
    if not f:
        time.sleep(0.3); continue
    oy, ox, cells = f
    lost = any(v == "*" for line in cells for v in line)
    won = not lost and not any(v == "#" for line in cells for v in line)
    if won and time.time() - t0 > 25:
        time.sleep(3.0)     # end the take on a won game
        break
    if lost or won:
        time.sleep(2.5)
        key("n"); time.sleep(1.0); continue
    d = decide(cells)
    if not d:
        time.sleep(0.5); continue
    act, y, x = d
    goto(oy, ox, y, x)
    time.sleep(0.12)
    key({"open": "Space", "guess": "Space", "flag": ".", "chord": "Enter"}[act])
    time.sleep(0.25 if act != "guess" else 0.6)
