#include "../core.h"

void rlColor(Color color)
{
	rlColor4ub(color.r, color.g, color.b, color.a);
}

void rlVertex2v(Vector2 p)
{
	rlVertex2f(p.x, p.y);
}

void DrawQuad(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Color color)
{
	DrawQuadGradient(p0, p1, p2, p3, color, color, color, color);
}

void DrawQuadGradient(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, Color color0, Color color1, Color color2, Color color3)
{
	rlBegin(RL_QUADS);
	{
		rlColor(color0);
		rlVertex2v(p0);
		rlColor(color1);
		rlVertex2v(p1);
		rlColor(color2);
		rlVertex2v(p2);
		rlColor(color3);
		rlVertex2v(p3);
	}
	rlEnd();
}

void DrawTextureCentered(Texture texture, Vector2 position, Color tint)
{
	position.x -= 0.5f * texture.width;
	position.y -= 0.5f * texture.height;
	DrawTextureEx(texture, position, 0, 1, tint);
}

void DrawTextureCenteredAndFlippedVertically(Texture texture, Vector2 position, Color tint)
{
	Rectangle source = { 0, 0, -(float)texture.width, (float)texture.height };
	Rectangle destination = {
		position.x - 0.5f * texture.width,
		position.y - 0.5f * texture.height,
		+(float)texture.width,
		+(float)texture.height
	};

	Vector2 origin = { 0, 0 };
	DrawTexturePro(texture, source, destination, origin, 0, tint);
}

void DrawTextureCenteredScaled(Texture texture, Vector2 position, float scale, Color tint)
{
	position.x -= 0.5f * texture.width;
	position.y -= 0.5f * texture.height;
	DrawTextureEx(texture, position, 0, scale, tint);
}
