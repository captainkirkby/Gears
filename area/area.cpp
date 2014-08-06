// Calculate the area of a circle overlapped by a notch
// Dylan Kirkby 8/4/14

// Coordinate System: origin at top of notch
//
//	+-----> +x
//	|
//	|
//	| +y
//
//

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib> 
#include <ctime>
#include <csignal>

#include "Notch.h"
#include "Grid.h"
#include "Circle.h"
#include "Point.h"

#define BATCH_MODE 0
#define ERROR_MODE 1
#define NORMAL_MODE 2

uint8_t MODE = BATCH_MODE;

static const double pi = 4*std::atan(1);
static const double piHalves = 2*std::atan(1);

bool doubleRatio = true;
bool monteCarlo = true;

uint16_t status = 0;
uint16_t ysteps = 21;

/* A random number generator with a period of about 2 x 10^19 */

unsigned long long int v;

unsigned long long int64() {
    v ^= v >> 21; v ^= v << 35; v ^= v >> 4;
    return v*2685821657736338717LL;
}

double mersenneRandom() {
    return 5.42101086242752217e-20*int64();
}

void setSeed(unsigned long long int j) {
    v = 4101842887655102017LL;
    v ^= j;
    v = int64();
}

// Function prototypes
double getFractionalArea(Grid grid, Circle circle, Notch notch);
double getFractionalAreaGrid(Grid grid, Circle circle, Notch notch);
double getFractionalAreaMonteCarlo(Grid grid, Circle circle, Notch notch);
double deg2rad(double degrees);

uint8_t calculateError(Grid grid, Notch notch);

void printDYDFForRange(double ystart, double yrange, double ysteps, Grid grid, Circle circle, Notch notch);
void printDYDF(double y1, double y2, Grid grid, Circle circle, Notch notch);
void printCurve(double xstart, double xrange, double xsteps, Grid grid, Circle circle, Notch notch);
void printCurvesForRange(double ystart, double yrange, double ysteps, Grid grid, Circle circle, Notch notch);

static void catch_function(int signo);


int main(int argc, char const *argv[])
{
	// Deal with command line arguments
	if(argc > 1){
		if(*argv[1] == 'b') MODE = BATCH_MODE;
		else if(*argv[1] == 'e') MODE = ERROR_MODE;
		else if(*argv[1] == 'n') MODE = NORMAL_MODE;
	} else {
		MODE = NORMAL_MODE;
	}

	// Handle Signals
	std::signal(SIGINFO, catch_function);

	// Initialize random number generator
	setSeed(24071966);

	// Circle Parameters
	Circle circle(0.001,Point(0,0));

	// Grid Parameters
	Grid grid(10000,circle);
	
	// Notch Parameters (vertex @ origin)
	Notch notch(deg2rad(10.0));


	// doubleRatio = false;
	// printf("Monte Carlo Fractional Area Single: %.18f\n",getFractionalAreaMonteCarlo(grid,circle,notch));
	// printf("Grid Fractional Area Single: %.18f\n",getFractionalArea(grid,circle,notch));
	// doubleRatio = true;
	// printf("Monte Carlo Fractional Area Double: %.18f\n",getFractionalAreaMonteCarlo(grid,circle,notch));
	// printf("Grid Fractional Area Double: %.18f\n",getFractionalArea(grid,circle,notch));
	// printf("Expected: %.8f\n\n",(1.0/18.0));


	// return 0;

	Notch squareNotch(deg2rad(90.0));

	Notch notch80Degrees(deg2rad(80.0));
	Notch notch70Degrees(deg2rad(70.0));
	Notch notch60Degrees(deg2rad(60.0));

	Notch notch45Degrees(deg2rad(45.0));
	Notch notch20Degrees(deg2rad(20.0));
	Notch notch10Degrees(deg2rad(10.0));
	

	if(calculateError(grid,notch)) return 0;

	// double yOptimum = (circle.getR()/(2*tan(notch.getAngle())));
	double yOptimum = 0;

	// printCurvesForRange(yOptimum,0.00002,3,grid,circle,notch);
	printDYDFForRange(yOptimum,0.0008,ysteps,grid,circle,squareNotch);

	// printDYDFForRange((circle.getR()/(2*tan(notch80Degrees.getAngle()))),0.0008,101.0,grid,circle,notch80Degrees);
	// printDYDFForRange((circle.getR()/(2*tan(notch70Degrees.getAngle()))),0.0008,101.0,grid,circle,notch70Degrees);
	// printDYDFForRange((circle.getR()/(2*tan(notch60Degrees.getAngle()))),0.0008,101.0,grid,circle,notch60Degrees);

	// printDYDFForRange((circle.getR()/(2*tan(notch45Degrees.getAngle()))),0.0008,101.0,grid,circle,notch45Degrees);
	// printDYDFForRange((circle.getR()/(2*tan(notch20Degrees.getAngle()))),0.0008,101.0,grid,circle,notch20Degrees);
	// printDYDFForRange((circle.getR()/(2*tan(notch10Degrees.getAngle()))),0.0008,101.0,grid,circle,notch10Degrees);

	return 0;
}

