set terminal png
set output "benchmark.png"

set datafile separator ','

set title "libabr benchmark"

set key bottom right

plot 'benchmark.csv' u ($2*$3):($4/1E6) t "OpenCV",\
     'benchmark.csv' u ($2*$3):($5/1E6) t "libabr (CPU)",\
     'benchmark.csv' u ($2*$3):($7/1E6) t "libabr (CPU/SSE)"

