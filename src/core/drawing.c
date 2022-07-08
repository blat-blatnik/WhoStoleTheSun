#include "../core.h"

void DrawTextureCentered(Texture texture, Vector2 position, Color tint)
{
	position.x -= 0.5f * texture.width;
	position.y -= 0.5f * texture.height;
	DrawTextureEx(texture, position, 0, 1, tint);
}
