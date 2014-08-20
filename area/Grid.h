#ifndef GRID_H
#define GRID_H

#include "Circle.h"

class Grid
{
public:
	Grid(uint32_t n, Circle c)
	{
		this->_n = n;
		this->_d = 2.0*c.getR()/n;
	}

	uint32_t getN() const
	{
		return this->_n;
	}

	double getD() const
	{
		return this->_d;
	}

private:
	uint32_t _n;
	double _d;

};

#endif
