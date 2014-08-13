/*
Notch class to represent the notch a beam of light passes through
Dylan Kirkby
8/11/14

Note: stores a center point, but isNotch() is called with the assumption
that the center is at the top/center of the notch.
*/

#ifndef NOTCH_H
#define NOTCH_H

#include <cmath>
#include "Point.h"
#include "Polygon.h"


// Notch Class
class Notch : public Polygon
{
public:

	// Default creates at (0,0)
	Notch(double angle)
	{
		this->_halfWidth = 0.002;
		this->_angle = angle;			// In radians
		Point point(0,0);
		this->_center = point;
		if(angle != 2*std::atan(1)) {	// Pi halves
			this->_slope = std::tan(angle);
			this->_infSlope = false;
		} else {
			this->_slope = 0;
			this->_infSlope = true;
		}
	}

	Notch(double angle, Point p)
	{
		this->_halfWidth = 0.002;
		this->_angle = angle;			// In radians
		this->_center = p;
		if(angle != 2*std::atan(1)) {	// Pi halves
			this->_slope = std::tan(angle);
			this->_infSlope = false;
		} else {
			this->_slope = 0;
			this->_infSlope = true;
		}
	}

	Notch(double angle, Point p, double halfwidth)
	{
		this->_halfWidth = halfwidth;
		this->_angle = angle;			// In radians
		this->_center = p;
		if(angle != 2*std::atan(1)) {	// Pi halves
			this->_slope = std::tan(angle);
			this->_infSlope = false;
		} else {
			this->_slope = 0;
			this->_infSlope = true;
		}
	}

	bool inNotch(Point p) const
	{
		double abs_x = std::fabs(p.x);
		// Is point within the width and inside the angle?
		return (((this->_infSlope)?(p.y >= 0):(abs_x) <= p.y*this->_slope) && abs_x <= this->_halfWidth);
	}

	double getAngle() const
	{
		return this->_angle;
	}

	double getX() const
	{
		return this->_center.x;
	}

	double getY() const
	{
		return this->_center.y;
	}

	void setAngle(double angle)
	{
		this->_angle = angle;
	}

private:
	double _halfWidth;
	double _angle;
	double _slope;
	bool _infSlope;
	Point _center;
};

#endif
