target=$OVNI_TEST_BIN

# We will set our own
unset NOSV_CONFIG
unset NOSV_CONFIG_OVERRIDE

export NOSV_APPID=1

# Small kernel ring size to make execution fast
ringsize=$((64*1024))

# Each CS event pair occupies 16 bytes.
cspair=32

# How many CS event pairs fit in the ring buffer?
navail=$(($ringsize / $cspair))

# For the test, write 8 times the ring buffer
ncs=$((8 * $navail))

# Flush the kernel after writing 1/4 of the ring buffer. 
nflush=$(($ringsize / $cspair / 4))

cat > nosv.toml << EOF
instrumentation.version = "ovni"
ovni.level = 3
ovni.kernel_ringsize = "${ringsize}"
EOF

$target $ncs $nflush 2>&1 | tee err.log

# Ensure no nOS-V warnings at runtime
if grep -c "cannot enable kernel events" err.log; then
  echo "cannot enable kernel events in all threads" >&2
  exit 1
fi

# We need to sort the trace as it has unsorted regions
ovnisort ovni

# Ensure we have at least the number of context switches we caused, and ensure
# the number of "in" and "out" events match.
ovnitop ovni | awk '{n[$1] = $2} END { if (n["KCO"] != n["KCI"] || n["KCO"] < '$ncs') exit 1 }'

# Run the emulator
ovniemu -l ovni
