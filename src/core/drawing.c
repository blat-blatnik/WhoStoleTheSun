#include "../core.h"

void DrawTextureCentered(Texture texture, Vector2 position, Color tint)
{
	position.x -= 0.5f * texture.width;
	position.y -= 0.5f * texture.height;
	DrawTextureEx(texture, position, 0, 1, tint);
}

void DrawTextureCenteredAndFlippedVertically(Texture texture, Vector2 position, Color tint)
{
	Rectangle source = { 0, 0, -texture.width, texture.height };
	Rectangle destination = {
		position.x - 0.5f * texture.width,
		position.y - 0.5f * texture.height,
		+(float)texture.width,
		+(float)texture.height
	};

	Vector2 origin = { 0, 0 };
	DrawTexturePro(texture, source, destination, origin, 0, tint);
	
	//Rectangle source = {
	//	(float)(texture.width - 1),
	//	(float)(0),
	//	(float)(-texture.width),
	//	(float)(texture.height)
	//};
	//DrawTextureRec(texture, source, position, tint);
}

void DrawTextureCenteredScaled(Texture texture, Vector2 position, float scale, Color tint)
{
	position.x -= 0.5f * texture.width;
	position.y -= 0.5f * texture.height;
	DrawTextureEx(texture, position, 0, scale, tint);
}
