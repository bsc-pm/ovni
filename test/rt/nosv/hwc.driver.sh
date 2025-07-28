target=$OVNI_TEST_BIN

# We will set our own
unset NOSV_CONFIG
unset NOSV_CONFIG_OVERRIDE

export NOSV_APPID=1

cat > nosv.toml << EOF
instrumentation.version = "ovni"
ovni.level = 2
hwcounters.backend = "papi"
hwcounters.papi_events = [ "PAPI_TOT_INS", "PAPI_TOT_CYC" ]
EOF

$target

ovniemu -l ovni
