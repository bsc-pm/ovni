export NOSV_APPID=1
export NOSV_CONFIG_OVERRIDE=instrumentation.version=ovni,ovni.level=2
#export OMP_NUM_THREADS=1
export OMP_OVNI=1

b6_heat_ompv -b 128 -t 10

ovnisort ovni
ovnitop ovni
ovniemu -b ovni
