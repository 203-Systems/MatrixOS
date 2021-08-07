#pragma once

class Point {
public:
	uint8_t x, y;
	Point()
	{
	}
	Point(uint8_t _x, uint8_t _y) : 
		x(_x), y(_y)
	{
	}
	Point operator +(const Point& cp)
	{
		return Point( x + cp.x, y + cp.y );
	}
    bool operator !=(const Point& cp)
    {
        return cp.x != x || cp.y != y;
    }
};