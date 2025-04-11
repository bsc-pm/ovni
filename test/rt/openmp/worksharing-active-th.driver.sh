target=$OVNI_TEST_BIN

export NOSV_APPID=1
export OMP_NUM_THREADS=1

$target

ovnisort ovni

ovniemu -l ovni

# Mark API adds 100 to the type
prvtype="100"

row=$(grep '100:123$' ovni/thread.prv | head -1 | cut -d: -f 5)
t0=$(grep '100:123$' ovni/thread.prv | head -1 | cut -d: -f 6)
t1=$(grep '100:123$' ovni/thread.prv | tail -1 | cut -d: -f 6)

PRV_THREAD_STATE=4
TH_ST_PAUSED=2

# 2:0:1:1:1:15113228:100:123
count=$(grep "2:0:1:1:$row:.*:$PRV_THREAD_STATE:$TH_ST_PAUSED" ovni/thread.prv |\
  awk -F: '$6 >= '$t0' && $6 <= '$t1' {n++} END {print n}')

if [ "$count" != 100 ]; then
  echo "FAIL: expected 100 pause events"
  exit 1
else
  echo "OK: found 100 pause events"
fi
