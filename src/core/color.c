#include "../core.h"

Color Brighten(Color color, float amount);

Color Darken(Color color, float amount);

Color Brighter(Color color);

Color Darker(Color color);

Color ColorToGrayscale(Color color);

Color GetColorOfOpositeHue(Color color)
{
	Vector3 hsv = ColorToHSV(color);
	hsv.x = Wrap01(hsv.x + 0.5f);
	Color result = ColorFromHSV(hsv.x, hsv.y, hsv.z);
	result.a = color.a;
	return result;
}

Color ColorFromGrayscale(unsigned char intensity);

Color ColorFromFloatGrayscale(float intensity);

Color ColorFromFloatRgb(float red, float green, float blue);

Color ColorFromFloatRgba(float red, float green, float blue, float alpha);