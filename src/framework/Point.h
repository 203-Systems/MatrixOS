#pragma once

class Point {
public:
	int16_t x, y;
	Point()
	{
		x = 0;
		y = 0;
	}

	Point(int16_t x, int16_t y) 
	{
		this->x = x;
		this->y = y;
	}

	Point(uint32_t rawByte)
	{
		x = (int16_t)(rawByte >> 16);
		y = (int16_t)(rawByte & 0xFFFF);
	}

	Point operator +(const Point& cp) const //cp stands for compare target
	{
		return Point( x + cp.x, y + cp.y );
	}

    bool operator !=(const Point& cp) const 
    {
        return cp.x != x || cp.y != y;
    }

	bool operator<(const Point& cp) const 
	{
		return x < cp.x || (x == cp.x && y < cp.y);
	}

	operator bool() { return x != INT16_MIN && y != INT16_MIN; }

	operator uint32_t() { return (uint32_t)(x << 16 & y); }

};