"""csokoban.py SESSION HOME: plays csokoban from Microban's first level on,
for the catalog preview (asciiverse/record/record.sh).

Each level is read from series/microban-1.txt and solved here by a
breadth-first search over the player's moves (the early Microban levels
are small), then the moves are typed with h j k l at a watchable pace;
Return goes on to the next level. Stops after SECONDS (default 50)."""
import os, subprocess, sys, time
from collections import deque

S, home = sys.argv[1], sys.argv[2]
secs = float(os.environ.get("SECONDS_LIMIT", "50"))
TMUX = ["tmux", "-L", os.environ["TMUX_SOCKET"]]


def key(*k):
    subprocess.run(TMUX + ["send-keys", "-t", S, *k])


def levels(path):
    """The levels of a set: lists of rows, in order."""
    out, cur = [], []
    for line in open(path).read().split("\n"):
        if line.startswith(";"):
            if cur:
                out.append(cur)
            cur = []
        elif line.strip():
            cur.append(line.rstrip())
    if cur:
        out.append(cur)
    return out


def solve(rows):
    """The shortest list of moves (h j k l) solving the level, or None."""
    walls, goals, boxes, player = set(), set(), set(), None
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            if c == "#": walls.add((y, x))
            if c in ".+*": goals.add((y, x))
            if c in "$*": boxes.add((y, x))
            if c in "@+": player = (y, x)
    dirs = {"h": (0, -1), "l": (0, 1), "k": (-1, 0), "j": (1, 0)}
    start = (player, frozenset(boxes))
    prev = {start: None}
    q = deque([start])
    while q:
        state = q.popleft()
        p, bx = state
        if bx <= goals:
            path = []
            while prev[state]:
                state, m = prev[state]
                path.append(m)
            return path[::-1]
        for m, (dy, dx) in dirs.items():
            n = (p[0] + dy, p[1] + dx)
            if n in walls:
                continue
            nb = bx
            if n in bx:
                b2 = (n[0] + dy, n[1] + dx)
                if b2 in walls or b2 in bx:
                    continue
                nb = (bx - {n}) | {b2}
            s2 = (n, frozenset(nb))
            if s2 not in prev:
                prev[s2] = (state, m)
                q.append(s2)
            if len(prev) > 400000:
                return None
    return None


sets = levels(os.path.join(home, "bin", "series", "microban-1.txt"))
time.sleep(2.0)
t0 = time.time()
for n, rows in enumerate(sets):
    moves = solve(rows)
    if moves is None:
        key("N"); time.sleep(1.0); continue
    for m in moves:
        key(m)
        time.sleep(0.16)
    time.sleep(1.4)
    if time.time() - t0 > secs:
        break
    key("Enter")
    time.sleep(1.0)
