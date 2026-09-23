#!/bin/sh

if [ -z "$1" ]; then
	exit 1
fi

# Fedora/Debian pkg-config names vary; probe common ones.
probe_pc() {
	for pc in "$@"; do
		libs=$(pkg-config --libs "$pc" 2>/dev/null) || continue
		if [ -n "$libs" ]; then
			echo "$pc"
			return 0
		fi
	done
	return 1
}

TYPE=""
inc=""
AP=""
cflags=""

PC=$(probe_pc ayatana-appindicator-0.1 ayatana-appindicator)
if [ -n "$PC" ]; then
	TYPE="ayatana-appindicator"
	inc="libayatana-appindicator/app-indicator.h"
	AP=$(pkg-config --libs "$PC" 2>/dev/null)
	cflags=$(pkg-config --cflags "$PC" 2>/dev/null)
else
	PC=$(probe_pc appindicator-0.1 appindicator)
	if [ -n "$PC" ]; then
		TYPE="appindicator"
		inc="libappindicator/app-indicator.h"
		AP=$(pkg-config --libs "$PC" 2>/dev/null)
		cflags=$(pkg-config --cflags "$PC" 2>/dev/null)
	fi
fi

case $1 in
	type) echo "$TYPE";;
	cflags) echo "$cflags";;
	lib) echo "$AP";;
	config) config=1;;
	*) exit 1
esac
d=$(basename "$(pwd)")
if [ "$d" = "src" ]; then exit 0; fi
x="src/config.simple.h"
# Always rewrite when 'config' is requested so a stale header cannot
# permanently disable AppIndicator after installing the devel package.
if [ -n "$config" ] || [ ! -e "$x" ]; then
	echo "#ifndef _CONFIG_SIMPLE_H_" > "$x"
	echo "#define _CONFIG_SIMPLE_H_ 1" >> "$x"
	if [ -n "$TYPE" ]; then
		echo "#define HAVE_APPINDICATOR" >> "$x"
		echo "#include <$inc>" >> "$x"
	fi
	echo "#endif" >> "$x"
fi
