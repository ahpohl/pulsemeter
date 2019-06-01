#!/bin/bash
# energy is stored as counts
# power is stored as counts/s

rrdtool create pulse.rrd \
		--start 1559333640 \
		--step 60 \
        "DS:energy:GAUGE:120:0:U" \
        "DS:power:COUNTER:120:0:48000" \
        "RRA:LAST:0.5:1:4320" \
        "RRA:AVERAGE:0.5:1:4320" \
        "RRA:LAST:0.5:1440:30" \
        "RRA:AVERAGE:0.5:1440:30" \
        "RRA:LAST:0.5:10080:520" \
        "RRA:AVERAGE:0.5:10080:520"

rrdtool update pulse.rrd \
		"1559333660:1:1" \
		"1559333720:2:2" \
		"1559333750:3:3" \
		"1559333800:40:40" \
		"1559333900:50:50" \
		"1559333910:60:60" \
		"1559333920:71:71" \
		"1559333930:72:72" \
		"1559333940:75:75" \
		"1559333950:100:100" \
		"1559333960:110:110" \
		"1559334020:120:120" \
		"1559334080:130:130"

# revolutions per kWh of ferraris disk
rev_per_kwh=75

# convert counts to energy in kWh
rrdtool graph energy.png \
  --start 1559333640 --end 1559334080 \
  "DEF:counts=pulse.rrd:energy:LAST" \
  "CDEF:energy=counts,$rev_per_kwh,/" \
  LINE2:energy#000000:"Meter reading [kWh]"

# factor: 3600*1000/75
factor=48000
# convert counts/s to power in W
rrdtool graph power.png \
  --start 1559333640 --end 1559334080 \
  "DEF:counts_per_sec=pulse.rrd:power:AVERAGE" \
  "CDEF:power=counts_per_sec,$factor,*" \
  LINE2:power#00FF00:"Power [W]"
