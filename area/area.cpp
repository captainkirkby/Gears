// Calculate the area of a circle overlapped by a notch
// Dylan Kirkby 8/4/14

// Coordinate System: origin at top of notch
//
//	+-----> +x
//	|
//	|
//	| +y
//
//  e.g.
// 	printDYDFForRange((circle.getR()/(2*tan(notch80Degrees.getAngle()))),0.0008,yStepsPerRange,grid,circle,notch80Degrees);
//

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib> 
#include <ctime>
#include <csignal>

#include <vector>

#include "Notch.h"
#include "Grid.h"
#include "Circle.h"
#include "Point.h"
#include "Polygon.h"
#include "Fingers.h"

#define BATCH_MODE 0
#define ERROR_MODE 1
#define NORMAL_MODE 2
#define GRAPH_MODE 3


uint8_t MODE = BATCH_MODE;

static const double pi = 4*std::atan(1);
static const double piHalves = 2*std::atan(1);

bool doubleRatio = true;
bool monteCarlo = true;

uint16_t status = 0;
uint16_t maxSteps = 0;


/* A random number generator with a period of about 2 x 10^19 */

unsigned long long int v;

unsigned long long int64() {
    v ^= v >> 21; v ^= v << 35; v ^= v >> 4;
    return v*2685821657736338717LL;
}

double randomDouble() {
    return 5.42101086242752217e-20*int64();
}

void setSeed(unsigned long long int j) {
    v = 4101842887655102017LL;
    v ^= j;
    v = int64();
}

// Function prototypes
double getFractionalArea(const Grid &grid, Circle &circle, const Polygon &notch);
double getFractionalAreaGrid(const Grid &grid, Circle &circle, const Polygon &notch);
double getFractionalAreaMonteCarlo(const Grid &grid, Circle &circle, const Polygon &notch);
double deg2rad(double degrees);

uint8_t calculateError(const Grid &grid, const Notch &notch);

void printDYDFForRange(double ystart, double yrange, double ysteps, const Grid &grid, Circle &circle, const Polygon &notch);
void printCurvesForRange(double ystart, double yrange, double ysteps, const Grid &grid, Circle &circle, const Polygon &notch);
void printCurvesForRange(double ystart, double yrange, double ysteps, double xstart, double xrange, double xsteps, 
	const Grid &grid, Circle &circle, const Polygon &notch);
void printDYDF(double y1, double y2, const Grid &grid, Circle &circle, const Polygon &notch);
void printCurve(double xstart, double xrange, double xsteps, const Grid &grid, Circle &circle, const Polygon &notch);
void printCurve(uint32_t totalSamples, const Grid &grid, Circle &circle, const Polygon &notch);

static void catch_function(int signo);

double yForCircle(double x);
double initialVelocity();
double acceleration();



