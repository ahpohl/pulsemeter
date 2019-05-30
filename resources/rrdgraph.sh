#!/bin/sh
rrd=$1

# meter reading (kWh)
rrdtool graph meter_reading.png \
  -s 'now -30 min' -e 'now' \
  -X 0 -Y -A \
  DEF:counter=$rrd:counter:LAST \
  LINE2:counter#000000:"Meter reading [kWh]"
# energy (Ws)
rrdtool graph energy.png \
  -s 'now -30 min' -e 'now' \
  -Y -A \
  DEF:energy=$rrd:energy:AVERAGE \
  LINE2:energy#00FF00:"Energy [Ws]" 
