#!/bin/bash

# This script return 0 if and only if the given program returns non-zero
# AND the regex matches the output

# $1 = the regex as grep
# $2... The program

regex="$1"
shift

"${@}" 2>&1 | stdbuf -i0 -o0 tee /dev/stderr | grep -q "${regex}"

rcprog=${PIPESTATUS[0]} rcgrep=${PIPESTATUS[2]}

echo "rcprog='$rcprog' rcgrep='$rcgrep'"

if [ "$rcprog" != 0 ] && [ "$rcgrep" = 0 ]; then
  echo "ok: program failed and grep matched the error line"
  exit 0
else
  if [ "$rcprog" = 0 ]; then
    echo "error: program exited with 0 rather than failure"
  fi
  if [ "$rcgrep" != 0 ]; then
    echo "error: regex \"${regex}\" not matched"
  fi
  exit 1
fi
