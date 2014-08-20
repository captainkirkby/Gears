# Area makefile
# Dylan Kirkby 8/5/14
#
# make graph
#
# g++-4.9

CC						= clang++
CFLAGS					= -lm -Wall -std=c++11 -o area.out
NUM_GRAPHS				= 2
POINTS_PER_GRAPH		= 2048
ONE						= 1
POINTS_MINUS_ONE		= $(shell echo $(POINTS_PER_GRAPH)-$(ONE) | bc)
NUM_GRAPHS_MINUS_ONE	= $(shell echo $(NUM_GRAPHS)-$(ONE) | bc)


compile:
	$(CC) area.cpp $(CFLAGS) 

run: area.out
	./area.out

dydf: compile
	./area.out d > temp.dat \
	&& gnuplot -geometry 500x500 -p -e 'set size square; set xrange [-0.0015:0.0015]; set yrange [0:5]; \
	plot for [t=0:$(NUM_GRAPHS_MINUS_ONE)] "temp.dat" using 2:1 \
	every ::(t*$(POINTS_PER_GRAPH))::(t*$(POINTS_PER_GRAPH)+$(POINTS_MINUS_ONE)) with lines title "".t'

graph: compile
	./area.out g > temp.dat \
	&& gnuplot -geometry 700x700 -p -e 'set size square; set xrange [-0.006:0.006]; set yrange [-0.001:0.01]; plot  "temp.dat" using 1:2 title "" '

shape: compile
	./area.out s > temp.dat \
	&& gnuplot -geometry 700x700 -p -e 'set size square; set xrange [-0.030:0.030]; set yrange [-0.015:0.01] reverse; plot  "temp.dat" using 1:2 title "" '

curve: compile
	./area.out c > temp.dat \
	&& gnuplot -geometry 700x700 -p -e 'set size square; plot "temp.dat" using 1:2 with lines title "" '

heights: compile
	./area.out h > temp.dat \
	&& gnuplot -geometry 700x700 -p -e 'set size square; plot for [t=0:$(NUM_GRAPHS_MINUS_ONE)] "temp.dat" using 1:2 \
	every ::(t*$(POINTS_PER_GRAPH))::(t*$(POINTS_PER_GRAPH)+$(POINTS_MINUS_ONE)) with lines title "".t'

velocities: compile
	./area.out v > temp.dat \
	&& gnuplot -geometry 700x700 -p -e 'set size square; plot for [t=0:$(NUM_GRAPHS_MINUS_ONE)] "temp.dat" using 1:2 \
	every ::(t*$(POINTS_PER_GRAPH))::(t*$(POINTS_PER_GRAPH)+$(POINTS_MINUS_ONE)) with lines title "".t'

last: temp.dat
	gnuplot -geometry 700x700 -p -e 'set size square; plot for [t=0:$(NUM_GRAPHS_MINUS_ONE)] "temp.dat" using 1:2 \
	every ::(t*$(POINTS_PER_GRAPH))::(t*$(POINTS_PER_GRAPH)+$(POINTS_MINUS_ONE)) with lines title "".t'
