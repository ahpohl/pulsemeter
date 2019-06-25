#!/bin/bash

# PV output
api=212dc3361019148fdb63eb0ba53b8d2dfcc4e2ec
sys_id=66419 # Ilvesheim: 66419, Ilvesheim_test: 67956
url=https://pvoutput.org/service/r2/addstatus.jsp

# rrdtool
rrd=/var/lib/rrdcached/pulse.rrd
rrdcached=unix:/run/rrdcached/rrdcached.sock

# counter resolution
rev_per_kwh=75
step_size=300

# fetch data at current time minus 6 minutes
#fetch_time=$(date +%H:%M --date '-6 min')
fetch_time=$(date +%H:%M --date '@1561495500')

# calculate energy in Wh
energy_json=$(rrdtool xport --daemon $rrdcached --step $step_size --start e-$step_size --end $fetch_time --json  "DEF:counts=$rrd:energy:LAST" "CDEF:energy_kwh=counts,$rev_per_kwh,/" "CDEF:energy=energy_kwh,1000,*" XPORT:energy)
energy=$(echo $energy_json | jq '.data[0][0]')

# calculate power in W
power_json=$(rrdtool xport --daemon $rrdcached --step $step_size --start e-$step_size --end $fetch_time --json "DEF:value=$rrd:power:AVERAGE" "CDEF:power=value,48000,*" XPORT:power)
power=$(echo $power_json | jq '.data[0][0]')

# pulse date and time
epoch=$(echo $power_json | jq '.meta.end')
date=$(date -d @$epoch +%Y%m%d)
time=$(date -d @$epoch +%H:%M)

# upload
#curl -d "d=$date" -d "t=$time" -d "v3=$energy" -d "v4=$power" -d "c1=1" -H "X-Pvoutput-Apikey: $api" -H "X-Pvoutput-SystemId: $sys_id" $url
echo
echo "Date: $date $time, Energy: $energy Wh, Power: $power W, Sys Id: $sys_id"
