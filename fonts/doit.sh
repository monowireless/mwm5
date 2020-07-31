#!/bin/bash
export LANG=en_US.utf8

# update kanji table
bash MakeKTbl.sh ../examples/

# make font files
DIRS="mplus10 mplus12 shinonome12 shinonome14 shinonome16"
rm outdir/*
for f in $DIRS; do
    echo generating source codes - $f 
    python3 BdfToCppSrc.py --config $f/config.ini
done

# copy files to source dir
cp -vf outdir/* ../src/font

