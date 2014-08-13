#ifndef FINGERS_H
#define FINGERS_H

#include <cmath>
#include <vector>
#include "Point.h"
#include "Notch.h"
#include "Polygon.h"

// Fingers Class
class Fingers : public Polygon
{
public:
	Fingers(std::vector<Notch> notches)
	{
		this->_notches = notches;
	}

	bool inNotch(Point p) const
	{
		for (std::vector<Notch>::const_iterator i = _notches.begin(); i != _notches.end(); ++i)
		{
			Point newP = transformPoint(p, i->getX(), i->getY());
			if(i->inNotch(newP)) return true;
		}
		return false;
	}


private:
	std::vector<Notch> _notches;

	Point transformPoint(const Point point, double x_transform, double y_transform) const
	{
		Point p(point.x + x_transform, point.y + y_transform);
		return p;
	}


};

#endif
