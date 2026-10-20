#!/bin/sh
# Records a game's catalog preview at 100x30:
#
#   asciiverse/record/record.sh GAME FOLDER [ARGS...]
#
# FOLDER holds the game as packed (the program, and series/ for csokoban
# and cblocks). asciinema rec runs it by name, from a throwaway copy on
# PATH, with a throwaway home, in a detached 100x30 tmux session, while
# asciiverse/record/GAME.py plays it through the pane. Then the game is
# stopped with SIGKILL, so it draws nothing more (its own exit clears
# the screen), and the cast ends on the game. Writes asciiverse/GAME.cast.
set -eu
cd "$(dirname "$0")/../.."
game=$1
src=$(cd "$2" && pwd)
shift 2
out=$PWD/asciiverse/$game.cast
S=$game-preview-$$
home=$(mktemp -d)
trap 'tmux -L "$S" kill-server 2>/dev/null; rm -rf "$home"' EXIT
mkdir "$home/bin"
cp -r "$src"/. "$home/bin/"
tmux -L "$S" new-session -d -s "$S" -x 100 -y 30 -c "$home" \
  "env -u TMUX HOME=$home XDG_DATA_HOME= PATH=$home/bin:$PATH TERM=xterm-256color LANG=C.UTF-8 USER=player LOGNAME=player \
   asciinema rec --overwrite -q --window-size 100x30 --idle-time-limit 2 --title $game \
     --capture-env TERM,LANG -c 'exec $game $*' $out"
TMUX_SOCKET=$S python3 asciiverse/record/$game.py "$S" "$home"
sleep 1
# the game under this pane (not any other one running)
pids=$(tmux -L "$S" display-message -p -t "$S" '#{pane_pid}')
for _ in 1 2 3; do
  pids="$pids $(for p in $pids; do pgrep -P "$p" || true; done)"
done
for p in $(echo $pids | tr ' ' '\n' | sort -u); do
  [ "$(ps -o comm= -p "$p" 2>/dev/null)" = "$game" ] && kill -KILL "$p"
done
for _ in $(seq 1 20); do
  tmux -L "$S" has-session -t "$S" 2>/dev/null || break
  sleep 0.5
done
python3 - "$out" <<'PY'
import json, sys
lines = open(sys.argv[1]).read().splitlines()
keep = [lines[0]]
for line in lines[1:]:
    ev = json.loads(line)
    if ev[1] == "x" or (ev[1] == "o" and "\x1b[?1049l" in ev[2]):
        break
    keep.append(line)
open(sys.argv[1], "w").write("\n".join(keep) + "\n")
PY
echo "recorded $out"
