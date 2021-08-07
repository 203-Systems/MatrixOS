#pragma once

#include <stdint.h>

class Color
{
	public:
		uint8_t R = 0;
		uint8_t G = 0;
		uint8_t B = 0;
		uint8_t W = 0;

		Color();
		Color(uint32_t WRGB);
		Color(uint8_t nR, uint8_t nG, uint8_t nB, uint8_t nW = 0);

		uint32_t RGB(uint8_t brightness = 255);
		uint32_t GRB(uint8_t brightness = 255);

		uint8_t scale8(uint8_t i, uint8_t scale);
		uint8_t scale8_video(uint8_t i, uint8_t scale);
};
