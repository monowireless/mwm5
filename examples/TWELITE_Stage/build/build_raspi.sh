#!/bin/bash

STGBASE=TWELITE_Stage
PARALLEL=-j4

function bld() {
  make RPI_SDL2_LIB=$1 RPI_ARMv6=$2 $PARALLEL

  SUFF=""
  if [ "$2" = "1" ]; then SUFF="_v6"; fi

  mv ${STGBASE}.run ${STGBASE}_$1${SUFF}.run
}

rm -f objs/gen/sdl2*.o

bld X11 0
bld fb 0

rm -f objs/gen/sdl2*.o

bld X11 1
bld fb 1

