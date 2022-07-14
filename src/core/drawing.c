#include "../core.h"

void DrawTextureCentered(Texture texture, Vector2 position, Color tint)
{
	position.x -= 0.5f * texture.width;
	position.y -= 0.5f * texture.height;
	DrawTextureEx(texture, position, 0, 1, tint);
}

void DrawTextureCenteredAndFlippedVertically(Texture texture, Vector2 position, Color tint)
{
	position.x -= 0.5f * texture.width;
	position.y -= 0.5f * texture.height;
	Rectangle source = {
		(float)(texture.width - 1),
		(float)(0),
		(float)(-texture.width),
		(float)(texture.height)
	};
	DrawTextureRec(texture, source, position, tint);
}

void DrawTextureCenteredScaled(Texture texture, Vector2 position, float scale, Color tint)
{
	position.x -= 0.5f * texture.width;
	position.y -= 0.5f * texture.height;
	DrawTextureEx(texture, position, 0, scale, tint);
}
