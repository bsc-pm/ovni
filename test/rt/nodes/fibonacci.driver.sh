target=$OVNI_TEST_BIN

$target 2>stderr.log

# Ensure no nOS-V warnings at runtime
if grep -c "cannot enable kernel events" stderr.log; then
  echo "cannot enable kernel events in all threads" >&2
  exit 1
fi

# We need to sort the trace as it has unsorted regions
ovnisort ovni

# Ensure we have at least a pair of kernel CS events
ovnitop ovni | grep KCO
ovnitop ovni | grep KCI

# Run the emulator
ovniemu -l ovni
