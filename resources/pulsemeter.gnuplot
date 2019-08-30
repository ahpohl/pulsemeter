set datafile separator ","
set terminal png size 900,400
set title "Pulse energy meter"
set xdata time
set timefmt "%s"
set format x "%H:%M"
set xlabel "Time (hh:mm)"
set ylabel "Energy (kWh)"
set y2label "Power (W)"
set key top left
set grid
plot "pulsemeter.log" using ($2 + 2*60*60):($4/1000) with lines lw 2 lt 2 lc rgb "red" title 'energy' axis x1y1, "pulsemeter.log" using ($2 + 2*60*60):5 with lines lw 2 lt 2 lc rgb "blue" title 'power' axis x1y2
