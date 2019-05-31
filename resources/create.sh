#!/bin/sh
rrdtool create pulse.rrd \
		--start 1559333640 \
		--step 60 \
        "DS:energy:COUNTER:3600:0:U" \
        "DS:power:GAUGE:3600:0:U" \
        "RRA:LAST:0.5:1:4320" \
        "RRA:AVERAGE:0.5:1:4320" \
        "RRA:LAST:0.5:1440:30" \
        "RRA:AVERAGE:0.5:1440:30" \
        "RRA:LAST:0.5:10080:520" \
        "RRA:AVERAGE:0.5:10080:520"

rrdtool update pulse.rrd \
		"1559333660:1:1" \
		"1559333720:1:1" \
		"1559333750:1:1" \
		"1559333800:1:1" \
		"1559333900:1:1" \
		"1559333910:1:1" \
		"1559333920:1:1" \
		"1559333930:1:1" \
		"1559333940:1:1" \
		"1559333950:1:1" \
		"1559333960:1:1" \
		"1559334020:1:1" \
		"1559334080:1:1"

rrdtool graph energy.png \
  -s 'now -5 min' -e 'now' \
  -X 0 -Y -A \
  DEF:energy=pulse.rrd:energy:LAST \
  LINE2:energy#000000:"counts"

rrdtool graph power.png \
  -s 'now -30 min' -e 'now' \
  -Y -A \
  DEF:power=pulse.rrd:power:AVERAGE \
  LINE2:power#00FF00:"counts per seconds"
