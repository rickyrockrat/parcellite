#!/bin/sh
 
echo "$0">svnversion.log
if [ -n "$1" ]; then
  echo "Immediate Mode">>svnversion.log
  IM=1
else
  IM=0
fi
which svnversion > _svnversion_test
if [ $? -ne 0 ] ;then
  echo "svn"
else
  CWD=$(pwd)
#path supplied?
  D=$(dirname $0)
  if [ "." = "$D" ]; then
    PPATH="$CWD" #no
  else
    PPATH="$D"
  fi
  cd "$PPATH"
# are we lndired?
  LINK=$(readlink configure.ac)
  if [ -n "$LINK" ]; then #link
   FULLPATH=$(dirname "$LINK")
  else
   FULLPATH="$CWD"
  fi
  cd "$FULLPATH"
  echo "Using $FULLPATH">>"$CWD/svnversion.log"
  SVER=$(svn info .|grep -i "revision"|sed 's!.*: !!'|tr -d ' ')
  echo -n "svn$SVER"
  cd $CWD
  if [ "$IM" = "1" ]; then
    sed -i "s#\(.*VERSION \).*#\1\"svn$SVER\"#" config.h
    echo "Make sure to do svn up in source svn to update local svn info, then run again."
    rm src/main.o
    echo "Removed src/main.o. Recompile."
  fi
fi