void printDYDFForRange(double ystart, double yrange, double ysteps, Grid grid, Circle circle, Notch notch)
{
	double iy;

	for (iy = 1; iy < ysteps; ++iy)
	{
		double yc1 = ystart + yrange*((iy-1)/ysteps);
		double yc2 = ystart + yrange*(iy/ysteps);

		clock_t start_s,finish_s;
		start_s = std::clock();

		++status;
	
		// Calculate curve
		printDYDF(yc1,yc2,grid,circle,notch);

		if(MODE != BATCH_MODE){
			std::printf("Finished y=%.3fmm*************************\n", ((yc1+yc2)/2)*1000);

			finish_s = std::clock();
			std::printf("Cycles: %lu\nTime: %.3f sec\n\n", (finish_s - start_s), ((finish_s - start_s)/((double) CLOCKS_PER_SEC)));
		}
	}
}

// Prints in micrometers
void printDYDF(double y1, double y2, Grid grid, Circle circle, Notch notch)
{
	circle.setX(0);
	circle.setY(y1);
	double fractionalArea1 = getFractionalArea(grid,circle,notch);
	circle.setY(y2);
	double fractionalArea2 = getFractionalArea(grid,circle,notch);

	if(MODE != BATCH_MODE) std::printf("y1: %.10f f1: %.10f\n",y1,fractionalArea1);
	if(MODE != BATCH_MODE) std::printf("y2: %.10f f2: %.10f\n",y2,fractionalArea2);
	if(MODE != BATCH_MODE) std::printf("dy: %.10f df: %.10f\n",std::fabs(y2-y1),std::fabs(fractionalArea2-fractionalArea1));
	if(MODE != BATCH_MODE) std::printf("Grid d: %.10f\n",grid.getD());



	std::printf("%.16f\n",(1000*1000*std::fabs(y2-y1)/(std::fabs(fractionalArea2-fractionalArea1)*1024.0)));
}

void printCurvesForRange(double ystart, double yrange, double ysteps, Grid grid, Circle circle, Notch notch)
{
	double iy;

	for (iy = 0; iy < ysteps; ++iy)
	{
		double yc = ystart + yrange*(iy/ysteps);
		circle.setY(yc);

		clock_t start_s,finish_s;
		start_s = std::clock();
	
		// Calculate curve
		printCurve(0,0.1*circle.getR(),1,grid,circle,notch);

		if(MODE != BATCH_MODE){
			std::printf("Finished y=%.3fmm*************************\n", yc*1000);

			finish_s = std::clock();
			std::printf("Cycles: %lu\nTime: %.3f sec\n\n", (finish_s - start_s), ((finish_s - start_s)/((double) CLOCKS_PER_SEC)));		
		}

	}
}

