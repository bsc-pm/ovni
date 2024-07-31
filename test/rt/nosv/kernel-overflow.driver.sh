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

# Flush the kernel after overflowing the ring buffer, by writing to it twice the
# ring size
nflush=$((2 * $ringsize / $cspair))

cat > nosv.toml << EOF
instrumentation.version = "ovni"
ovni.level = 3
ovni.kernel_ringsize = "${ringsize}"
EOF

# Should fail in runtime
set +e
$target $ncs $nflush 2> err.log
rc=$?
set -e

# Dump log
cat err.log

# Ensure no nOS-V warnings at runtime
if grep -c "cannot enable kernel events" err.log; then
  echo "cannot enable kernel events in all threads" >&2
  exit 1
fi

# Ensure it detects the error
if ! grep -c "Kernel events lost" err.log; then
  echo "cannot find 'Kernel events lost' error message" >&2
  exit 1
fi

if [ "$rc" == "0" ]; then
  echo "program didn't failed" >&2
  exit 1
fi
