target=$OVNI_TEST_BIN

export NOSV_APPID=1
export OMP_OVNI=1
export OMP_WAIT_POLICY=passive
export OMP_NUM_THREADS=4
export NOSV_CONFIG_OVERRIDE="instrumentation.version=ovni,ovni.level=2"

# Repeat several times, as the test is not stable. We only want to be sure that
# we never generate too many events.
for i in $(seq 10); do
  rm -rf ovni

  $target

  ovnisort ovni
  ovniemu -l ovni

  # Make sure that we only see a low number of threads being paused
  ovnitop ovni > top.txt
  cat top.txt
  awk -v n=500 '/^OHp/ && $2 > n { printf("too many OHp events: %d > %d", $2, n); exit 1 }' < top.txt
done
