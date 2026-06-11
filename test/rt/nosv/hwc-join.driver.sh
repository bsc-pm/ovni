target=$OVNI_TEST_BIN

# We will set our own
unset NOSV_CONFIG
unset NOSV_CONFIG_OVERRIDE

export NOSV_APPID=1

cat > nosv.toml << EOF
instrumentation.version = "ovni"
ovni.level = 3
hwcounters.backend = "papi"
hwcounters.papi_events = [ "PAPI_TOT_INS", "PAPI_TOT_CYC" ]
EOF

$target

ovnisort ovni
ovniemu -l ovni

cat > v.awk << 'EOF'
BEGIN { rc = 0 }

$2 == "VAj" { rec = 1; need = 0; hwc = 0 }
rec && $2 == "VAJ" { done = 1 }

$2 == "VAw" { rec = 1; need = 0; hwc = 0 }
rec && $2 == "VAW" { done = 1 }

$2 == "VTp" { need = 1 } # Make mandatory HWC if task pauses

rec { line = line"\n"$0 }
rec && $2 == "VWC" { hwc++ }

rec && done { if (need && hwc < 2) { print "ERROR: missing HWC events: \n---" line "\n---"; rc = 1 }; rec = 0; line = ""; done = 0 }

END { print "rc =", rc; exit rc }
EOF


# Make sure we have HWC events inside nosv_join or nosv_waitfor when the task is
# paused
for loom in ovni/loom*; do
  for proc in $loom/proc*; do
    for thread in $proc/thread*; do
      ovnidump $thread | awk -f v.awk
    done
  done
done
