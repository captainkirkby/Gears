// C++ version of an interface/protocol
// Dylan Kirkby
// 8/11/14

#ifndef POLYGON_H
#define POLYGON_H

class Polygon
{
public:
	// Subclasses are required by law to implement this method
    virtual bool inNotch(Point p) const = 0;
    virtual ~Polygon(){};
};

#endif