void printCurve(double xstart, double xrange, double xsteps, Grid grid, Circle circle, Notch notch)
{
	double ix;

	for (ix = 0; ix < xsteps; ++ix)
	{
		double xc = xstart + xrange*(ix/xsteps);
		circle.setX(xc);

		clock_t start_s,finish_s;
		start_s = std::clock();

		// Calculate area
		double fractionalArea = getFractionalArea(grid,circle,notch);
	

		if(MODE != BATCH_MODE){
			finish_s = std::clock();
			std::printf("Cycles: %lu\nTime: %.3f sec\n\n", (finish_s - start_s), ((finish_s - start_s)/((double) CLOCKS_PER_SEC)));
		} else {
			std::printf("%.16f\n",fractionalArea);
		}
	}
}

// Assume for accuracy that theta is 10 degrees
uint8_t calculateError(Grid grid, Notch notch)
{
	if(MODE != BATCH_MODE){
		// Percent Error
		Circle errorCircle(0.001);

		if(MODE == ERROR_MODE){
			uint32_t errorN;
	
			for(errorN = 101; errorN < 100000; errorN*=1.1){

				clock_t start_s,finish_s;
				start_s = std::clock();

				Grid errorGrid(errorN, errorCircle);

				double errorFractionalArea = getFractionalArea(errorGrid,errorCircle,notch);
	
				finish_s = std::clock();
				std::printf("Error: %.8fppm\tTime: %.3f\tn=%u\n\n",fabs((errorFractionalArea*pi/notch.getAngle()-1)*1000000), 
					((finish_s - start_s)/((double) CLOCKS_PER_SEC)), errorGrid.getN());
			}
			return 1;
		} else {
			double errorFractionalArea = getFractionalArea(grid,errorCircle,notch);
			printf("Error: %.8fppm\tn=%u\n\n",fabs((errorFractionalArea*pi/notch.getAngle()-1)*1000000), grid.getN());
		}
	}
	return 0;
}

double getFractionalArea(Grid grid, Circle circle, Notch notch)
{
	return((monteCarlo)?(getFractionalAreaMonteCarlo(grid,circle,notch)):(getFractionalAreaGrid(grid,circle,notch)));
}

double getFractionalAreaGrid(Grid grid, Circle circle, Notch notch)
{
	uint64_t areaCircle = 0;
	uint64_t areaCircleAndNotch = 0;

	uint32_t ix;
	uint32_t iy;

	double x = circle.getX();
	double y = circle.getY();
	double r = circle.getR();
	double d = grid.getD();

	const uint32_t n = grid.getN();

	for (ix = 0; ix < n; ++ix)
	{
		Point p(0,0);
		for (iy = 0; iy < n; ++iy)
		{
			p.x = x - r + ix*d + d/2.0;
			p.y = y - r + iy*d + d/2.0;
	
			if(circle.inCircle(p)){
				++areaCircle;
				if(notch.inNotch(p)){
					++areaCircleAndNotch;
				}
			}
		}
	}

	// if(MODE != BATCH_MODE) printf("Number of grids: %" PRIu64 "\n", area);
	
	if(doubleRatio){
		return ((double) areaCircleAndNotch)/(areaCircle);
	} else {
		return ((double)areaCircleAndNotch)*d*d/(pi*circle.getRSq());
	}
}

double getFractionalAreaMonteCarlo(Grid grid, Circle circle, Notch notch)
{
	uint64_t areaCircle = 0;
	uint64_t areaCircleAndNotch = 0;

	uint32_t ix;
	uint32_t iy;

	double x = circle.getX();
	double y = circle.getY();
	double r = circle.getR();

	const uint32_t n = grid.getN();

	for (ix = 0; ix < n; ++ix)
	{
		Point p(0,0);
		for (iy = 0; iy < n; ++iy)
		{
			p.x = x - r + mersenneRandom()*2*r;
			p.y = y - r + mersenneRandom()*2*r;
	
			if(circle.inCircle(p)){
				++areaCircle;
				if(notch.inNotch(p)){
					++areaCircleAndNotch;
				}
			}
		}
	}
	
	if(doubleRatio){
		return ((double) areaCircleAndNotch)/(areaCircle);
	} else {
		return ((double) areaCircleAndNotch*4)/(pi*n*n);
	}
}

double deg2rad(double degrees)
{
	return (degrees/180.0)*pi;
}

static void catch_function(int signo) {
    std::fprintf(stderr, "%i/%i\n", status, ysteps);
}




