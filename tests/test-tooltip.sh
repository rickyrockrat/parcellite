#!/bin/sh
# Native hover acceptance on an isolated display; no real clipboard/history.
set -eu
binary=$(realpath "${1:-$(dirname "$0")/../src/parcellite}")
tmp=$(mktemp -d /tmp/parcellite-tooltip.XXXXXX)
app=''; xvfb=''
cleanup() {
    test -z "$app" || kill "$app" 2>/dev/null || true
    test -z "$xvfb" || kill "$xvfb" 2>/dev/null || true
}
trap cleanup EXIT HUP INT TERM
mkdir -p "$tmp/home" "$tmp/config/parcellite" "$tmp/data"
export HOME="$tmp/home" XDG_CONFIG_HOME="$tmp/config" XDG_DATA_HOME="$tmp/data"
menu_y=${MENU_Y:-40}
cursor_y=$((menu_y + 14))
cat > "$tmp/config/parcellite/parcelliterc" <<EOF
[rc]
use_copy=true
use_primary=false
save_history=false
automatic_paste=false
item_length=50
single_line=true
history_pos=true
history_x=20
history_y=$menu_y
history_key=<Alt>V
no_icon=true
EOF
python3 - "$tmp/payload" "${PREVIEW_CASE:-normal}" <<'PY'
from pathlib import Path
import sys
Path(sys.argv[1]).write_text('FULL_CLIP_START <b>literal markup</b> & "quotes"\n'
                           'Second line with a preserved newline.\n'
                           + 'A roomy preview can display the text missing from the menu. ' * 6
                           + '\nFULL_CLIP_END\n\n  ', encoding='utf-8')
if sys.argv[2] == 'large':
    Path(sys.argv[1]).write_text('FULL_CLIP_START\n' + 'Synthetic caf\u00e9 / \u65e5\u672c\u8a9e / \U0001f642 ' * 20
                               + '\n' + 'Large preview line\n' * 20000 + 'FULL_CLIP_END\n')
elif sys.argv[2] == 'short':
    Path(sys.argv[1]).write_text('Short preview\nTiny')
PY
Xvfb -displayfd 3 -screen 0 1024x768x24 -nolisten tcp 3> "$tmp/display" > "$tmp/xvfb.log" 2>&1 &
xvfb=$!
for n in $(seq 1 50); do test ! -s "$tmp/display" || break; sleep 0.1; done
test -s "$tmp/display"
export DISPLAY=":$(cat "$tmp/display")"
xclip -selection clipboard -i "$tmp/payload"
"$binary" > "$tmp/app.log" 2>&1 &
app=$!
sleep 1
xdotool key alt+v
xdotool mousemove 950 740
sleep 0.8
for window in $(xdotool search --onlyvisible --pid "$app"); do
    if xprop -id "$window" _NET_WM_WINDOW_TYPE | grep -q '_NET_WM_WINDOW_TYPE_TOOLTIP'; then
        echo "Preview appeared without hovering"; exit 1
    fi
done
xdotool mousemove 80 "$cursor_y"
sleep 0.2
xdotool mousemove 83 "$cursor_y"
sleep 0.15
xdotool mousemove 80 "$cursor_y"
for window in $(xdotool search --onlyvisible --pid "$app"); do
    if xprop -id "$window" _NET_WM_WINDOW_TYPE | grep -q '_NET_WM_WINDOW_TYPE_TOOLTIP'; then
        echo "Tooltip appeared before its 500ms delay"; exit 1
    fi
done
if test "${PREVIEW_CASE:-normal}" = large; then
    # First-time multilingual font/layout setup can outlast the timer itself.
    sleep 1
else
    sleep 0.3
fi
tip=''
menu=''
for window in $(xdotool search --onlyvisible --pid "$app"); do
    if xprop -id "$window" _NET_WM_WINDOW_TYPE | grep -q '_NET_WM_WINDOW_TYPE_TOOLTIP'; then
        tip=$window
    elif xprop -id "$window" _NET_WM_WINDOW_TYPE | grep -q '_NET_WM_WINDOW_TYPE_POPUP_MENU'; then
        menu=$window
    fi
done
test -n "$tip" || { echo "No visible tooltip; evidence: $tmp"; exit 1; }
test -n "$menu"
right=$(xdotool getwindowgeometry --shell "$menu" | awk -F= '/^X=/{x=$2} /^WIDTH=/{print x+$2-5}')
xdotool getwindowgeometry --shell "$tip" > "$tmp/geometry-before"
awk -F= -v py="$cursor_y" '/^X=/{if ($2 != 80) exit 1} /^Y=/{y=$2} /^HEIGHT=/{h=$2}
         END {if (y<=py && y+h>py) exit 1}' "$tmp/geometry-before"
awk -F= '/^X=/{x=$2} /^Y=/{y=$2} /^WIDTH=/{w=$2} /^HEIGHT=/{h=$2}
         END {if (x<0 || y<0 || x+w>1024 || y+h>768) exit 1}' "$tmp/geometry-before"
if test "${PREVIEW_CASE:-normal}" = short; then
    awk -F= '/^WIDTH=/{if ($2 >= 200) exit 1}' "$tmp/geometry-before"
fi
for point in "83 $cursor_y" "$(( (80+right)/2 )) $((cursor_y+6))" "$right $((cursor_y+10))" "80 $cursor_y"; do
    xdotool mousemove $point
    sleep 0.8
    xwininfo -id "$tip" | grep -q 'Map State: IsViewable'
    xdotool getwindowgeometry --shell "$tip" > "$tmp/geometry-after"
    cmp "$tmp/geometry-before" "$tmp/geometry-after"
done
xclip -selection clipboard -o > "$tmp/after-hover"
cmp "$tmp/payload" "$tmp/after-hover"
python3 - "$tmp/hover.png" <<'PY'
import importlib.util
import sys
if importlib.util.find_spec('PyQt6'):
    from PyQt6.QtWidgets import QApplication
    app = QApplication([])
    assert app.primaryScreen().grabWindow(0).save(sys.argv[1])
else:
    print('Skipping optional screenshot: PyQt6 is not installed.')
PY
for cycle in 1 2 3; do
    xdotool key Escape
    sleep 0.2
    xdotool key alt+v
    xdotool mousemove 80 "$cursor_y"
    sleep 1
    kill -0 "$app"
done
xdotool key Escape
sleep 0.2
if grep -Eq 'CRITICAL|invalid.*(pointer|object)' "$tmp/app.log"; then
    cat "$tmp/app.log"
    exit 1
fi
xclip -selection clipboard -o > "$tmp/after-close"
cmp "$tmp/payload" "$tmp/after-close"
echo "PASS: delayed tooltip appears below cursor and stays still; clipboard unchanged. Evidence: $tmp"
