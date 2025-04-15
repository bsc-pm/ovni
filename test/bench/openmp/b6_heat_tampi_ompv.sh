export NOSV_APPID=1
export NOSV_CONFIG_OVERRIDE=instrumentation.version=ovni,ovni.level=2
export OMP_OVNI=1

b6_heat_tampi_ompv -b 256 -t 4

ovnisort ovni
ovnitop ovni
ovniemu -b ovni

# Make sure the trace is not too big (limit at 128 MiB)
maxsize=$((128 * 1024 * 1024))

for f in ovni/{cpu,thread,openmp-breakdown}.prv; do
  size=$(stat -c %s $f)
  if [ $size -lt $maxsize ]; then
    echo "$f: size ok ($size < $maxsize)"
  else
    echo "$f: too big ($size >= $maxsize)"
  fi
done
