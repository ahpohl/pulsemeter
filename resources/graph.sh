#!/bin/bash
# energy is stored as counts
# power is stored as counts/s
rrd=$1

# revolutions per kWh of ferraris disk
rev_per_kwh=75

# convert counts to energy in kWh
rrdtool graph energy.png \
  --start 'now -20 min' --end 'now' \
  "DEF:counts=$rrd:energy:LAST" \
  "CDEF:energy=counts,$rev_per_kwh,/" \
  LINE2:energy#000000:"Meter reading [kWh]"

# factor: 3600*1000/75
factor=48000
# convert counts/s to power in W
rrdtool graph power.png \
  --start 'now -20 min' --end 'now' \
  "DEF:counts_per_sec=$rrd:power:AVERAGE" \
  "CDEF:power=counts_per_sec,$factor,*" \
  LINE2:power#00FF00:"Power [W]"
