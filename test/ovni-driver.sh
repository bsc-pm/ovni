#!/bin/sh
# Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

set -e

dir=$(readlink -f "${OVNI_CURRENT_DIR}")
testname="$dir/$1"
workdir="${testname}.trace"
tracedir="${workdir}/ovni"
emubin="${OVNI_BUILD_DIR}/ovniemu"

mkdir -p "${workdir}"
cd "${workdir}"

rm -rf "$tracedir"

if [ -z "$OVNI_NPROCS" ]; then
  OVNI_NPROCS=1
fi

if [ "$OVNI_NPROCS" -gt 1 ]; then
  for i in $(seq 1 "$OVNI_NPROCS"); do
    # Run the test in the background
    OVNI_RANK=$(($i-1)) OVNI_NRANKS=$OVNI_NPROCS "$testname" &
  done
  wait
else
  "$testname"
fi

# Then launch the emulator in lint mode
"$emubin" -l "$tracedir"

#rm -rf $tracedir
