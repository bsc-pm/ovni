#!/bin/sh
# Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

set -ex

if [ -n "$OVNI_TEST_VERBOSE" ]; then
  set -x
fi

dir=$(readlink -f "${OVNI_CURRENT_DIR}")
testname="$dir/$1"
workdir="${testname}.trace"
tracedir="${workdir}/ovni"
emubin="${OVNI_BUILD_DIR}/ovniemu"
sortbin="${OVNI_BUILD_DIR}/ovnisort"

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

if [ -n "$OVNI_DO_SORT" ]; then
  "$sortbin" "$tracedir"
fi

# Then launch the emulator in lint mode
"$emubin" $OVNI_EMU_ARGS -l "$tracedir"

# Run any post script that was generated
ls -1 *.sh | while read sh; do
  echo "Running '$sh'"
  bash -ex $sh
done

#rm -rf $tracedir
