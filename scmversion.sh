#!/bin/sh
LOG="scmversion.log" 
TMP=_scmversion_test
echo "$0">$LOG
if [ -n "$1" ]; then
  echo "Immediate Mode">>$LOG
  IM=1
else
  IM=0
fi
which svnversion > $TMP
which git >> $TMP
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
  echo "Using $FULLPATH">>"$CWD/$LOG"
  if [ -e .svn ]; then
    SVER=$(svn info .|grep -i "revision"|sed 's!.*: !!'|tr -d ' ')
    SVER="svn-$SVER"
  elif [ -e .git ]; then
    SVER="git-$(git log -n1 --pretty=format:%h)"
  fi
  echo -n "$SVER"
  cd $CWD
  if [ "$IM" = "1" ]; then
    sed -i "s#\(.*VERSION \).*#\1\"$SVER\"#" config.h
    echo "Make sure to do scm update in source scm to update local scm info, then run again."
    rm src/main.o
    echo "Removed src/main.o. Recompile."
  fi
fi
