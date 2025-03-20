target=$OVNI_TEST_BIN

# We will set our own
unset NOSV_CONFIG
unset NOSV_CONFIG_OVERRIDE

export NOSV_APPID=1

cat > nosv.toml << EOF
instrumentation.version = "ovni"
ovni.level = 3
EOF

$target

. ./vars.sh

# We need to sort the trace as it has unsorted regions
ovnisort ovni

# Ensure that we don't find any noisy events by using the limit of 5% of
# nosv_yield calls. It is fine if some events don't appear.
ovnitop ovni | awk -v n=$nyields \
  '/^V(AY|Ay|Sh|Sf|S\[|S\]|SN|Sn|Pr|Pp)/ { \
    if ($2 < 0.05 * n) { print $0, "OK" }
    else { print $0, "BAD"; bad++ }
  } END { if (bad != 0) { exit 1 } }'

# Perform the emulation with breakdown enabled
ovniemu -b ovni
