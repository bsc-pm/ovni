target=$OVNI_TEST_BIN

# Ensure move all files to the final place, including the process metadata, the
# thread streams and metadata files.

test_files() {
  dst="$1"
  test -e "$dst"
  test -e "$dst/loom.node.1"
  test -e "$dst/loom.node.1/proc.123"
  test -e "$dst/loom.node.1/proc.123/thread.123"
  test -e "$dst/loom.node.1/proc.123/thread.123/stream.json"
  test -e "$dst/loom.node.1/proc.123/thread.123/stream.obs"
}

test_no_files() {
  dst="$1"
  test '!' -e "$dst"
  test '!' -e "$dst/loom.node.1"
  test '!' -e "$dst/loom.node.1/proc.123"
  test '!' -e "$dst/loom.node.1/proc.123/thread.123"
  test '!' -e "$dst/loom.node.1/proc.123/thread.123/stream.json"
  test '!' -e "$dst/loom.node.1/proc.123/thread.123/stream.obs"
}

# Test setting OVNI_TMPDIR
(
  mkdir tmp
  export OVNI_TMPDIR=tmp
  $target
  test_files "ovni"
  test_no_files "tmp"
  ovniemu ovni
)

# Test without tmpdir created
(
  rm -f tmp
  export OVNI_TMPDIR=tmp
  $target
  test_files "ovni"
  test_no_files "tmp"
  ovniemu ovni
)

# Also with OVNI_TRACEDIR
(
  mkdir tmp
  export OVNI_TMPDIR=tmp
  export OVNI_TRACEDIR="a/b"
  $target
  test_files "a/b"
  test_no_files "tmp"
  ovniemu ovni
)
