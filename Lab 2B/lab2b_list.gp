#! /usr/local/cs/bin/gnuplot
set terminal png
set datafile separator ","

set title "Scalability-1: Throughput of Synchronized Lists"
set xlabel "# of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list w/spin' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'list w/mutex' with linespoints lc rgb 'red'
	

set title "Scalability-2: Per-operation Times for Mutex-Protected List Operations"
set xlabel "# of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Mean Time/Operation(ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'completion time' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'wait-for-lock time' with linespoints lc rgb 'red'
	

set title "Scalability-3: Correct Synchronization of Partitioned Lists"
set logscale x 2
set xrange [0.75:]
set xlabel "Threads"
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Unprotected'with points lc rgb 'green', \
    "< grep 'list-id-s,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Spin-Lock' with points lc rgb 'red', \
    "< grep 'list-id-m,[0-9]*,[0-9]*,4,' lab2b_list.csv" using ($2):($3) \
	title 'Mutex' with points lc rgb 'blue'

set title "Scalability-4: Throughput of Mutex Synchronized Partitioned Lists"
set xlabel "# of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_4.png'

plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb 'blue', \
	 "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb 'green', \
	 "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=16' with linespoints lc rgb 'yellow'
	

set title "Scalability-5: Throughput of Spin-Lock Synchronized Partitioned Lists"
set xlabel "# of Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_5.png'

plot \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb 'blue', \
	 "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb 'yellow', \
	 "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'lists=16' with linespoints lc rgb 'green'

