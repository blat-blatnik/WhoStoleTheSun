#include "../core.h"

Color Brighten(Color color, float amount)
{
	Vector3 hsv = ColorToHSV(color);
	hsv.z = Clamp01(hsv.z * amount);
	Color result = ColorFromHSV(hsv.x, hsv.y, hsv.z);
	result.a = color.a;
	return result;
}

Color Darken(Color color, float amount)
{
	return Brighten(color, 1 / amount);
}

Color Brighter(Color color)
{
	return Brighten(color, 1.6f);
}

Color Darker(Color color)
{
	return Darken(color, 1.6f);
}

Color GetColorOfOpositeHue(Color color)
{
	Vector3 hsv = ColorToHSV(color);
	hsv.x = Wrap(hsv.x + 180, 0, 360);
	Color result = ColorFromHSV(hsv.x, hsv.y, hsv.z);
	result.a = color.a;
	return result;
}

Color Grayscale(float intensity)
{
	return FloatRGBA(intensity, intensity, intensity, 1);
}

Color GrayscaleAlpha(float intensity, float alpha)
{
	return FloatRGBA(intensity, intensity, intensity, alpha);
}

Color FloatRGB(float red, float green, float blue)
{
	return FloatRGBA(red, green, blue, 1);
}

Color FloatRGBA(float red, float green, float blue, float alpha)
{
	float r = Clamp01(red);
	float g = Clamp01(green);
	float b = Clamp01(blue);
	float a = Clamp01(alpha);
	return (Color)
	{
		.r = (uint8_t)(r * 255.5f),
		.g = (uint8_t)(g * 255.5f),
		.b = (uint8_t)(b * 255.5f),
		.a = (uint8_t)(a * 255.5f),
	};
}

Color BlendColors(Color c0, Color c1, float t)
{
	float r = Clamp(Lerp((float)c0.r, (float)c1.r, t), 0, 255) + 0.5f;
	float g = Clamp(Lerp((float)c0.g, (float)c1.g, t), 0, 255) + 0.5f;
	float b = Clamp(Lerp((float)c0.b, (float)c1.b, t), 0, 255) + 0.5f;
	float a = Clamp(Lerp((float)c0.a, (float)c1.a, t), 0, 255) + 0.5f;
	return (Color)
	{
		.r = (uint8_t)(r),
		.g = (uint8_t)(g),
		.b = (uint8_t)(b),
		.a = (uint8_t)(a),
	};
}
