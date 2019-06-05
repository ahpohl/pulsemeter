#!/bin/bash
# energy is stored as counts
# power is stored as counts/s

# revolutions per kWh of ferraris disk
rev_per_kwh=75

# parameters
rrd=/scratch/git/pulse/pulse.rrd

# factor: 3600*1000/75
factor=48000
# convert counts/s to power in W
rrdtool xport \
  --start 'now -8 hours' --end now --showtime --step 300 \
  "DEF:counts_per_sec=$rrd:power:AVERAGE" \
  "CDEF:power=counts_per_sec,$factor,*" \
  "XPORT:power"
