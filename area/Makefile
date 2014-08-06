# Area makefile
# Dylan Kirkby 8/5/14
#
# make graph
#
# g++-4.9

CC					= clang++
CFLAGS				= -lm -Wall -fopenmp -o area.out
NUM_GRAPHS			= 1
POINTS_PER_GRAPH	= 20
ONE					= 1
POINTS_MINUS_ONE	= $(shell echo $(POINTS_PER_GRAPH)-$(ONE) | bc)

compile:
	$(CC) area.cpp $(CFLAGS) 

run: area.out
	./area.out

batch: area.out
	./area.out b

graph: area.out
	./area.out b > temp.dat \
	&& gnuplot -p -e 'plot for [t=0:$(NUM_GRAPHS)] "temp.dat" \
	every ::(t*$(POINTS_PER_GRAPH))::(t*$(POINTS_PER_GRAPH)+$(POINTS_MINUS_ONE)) with lines title "".t' \

last: temp.dat
	gnuplot -p -e 'plot for [t=0:$(NUM_GRAPHS)] "temp.dat" \
	every ::(t*$(POINTS_PER_GRAPH))::(t*$(POINTS_PER_GRAPH)+$(POINTS_MINUS_ONE)) with lines title "".t'
