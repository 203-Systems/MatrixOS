#include "Color.h"

Color::Color()
{
	W = 0;
	R = 0;
	G = 0;
	B = 0;
}


Color::Color(uint32_t WRGB)
{
	W = (WRGB & 0xFF000000) >> 24;
	R = (WRGB & 0x00FF0000) >> 16;
	G = (WRGB & 0x0000FF00) >> 8;
	B = (WRGB & 0x000000FF);
}


Color::Color(uint8_t nR, uint8_t nG, uint8_t nB, uint8_t nW)
{
	R = nR;
	G = nG;
	B = nB;
	W = nW;
}

uint32_t Color::RGB(uint8_t brightness)
{
	if(brightness != 255)
		return (scale8(R, brightness) << 16) | 
			   (scale8(G, brightness) << 8) | 
			    scale8(B, brightness);
	return (R << 16) | (G << 8) | B;
}

uint32_t Color::GRB(uint8_t brightness)
{
	if(brightness != 255)
		return (scale8(G, brightness) << 16) | 
			   (scale8(R, brightness) << 8) | 
			    scale8(B, brightness);
	return (G << 16) | (R << 8) | B;
}

// uint32_t Color::RGB(uint8_t brightness)
// {
// 	if(brightness != 255)
// 		return (led_gamma[scale8_video(R, brightness)] << 16) | 
// 			   (led_gamma[scale8_video(G, brightness)] << 8) | 
// 			    led_gamma[scale8_video(B, brightness)];
// 	return (led_gamma[R] << 16) | (led_gamma[G] << 8) | led_gamma[B];
// }

// uint32_t Color::GRB(uint8_t brightness)
// {
// 	if(brightness != 255)
// 		return (led_gamma[scale8_video(G, brightness)] << 16) | 
// 			   (led_gamma[scale8_video(R, brightness)] << 8) | 
// 			    led_gamma[scale8_video(B, brightness)];
// 	return (led_gamma[G] << 16) | (led_gamma[R] << 8) | led_gamma[B];
// }

uint8_t Color::scale8(uint8_t i, uint8_t scale)
{
	return ((uint16_t)i * (uint16_t)scale) >> 8;
}

uint8_t Color::scale8_video(uint8_t i, uint8_t scale)
{
	return (((uint16_t)i * (uint16_t)scale) >> 8) + ((i&&scale)?1:0);
}
