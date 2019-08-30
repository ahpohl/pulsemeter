set datafile separator ","
set terminal png size 900,400
set title "Sensor reading with low and high thresholds"
set xlabel "Time (sec)"
set ylabel "Raw value"
set nokey
set grid
#set mxtics
#set xrange [0:100]
set arrow from graph 0,first 95 to graph 1,first 95 nohead lw 2 lt 0 lc rgb "blue" front
set arrow from graph 0,first 110 to graph 1,first 110 nohead lw 2 lt 0 lc rgb "red" front
set label "high" at graph 0.01,first 112 left tc rgb "red"
set label "low" at graph 0.01,first 97 left tc rgb "blue"
plot "sensor.log" using ($1/1000):2 with lines lw 2 lt 2 lc rgb "black" title 'raw'
