#!/bin/bash

# PV output
api=212dc3361019148fdb63eb0ba53b8d2dfcc4e2ec
sys_id=67956 # Ilvesheim: 66419
url=https://pvoutput.org/service/r2/addstatus.jsp

# rrdtool
rrd=/var/lib/rrdcached/pulse.rrd
rrdcached=unix:/run/rrdcached/rrdcached.sock

# counter resolution
rev_per_kwh=75

# fetch data at current time minus 5 minutes
fetch_time=$(date +%H:%M --date '-5 min')
fetch=$(rrdtool fetch /var/lib/rrdcached/pulse.rrd LAST --daemon unix:/run/rrdcached/rrdcached.sock --start $fetch_time --end $fetch_time)

# pulse date and time
epoch=$(echo $fetch | cut -f1 -d ":" | cut -f3 -d " ")
date=$(date -d @$epoch +%Y%m%d)
time=$(date -d @$epoch +%H:%M)

# calculate energy
counts=$(echo $fetch | cut -f4 -d " ")
energy=$(awk 'BEGIN { printf "%.10e", '$counts' / '$rev_per_kwh' * 1000 }')

# calculate power
power=$(echo $fetch | cut -f5 -d " ")

# upload
curl -d "d=$date" -d "t=$time" -d "v3=$energy" -d "v4=$power" -d "c1=1" -H "X-Pvoutput-Apikey: $api" -H "X-Pvoutput-SystemId: $sys_id" $url
echo
