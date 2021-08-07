#pragma once

class Point {
public:
	int16_t x, y;
	Point()
	{
	}
	Point(int16_t _x, int16_t _y) : 
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