int main(int argc, char *argv[])
{
	clock_t start,finish;
	start = clock();

	// Handle Signals
	std::signal(SIGINFO, catch_function);

	// Initialize random number generator
	setSeed(24071966);

	// Circle Parameters
	Circle circle(0.001,Point(0,0));

	// Grid Parameters
	Grid grid(1000,circle);

	// Notch Parameters
	Notch notch(deg2rad(90));

	// Deal with command line arguments
	if(argc > 1){
		if(*argv[1] == 'b') {
			MODE = BATCH_MODE;
		}
		else if(*argv[1] == 'd') {

			MODE = BATCH_MODE;

			// Parameters of test
			uint16_t yStepsPerRange = 41;
			const uint8_t numDegreesToTest = 10;
			double degreesToTest[numDegreesToTest] = { 90.0, 80.0, 70.0, 60.0 , 50.0, 45.0, 40.0, 30.0, 20.0, 10.0};

			maxSteps += yStepsPerRange*numDegreesToTest;
			
			uint8_t i;	
			for(i = 0; i < numDegreesToTest; ++i)
			{
				Notch testNotch(deg2rad(degreesToTest[i]));
				printDYDFForRange(-0.0015,0.0030,yStepsPerRange,grid,circle,testNotch);
			}
		
			return 0;
		}
		else if(*argv[1] == 'e') {
			MODE = ERROR_MODE;
			calculateError(grid, notch);
		}
		else if(*argv[1] == 'c') {
			MODE = BATCH_MODE;

			double a = deg2rad(90.0);

			std::vector<Notch> v { 
				Notch(a,Point(-0.016,0.01)),
				Notch(a,Point(-0.008,0.01)),
				Notch(a,Point(0.00,0.00)),
				Notch(a,Point(0.012,0.01),0.006)
			};
			Fingers fingers(v);
			printCurve(2048,grid,circle,fingers);
		}
		else if(*argv[1] == 'h') {
			MODE = BATCH_MODE;

			double a = deg2rad(90.0);

			std::vector<Notch> v { 
				Notch(a,Point(-0.016,0.01)),
				Notch(a,Point(-0.008,0.01)),
				Notch(a,Point(0.00,0.00)),
				Notch(a,Point(0.012,0.01),0.006)
			};
			Fingers fingers(v);
			printCurvesForRange(-0.000005,0.00001,10,-0.025,0.050,200,grid,circle,fingers);
		}
		else if(*argv[1] == 'v') {
			MODE = BATCH_MODE;

			double a = deg2rad(90.0);

			std::vector<Notch> v { 
				Notch(a,Point(-0.016,0.01)),
				Notch(a,Point(-0.008,0.01)),
				Notch(a,Point(0.00,0.00)),
				Notch(a,Point(0.012,0.01),0.006)
			};
			Fingers fingers(v);
			printCurvesForRange(-0.000005,0.00001,10,-0.025,0.050,200,grid,circle,fingers);
		}
		else if(*argv[1] == 's') {
			MODE = GRAPH_MODE;

			double a = deg2rad(90.0);

			std::vector<Notch> v { 
				Notch(a,Point(-0.016,0.01)),
				Notch(a,Point(-0.008,0.01)),
				Notch(a,Point(0.00,0.00)),
				Notch(a,Point(0.012,0.01),0.006)
			};
			Fingers fingers(v);

			// Circle Parameters
			Circle shapeCircle(0.043,Point(0,0));

			// Grid Parameters
			Grid shapeGrid(1000,shapeCircle);

			getFractionalAreaMonteCarlo(shapeGrid,shapeCircle,fingers);
		} else {
			MODE = NORMAL_MODE;
		}

	}

	finish = clock();
    std::fprintf(stderr, "Time Elapsed: %.3f\n",((finish - start)/((double) CLOCKS_PER_SEC)));
	return 0;
}

void printDYDFForRange(double ystart, double yrange, double ysteps, const Grid &grid, Circle &circle, const Polygon &notch)
{
	double iy;

	for (iy = 0; iy <= ysteps; ++iy)
	{
		double yc1 = ystart + yrange*(iy/ysteps);
		double yc2 = ystart + yrange*((iy+1)/ysteps);

		clock_t start_s,finish_s;
		start_s = std::clock();

		++status;
	
		// Calculate curve
		printDYDF(yc1,yc2,grid,circle,notch);

		if(MODE == NORMAL_MODE){
			std::printf("Finished y=%.3fmm*************************\n", ((yc1+yc2)/2)*1000);

			finish_s = std::clock();
			std::printf("Cycles: %lu\nTime: %.3f sec\n\n", (finish_s - start_s), ((finish_s - start_s)/((double) CLOCKS_PER_SEC)));
		}
	}
}

// Prints in micrometers
void printDYDF(double y1, double y2, const Grid &grid, Circle &circle, const Polygon &notch)
{
	circle.setX(0);
	circle.setY(y1);
	double fractionalArea1 = getFractionalArea(grid,circle,notch);
	circle.setY(y2);
	double fractionalArea2 = getFractionalArea(grid,circle,notch);

	if(MODE == NORMAL_MODE) {
		std::printf("y1: %.10f f1: %.10f\n",y1,fractionalArea1);
		std::printf("y2: %.10f f2: %.10f\n",y2,fractionalArea2);
		std::printf("dy: %.10f df: %.10f\n",std::fabs(y2-y1),std::fabs(fractionalArea2-fractionalArea1));
		std::printf("Grid d: %.10f\n",grid.getD());
	} else if(MODE == BATCH_MODE) {
		std::printf("%.16f %.16f\n",(1000*1000*std::fabs(y2-y1)/(std::fabs(fractionalArea2-fractionalArea1)*1024.0)),(y2+y1)/2);
	}
}

void printCurvesForRange(double ystart, double yrange, double ysteps, double xstart, double xrange, 
	double xsteps, const Grid &grid, Circle &circle, const Polygon &notch)
{
	double iy;

	for (iy = 0; iy <= ysteps; ++iy)
	{
		double yc = ystart + yrange*(iy/ysteps);
		circle.setY(yc);

		clock_t start_s,finish_s;
		start_s = std::clock();

		++status;
	
		// Calculate curve
		printCurve(xstart,xrange,xsteps,grid,circle,notch);

		if(MODE == NORMAL_MODE){
			std::printf("Finished y=%.3fmm*************************\n", yc*1000);

			finish_s = std::clock();
			std::printf("Cycles: %lu\nTime: %.3f sec\n\n", (finish_s - start_s), ((finish_s - start_s)/((double) CLOCKS_PER_SEC)));		
		}

	}
}


