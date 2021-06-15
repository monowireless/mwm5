#!/bin/bash

STGBASE=TWELITE_Stage

function bld() {
  echo make RPI_SDL2_LIB=$1 RPI_ARMv6=$2

  SUFF=""
  if [ "$2" = "1" ]; then SUFF="_v6"; fi

  echo mv ${STGBASE}.run ${STGBASE}_$1${SUFF}.run
}

bld X11 1
bld fb 1
#bld X11 0
#bld fb 0

echo "# type '...$0 | sh' to execute."
