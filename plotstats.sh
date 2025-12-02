#!/usr/bin/gnuplot

set key below
set style data points
set style line 100 lw 2 
set y2tics
set ytics nomirror
set ylabel "has ply \\& eval"
set y2label "ply \\& pc \\& mob"
set format x "%.0s%c"
set autoscale xfix
#set xrange [50000000000:*]
set terminal png
set output "stats.png"

plot "stats.txt" \
     u 1:2 w p pt 0 lc 1 notitle, \
  "" u 1:2 smooth bezier w l lw 2 lc 1 t "has ply", \
  "" u 1:3 axes x1y2 w p pt 0 lc 2 notitle, \
  "" u 1:3 axes x1y2 smooth bezier w l lw 2 lc 2 t "ply", \
  "" u 1:4 w p pt 0 lc 3 notitle, \
  "" u 1:4 smooth bezier w l lw 2 lc 3 t "eval", \
  "" u 1:5 axes x1y2 w p pt 0 lc 4 notitle, \
  "" u 1:5 axes x1y2 smooth bezier w l lw 2 lc 4 t "pc", \
  "" u 1:6 axes x1y2 w p pt 0 lc 5 notitle, \
  "" u 1:6 axes x1y2 smooth bezier w l lw 2 lc 5 t "mob"
