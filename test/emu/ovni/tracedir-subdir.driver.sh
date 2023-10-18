target=$OVNI_TEST_BIN

# Test referring to a empty nested subdirectory
(
  export OVNI_TRACEDIR=a/b/c
  $target
  ovniemu $OVNI_TRACEDIR
)

# Test referring to a parent directory
(
  mkdir -p x/y/z
  cd x/y/z
  export OVNI_TRACEDIR=../../w
  $target
  ovniemu $OVNI_TRACEDIR
)

# Test with full path
(
  export OVNI_TRACEDIR=$(pwd)/fullpath
  $target
  ovniemu $OVNI_TRACEDIR
)
