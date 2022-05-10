#!/bin/sh

if [ -z "$1" ]; then
	exit 1
fi

APPI=$(pkg-config --libs appindicator)
AY_APPI=$(pkg-config --libs ayatana-appindicator)

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
cflags=$(pkg-config --cflags $AP)
case $1 in 
	type) echo "$TYPE";;
	cflags) echo "$cflags";;
	lib) echo "$AP";;
	config) config=1;;
	*) exit 1
esac
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
