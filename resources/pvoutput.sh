#/bin/bash

# PV output
api=212dc3361019148fdb63eb0ba53b8d2dfcc4e2ec
sys_id=67956 # Ilvesheim: 66419
url=https://pvoutput.org/service/r2/addstatus.jsp

# rrdtool
rrd=/var/lib/rrdcached/pulse.rrd
rrdcached=unix:/run/rrdcached/rrdcached.sock

# pulse
rev_per_kwh=75
counts=$(rrdtool lastupdate $rrd --daemon $rrdcached | tail -n 1 | cut -f2 -d " ")
energy=$(echo "scale=3; $counts/$rev_per_kwh*1000" | bc)

# pulse date and time
epoch=$(rrdtool lastupdate $rrd --daemon $rrdcached | tail -n 1 | cut -f1 -d ":")
date=$(date -d @$epoch +%Y%m%d)
time=$(date -d @$epoch +%H:%M)

# upload
curl -d "d=$date" -d "t=$time" -d "v3=$energy" -d "c1=1" -H "X-Pvoutput-Apikey: $api" -H "X-Pvoutput-SystemId: $sys_id" $url
echo "\n"

