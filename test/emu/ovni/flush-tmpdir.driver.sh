target=$OVNI_TEST_BIN

mkdir tmp
export OVNI_TMPDIR=tmp
$target
ovniemu ovni
