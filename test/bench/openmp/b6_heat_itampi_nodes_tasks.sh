export NOSV_APPID=1
export NOSV_CONFIG_OVERRIDE=instrumentation.version=ovni,ovni.level=2
#export OMP_NUM_THREADS=1
export OMP_OVNI=1

# FIXME: Disable OFI for now as we don't have a working hfi network
export FI_PROVIDER=sockets
b6_heat_itampi_nodes_tasks -b 128 -t 10

ovnisort ovni
ovnitop ovni
ovniemu -b ovni

# Make sure the trace is not too big (limit at 128 MiB)
maxsize=$((128 * 1024 * 1024))

for f in ovni/{cpu,thread}.prv; do
  size=$(stat -c %s $f)
  if [ $size -lt $maxsize ]; then
    echo "$f: size ok ($size < $maxsize)"
  else
    echo "$f: too big ($size >= $maxsize)"
  fi
done
