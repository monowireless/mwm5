#!/bin/bash

export LANG=en_US.utf8

## ENV
DIR=$1
SRC=./MakeKTbl.tmp
MANDATE=./MakeKTbl.mandate

## check GCC
GCC=gcc
GCCOPT="-E -fpreprocessed"
UNAME_S=$(uname -s)
if type gcc-9; then
  # if found gcc-9, use it.
  GCC=gcc-9
elif [ $UNAME_S = Darwin ]; then
  # macOS has clang instead, -fpreprocessed is not. 
  GCCOPT="-E"
fi

echo analysing source codes...
echo   use source format \"$GCC $GCCOPT\"
# rm -f $SRC
for f in $DIR/*/*; do
  $GCC -E $GCCOPT 2>/dev/null $f 
done |python3 MakeKTblPre.py > $SRC

# make minimum set
echo generating mimimul set...
cat $SRC $MANDATE | python3 MakeKTbl.py > kanjitable_minimul.txt
wc -l kanjitable_minimul.txt

# make standard set
echo generating standard set...
cat kanjitable_joyo.txt $MANDATE | python3 MakeKTbl.py > kanjitable_std.txt
wc -l kanjitable_std.txt

