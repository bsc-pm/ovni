#!/bin/sh
#
# Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
