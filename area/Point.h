#ifndef POINT_H
#define POINT_H

// Point class
class Point
{
public:
	Point()
	{
		this->x = 0;
		this->y = 0;
	}

	Point(double x, double y)
	{
		this->x = x;
		this->y = y;
	}
	double x;
	double y;
};

#endif