void printCurvesForRange(double ystart, double yrange, double ysteps, const Grid &grid, Circle &circle, const Polygon &notch)
{
	double iy;

	for (iy = 0; iy <= ysteps; ++iy)
	{
		double yc = ystart + yrange*(iy/ysteps);
		circle.setY(yc);

		clock_t start_s,finish_s;
		start_s = std::clock();

		++status;
	
		// Calculate curve
		printCurve(0,circle.getR(),ysteps,grid,circle,notch);

		if(MODE == NORMAL_MODE){
			std::printf("Finished y=%.3fmm*************************\n", yc*1000);

			finish_s = std::clock();
			std::printf("Cycles: %lu\nTime: %.3f sec\n\n", (finish_s - start_s), ((finish_s - start_s)/((double) CLOCKS_PER_SEC)));		
		}

	}
}

void printCurve(double xstart, double xrange, double xsteps, const Grid &grid, Circle &circle, const Polygon &notch)
{
	double ix;

	for (ix = 0; ix <= xsteps; ++ix)
	{
		double xc = xstart + xrange*(ix/xsteps);
		circle.setX(xc);

		clock_t start_s,finish_s;
		start_s = std::clock();

		// Calculate area
		double fractionalArea = getFractionalArea(grid,circle,notch);
	

		if(MODE == NORMAL_MODE){
			finish_s = std::clock();
			std::printf("Cycles: %lu\nTime: %.3f sec\n\n", (finish_s - start_s), ((finish_s - start_s)/((double) CLOCKS_PER_SEC)));
		} else if(MODE == BATCH_MODE) {
			std::printf("%.16f %.16f\n",xc,fractionalArea);
		}
	}
}

// Samples should be an even number
void printCurve(uint32_t totalSamples, const Grid &grid, Circle &circle, const Polygon &notch)
{
	uint32_t ix;
	int32_t sample;
	double secondsPerSample = 6.400e-6;			// (10MHz/64)^-1
	double startingY = circle.getY();
	if(totalSamples%2)++totalSamples;		// Make sure samples is even

	for (ix = 0; ix <= totalSamples; ++ix)
	{
		sample = (totalSamples*-0.5+ix);
		double t = (sample*secondsPerSample);
		double xc = initialVelocity()*t + 0.5*acceleration()*t*t; 
		circle.setX(xc);
		circle.setY(startingY + yForCircle(xc));

		clock_t start_s,finish_s;
		start_s = std::clock();

		// Calculate area
		double fractionalArea = getFractionalArea(grid,circle,notch);
	

		if(MODE == NORMAL_MODE){
			finish_s = std::clock();
			std::printf("Cycles: %lu\nTime: %.3f sec\n\n", (finish_s - start_s), ((finish_s - start_s)/((double) CLOCKS_PER_SEC)));
		} else if(MODE == BATCH_MODE) {
			std::printf("%i %.16f\n",ix,fractionalArea);
		}
	}
}

double yForCircle(double x)
{
	return std::sqrt(1 - x*x) - 1;
}

// In meters per second
double initialVelocity()
{
	return 3.639;			// Measured: 3.639 m/s
}

double acceleration()
{
	return 0.0;
}

// Assume for accuracy that theta is 10 degrees
uint8_t calculateError(const Grid &grid, const Notch &notch)
{
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
	return 0;
}

double getFractionalArea(const Grid &grid, Circle &circle, const Polygon &notch)
{
	return((monteCarlo)?(getFractionalAreaMonteCarlo(grid,circle,notch)):(getFractionalAreaGrid(grid,circle,notch)));
}

double getFractionalAreaGrid(const Grid &grid, Circle &circle, const Polygon &notch)
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

	// if(MODE == NORMAL_MODE) printf("Number of grids: %" PRIu64 "\n", area);
	
	if(doubleRatio){
		return ((double) areaCircleAndNotch)/(areaCircle);
	} else {
		return ((double)areaCircleAndNotch)*d*d/(pi*circle.getRSq());
	}
}

double getFractionalAreaMonteCarlo(const Grid &grid, Circle &circle, const Polygon &notch)
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
			p.x = x - r + randomDouble()*2*r;
			p.y = y - r + randomDouble()*2*r;
	
			if(circle.inCircle(p)){
				++areaCircle;
				if(notch.inNotch(p)){
					++areaCircleAndNotch;
					if(MODE == GRAPH_MODE) printf("%.8f %.8f\n", p.x, p.y);
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
    std::fprintf(stderr, "%i/%i\n", status, maxSteps);
}




