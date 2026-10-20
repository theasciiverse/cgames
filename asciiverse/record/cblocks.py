"""cblocks.py SESSION HOME: plays cblocks for the catalog preview
(asciiverse/record/record.sh), from the first of its Classic puzzles.

Each puzzle is read from series/Classics.txt and solved here by a
breadth-first search over one-square slides (blocks are the letters and
digits of the map, # are walls; the target map and equiv lines give the
goal). The game is played with the keys as a player would: the arrow
keys move the selection square by square onto the block to slide, and
H J K L slide it. (Mouse clicks would do, but asciinema does not pass
injected mouse sequences on to the game.) Where the puzzle sits on the
100x30 screen is worked out as cblocks does (centred with its sidebar,
four columns and two rows a square), and the selection is followed
through the cursor tmux reports. Return goes on to the next puzzle;
stops after SECONDS (default 55)."""
import os, subprocess, sys, time
from collections import deque

S, home = sys.argv[1], sys.argv[2]
secs = float(os.environ.get("SECONDS_LIMIT", "55"))
TMUX = ["tmux", "-L", os.environ.get("TMUX_SOCKET", "default")]
COLS, LINES, SIDEBAR, MINHEIGHT = 100, 30, 18, 16
DIRS = {"K": (-1, 0), "J": (1, 0), "H": (0, -1), "L": (0, 1)}


def key(*k):
    subprocess.run(TMUX + ["send-keys", "-t", S, *k])


def cursorcell(p, oy, ox):
    """The square the selection cursor is on, or None when it is parked
    beside the sidebar (no square selected)."""
    out = subprocess.run(TMUX + ["display-message", "-p", "-t", S,
                                 "#{cursor_x} #{cursor_y}"],
                         capture_output=True, text=True).stdout.split()
    cx, cy = int(out[0]), int(out[1])
    if (cy - oy) % 2 or (cx - ox - 1) % 4:
        return None
    y, x = (cy - oy) // 2 - 1, (cx - ox - 1) // 4 - 1
    if 0 <= y < p["h"] and 0 <= x < p["w"]:
        return y, x
    return None


def select(p, oy, ox, cells, start):
    """Move the selection onto one of cells (a block's squares); start is
    the square the game moves from when none is selected."""
    at = cursorcell(p, oy, ox) or start
    for _ in range(40):
        if at in cells:
            return
        ty, tx = min(cells, key=lambda c: abs(c[0] - at[0]) + abs(c[1] - at[1]))
        if ty != at[0]:
            key("Down" if ty > at[0] else "Up")
        else:
            key("Right" if tx > at[1] else "Left")
        time.sleep(0.12)
        at = cursorcell(p, oy, ox) or at


def puzzles(path):
    out, cur, mode, lines = [], None, None, []
    for line in open(path).read().split("\n"):
        word = line.split(" ")[0]
        if mode in ("initial", "target"):
            cur[mode].append(line.ljust(cur["w"])[:cur["w"]])
            if len(cur[mode]) == cur["h"]:
                mode = None
            continue
        if word == "size":
            cur = {"w": int(line.split()[1]), "h": int(line.split()[2]),
                   "initial": [], "target": [], "equiv": []}
        elif word in ("initial", "target"):
            mode = word
        elif word == "equiv":
            cur["equiv"].append(line.split()[1])
        elif word == "end" and cur:
            out.append(cur); cur = None
    return out


def blocks(rows):
    """{name: frozenset of (y, x)}, walls: set, from a map."""
    seen, out, walls = set(), {}, set()
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c == "#":
                walls.add((y, x))
            elif c.isalnum() or c == "$":
                if (y, x) in seen:
                    continue
                cells, stack = set(), [(y, x)]
                while stack:
                    p = stack.pop()
                    if p in cells: continue
                    cells.add(p)
                    if c == "$": break
                    for dy, dx in DIRS.values():
                        q = (p[0] + dy, p[1] + dx)
                        if 0 <= q[0] < len(rows) and 0 <= q[1] < len(rows[0]) \
                           and rows[q[0]][q[1]] == c and q not in cells:
                            stack.append(q)
                seen |= cells
                name = c if c != "$" else f"${y},{x}"
                out[name] = frozenset(cells)
    return out, walls


def solve(p):
    init, walls = blocks(p["initial"])
    goal, _ = blocks(p["target"])
    names = sorted(init)
    # the goal: each target block's cells taken by a block of its class
    classes = {}
    for n in goal:
        cls = next((e for e in p["equiv"] if n in e), n)
        classes.setdefault(cls, []).append(goal[n])
    def shape(cells):
        my, mx = min(c[0] for c in cells), min(c[1] for c in cells)
        return frozenset((c[0] - my, c[1] - mx) for c in cells)
    def done(st):
        pos = dict(zip(names, st))
        for cls, targets in classes.items():
            have = {pos[n] for n in names if n in cls}
            if not all(t in have for t in targets):
                return False
        return True
    start = tuple(init[n] for n in names)
    prev = {start: None}
    q = deque([start])
    while q:
        st = q.popleft()
        if done(st):
            path = []
            while prev[st]:
                st, mv = prev[st]
                path.append(mv)
            return init, path[::-1]
        occ = set().union(*st) | walls
        for i, cells in enumerate(st):
            rest = occ - cells
            for k, (dy, dx) in DIRS.items():
                moved = frozenset((c[0] + dy, c[1] + dx) for c in cells)
                if any(not (0 <= c[0] < p["h"] and 0 <= c[1] < p["w"]) or c in rest for c in moved):
                    continue
                st2 = st[:i] + (moved,) + st[i + 1:]
                if st2 not in prev:
                    prev[st2] = (st, (names[i], k))
                    q.append(st2)
        if len(prev) > 300000:
            return init, None
    return init, None


def origin(p):
    """The screen row and column of the puzzle's border corner."""
    rows, cols = (p["h"] + 2) * 2, (p["w"] + 2) * 4
    h = max(rows, MINHEIGHT)
    return (LINES - h) // 2, (COLS - (cols + 2 + SIDEBAR)) // 2


if __name__ == "__main__":
    ps = puzzles(os.path.join(home, "bin", "series", "Classics.txt"))
    time.sleep(2.0)
    t0 = time.time()
    for p in ps:
        init, path = solve(p)
        if path is None:
            key("N"); time.sleep(1.0); continue
        oy, ox = origin(p)
        pos = dict(init)
        # cblocks starts with the first block of the map selected
        selected = min(pos, key=lambda n: min(pos[n]))
        for name, k in path:
            if name != selected:
                select(p, oy, ox, pos[name], min(pos[selected]))
                selected = name
                time.sleep(0.2)
            key(k)
            dy, dx = DIRS[k]
            pos[name] = frozenset((c[0] + dy, c[1] + dx) for c in pos[name])
            time.sleep(0.3)
        time.sleep(1.6)
        if time.time() - t0 > secs:
            break
        key("Enter")
        time.sleep(1.2)
