#ifndef NOTCH_H
#define NOTCH_H

#include <cmath>
#include "Point.h"

// Notch Class
class Notch
{
public:
	// static const double DEFAULT_WIDTH = 0.002;	// 2mm
	Notch(double angle)
	{
		this->_halfWidth = 0.002;
		this->_angle = angle;			// In radians
		if(angle != 2*std::atan(1)) {	// Pi halves
			this->_slope = std::tan(angle);
			this->_infSlope = false;
		} else {
			this->_slope = 0;
			this->_infSlope = true;
		}
	}

	bool inNotch(Point p)
	{
		double abs_x = std::fabs(p.x);
		// Is point within the width and inside the angle?
		return (((this->_infSlope)?(p.y >= 0):(abs_x) <= p.y*this->_slope) && abs_x <= this->_halfWidth);
	}

	double getAngle() const
	{
		return this->_angle;
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
};

#endif
