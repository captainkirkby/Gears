#ifndef CIRCLE_H
#define CIRCLE_H

#include "Point.h"

// Circle Class
class Circle
{
public:
	Circle(double r, Point c)
	{
		this->_r = r;
		this->_c = c;
		this->_rSq = r*r;
	}

	Circle(double r)
	{
		this->_r = r;
		this->_c = Point();
		this->_rSq = r*r;
	}

	bool inCircle(Point p)
	{
		// Is the distance from the point to the center of the circle less than the radius
		return (p.x-this->_c.x)*(p.x-this->_c.x) + (p.y-this->_c.y)*(p.y-this->_c.y) < this->_rSq;
	}


	double getR() const
	{
		return this->_r;
	}

	double getRSq() const
	{
		return this->_rSq;
	}

	Point getC() const
	{
		return this->_c;
	}

	double getX() const
	{
		return this->_c.x;
	}

	double getY() const
	{
		return this->_c.y;
	}

	void setX(double x)
	{
		this->_c.x = x;
	}

	void setY(double y)
	{
		this->_c.y = y;
	}

private:
	double _r;
	double _rSq;
	Point _c;
};

#endif
