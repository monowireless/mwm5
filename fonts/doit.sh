#!/bin/bash
export LANG=en_US.utf8

if [ ! -d outdir ]; then
  mkdir outdir
fi

# update kanji table
bash MakeKTbl.sh ../examples/

# make font files
DIRS="mplus10 mplus12 shinonome12 shinonome14 shinonome16"
rm -f outdir/*
for f in $DIRS; do
    echo generating source codes - $f 
    python3 BdfToCppSrc.py --config $f/config.ini
done

# copy files to source dir
echo --------------------------------------------------
echo -n "Copy source files to ../src/font? [y/N]"
read i
if [ "$i" == "y" -o "$i" == "Y" ]; then
  cp -vf outdir/* ../src/font
fi

