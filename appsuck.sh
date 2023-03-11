#!/bin/sh

if [ -z "$1" ]; then
	exit 1
fi

APPI=$(pkg-config --libs appindicator 2>/dev/null)
AY_APPI=$(pkg-config --libs ayatana-appindicator 2>/dev/null)

TYPE=""
if [ -n "$APPI" ]; then 
	TYPE="appindicator"
	inc="libappindicator/app-indicator.h"
	AP="$APPI"
fi
if [ -n "$AY_APPI" ]; then
	TYPE="ayatana-appindicator"
	inc="libayatana-appindicator/app-indicator.h"
	AP="$AY_APPI"
fi
if [ -n "$AP" ]; then
	cflags=$(pkg-config --cflags $AP)
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
if [ ! -e "$x" ]; then
	echo "#ifndef _CONFIG_SIMPLE_H_" > $x
	echo "#define _CONFIG_SIMPLE_H_ 1" >> $x
	if [ -n "$TYPE" ]; then
                echo "#define HAVE_APPINDICATOR" >> $x
		echo "#include <$inc>" >> $x
        fi
        echo "#endif" >> $x
fi
