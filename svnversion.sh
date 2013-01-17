#!/bin/sh
echo "$0">svnversion.log
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
  SVER=$(svn info -r HEAD|grep "Last Changed Rev"|sed 's!.*: !!')
  echo -n "svn$SVER"
  cd $CWD
fi
