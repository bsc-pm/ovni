target=$OVNI_TEST_BIN

# We will set our own
unset NOSV_CONFIG
unset NOSV_CONFIG_OVERRIDE

export NOSV_APPID=1

cat > nosv.toml << EOF
instrumentation.version = "ovni"
ovni.level = 4
EOF

$target

. ./vars.sh

# We need to sort the trace as it has unsorted regions
ovnisort ovni

# Ensure we get a lot of VA[yY] and VS[nN] events. We need to make sure that all
# events are matched.
ovnitop ovni | awk -v n=$nyields \
  '/^VA[Yy]/ { \
    /* Match the number or nosv_yield calls exactly */
    if ($2 == n) { print $0, "OK"; ok++ }
    else { print $0, "BAD" }
  } \
  /^VS[Nn]/ { \
    /* Use 2% for the non-blocking scheduler events */
    if ($2 > 0.02 * n) { print $0, "OK"; ok++ }
    else { print $0, "BAD" }
  } END {
    /* Be sure we matched the 4 rules */
    if (ok != 4) { exit 1 }
  }'

# Avoid emulation as may be huge
