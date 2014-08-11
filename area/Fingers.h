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
		this->_notches;
	}

private:
	std::vector<Notch> _notches;
};

#endif