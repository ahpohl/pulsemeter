#!/bin/bash

# PV output
api=212dc3361019148fdb63eb0ba53b8d2dfcc4e2ec
sys_id=67956 # Ilvesheim: 66419, Ilvesheim_test: 67956
url=https://pvoutput.org/service/r2/addstatus.jsp

# rrdtool
rrd=/var/lib/rrdcached/pulse.rrd
rrdcached=unix:/run/rrdcached/rrdcached.sock

# counter resolution
rev_per_kwh=75
step_size=300

# fetch data at current time
#fetch_time=$(date +%H:%M --date '-1 min')
fetch_time=$1
json=$(rrdtool xport --daemon $rrdcached --step $step_size --end $fetch_time --start e-$step_size --json  "DEF:counts=$rrd:energy:LAST" "CDEF:energy_kwh=counts,$rev_per_kwh,/" "CDEF:energy=energy_kwh,1000,*" "DEF:value=$rrd:power:AVERAGE" "CDEF:power=value,48000,*" XPORT:energy XPORT:power)

# calculate energy in Wh
energy=$(echo $json | jq '.data[0][0]')

# calculate power in W
power=$(echo $json | jq '.data[0][1]')

# pulse date and time
epoch=$(echo $json | jq '.meta.end')
date=$(date -d @$epoch +%Y%m%d)
time=$(date -d @$epoch +%H:%M)

# upload
#curl -d "d=$date" -d "t=$time" -d "v3=$energy" -d "v4=$power" -d "c1=1" -H "X-Pvoutput-Apikey: $api" -H "X-Pvoutput-SystemId: $sys_id" $url
echo
echo "Date: $date $time, Energy: $energy Wh, Power: $power W, Sys Id: $sys_id"
