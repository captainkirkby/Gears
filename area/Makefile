# Area makefile
# Dylan Kirkby 8/5/14
#
# make graph
#
# g++-4.9

CC					= clang++
CFLAGS				= -lm -Wall -fopenmp -o area
NUM_GRAPHS			= 1
POINTS_PER_GRAPH	= 20
ONE					= 1
POINTS_MINUS_ONE	= $(shell echo $(POINTS_PER_GRAPH)-$(ONE) | bc)

compile:
	$(CC) area.cpp $(CFLAGS) 

run: area
	./area

batch: area
	./area b

graph: area
	./area b > temp.dat \
	&& gnuplot -p -e 'plot for [t=0:$(NUM_GRAPHS)] "temp.dat" \
	every ::(t*$(POINTS_PER_GRAPH))::(t*$(POINTS_PER_GRAPH)+$(POINTS_MINUS_ONE)) with lines title "".t' \

last:
	gnuplot -p -e 'plot for [t=0:$(NUM_GRAPHS)] "temp.dat" \
	every ::(t*$(POINTS_PER_GRAPH))::(t*$(POINTS_PER_GRAPH)+$(POINTS_MINUS_ONE)) with lines title "".t'
