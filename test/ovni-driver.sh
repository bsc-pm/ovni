#!/bin/sh
# Copyright (c) 2021-2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

set -ex

if [ -n "$OVNI_TEST_VERBOSE" ]; then
  set -x
fi

dir=$(readlink -f "${OVNI_CURRENT_DIR}")
testname="$dir/$1"
workdir="${testname}.dir"
tracedir="${workdir}/ovni"

export PATH="${OVNI_BUILD_DIR}:$PATH"
export OVNI_TEST_BIN="$testname"

rm -rf "${workdir}"
mkdir -p "${workdir}"
cd "${workdir}"

if [ -z "$OVNI_NPROCS" ]; then
  OVNI_NPROCS=1
fi

# Let the test run its own driver
if [ -n "$OVNI_DRIVER" ]; then
  . "$OVNI_DRIVER"
else
  if [ "$OVNI_NPROCS" -gt 1 ]; then
    for i in $(seq 1 "$OVNI_NPROCS"); do
      rank=$(($i-1))
      # Run the test in the background
      (OVNI_RANK=$rank OVNI_NRANKS=$OVNI_NPROCS "$testname" && echo ok > ovni.rc.$rank) &
    done
    wait
    all_good=1
    for i in $(seq 1 "$OVNI_NPROCS"); do
      rank=$(($i-1))
      # Run the test in the background
      if ! grep -q ok "ovni.rc.$rank"; then
        echo "Test for rank $rank failed"
        all_good=0
      fi
    done
    if [ "$all_good" != 1 ]; then
      echo "Some tests failed"
      exit 1
    fi
  else
    "$testname"
  fi

  if [ -n "$OVNI_DO_SORT" ]; then
    ovnisort "$tracedir"
  fi

  if [ -z "$OVNI_NOEMU" ]; then
    # Then launch the emulator in lint mode
    ovniemu $OVNI_EMU_ARGS -l "$tracedir"
  fi
fi

# Run any post script that was generated
ls -1 *.sh | while read sh; do
  echo "Running '$sh'"
  bash -ex $sh
done

#rm -rf $tracedir
