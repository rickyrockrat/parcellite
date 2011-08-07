#!/bin/sh
which svnversion > _svnversion_test
if [ $? -ne 0 ] ;then
  echo "svn"
else
  CWD=$(pwd)
  FULLPATH=$(ls -l $0 |sed 's/.*->\(.*\)/\1/')
  SDIR=$(dirname $FULLPATH)
  cd $SDIR
  echo -n "svn" && svnversion -n
  cd $CWD
fi
