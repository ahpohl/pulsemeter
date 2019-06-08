#!/bin/bash
# energy is stored as counts
# power is stored as counts/s

# revolutions per kWh of ferraris disk
rev_per_kwh=75

# parameters
rrd=/var/lib/rrdcached/pulse.rrd

# convert counts to energy in kWh
rrdtool graph energy.png \
  --daemon "unix:/run/rrdcached/rrdcached.sock" \
  --start 'now -10 hours' --end now \
  -Y -X 0 -A \
  "DEF:counts=$rrd:energy:LAST" \
  "CDEF:energy=counts,$rev_per_kwh,/" \
  LINE2:energy#000000:"Meter reading [kWh]"

# factor: 3600*1000/75
factor=48000
# convert counts/s to power in W
rrdtool graph power.png \
  --daemon "unix:/run/rrdcached/rrdcached.sock" \
  -Y -X 0 -A \
  --start 'now -10 hours' --end now \
  "DEF:counts_per_sec=$rrd:power:AVERAGE" \
  "CDEF:power=counts_per_sec,$factor,*" \
  LINE2:power#00FF00:"Power [W]"
