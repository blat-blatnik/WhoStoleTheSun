#include "../core.h"

Sprite LoadSprite(const char *path)
{
	Sprite s = { 0 };
	if (not FileExists(path))
	{
		LogError("Couldn't load sprite from '%s' because that path doesn't exist.", path);
		return s;
	}

	if (IsPathFile(path))
	{
		s.numFrames = 1;
		s.frames = MemAlloc(sizeof s.frames[0]);
		s.frames[0] = LoadTexture(path);
	}
	else
	{
		FilePathList contents = LoadDirectoryFiles(path);
		{
			if (not contents.count)
			{
				LogError("Couldn't load sprite from '%s' because the directory is empty.", path);
				return s;
			}
			else
			{
				s.numFrames = (int)contents.count;
				s.frames = MemAlloc(s.numFrames * sizeof s.frames[0]);
				for (int i = 0; i < s.numFrames; ++i)
					s.frames[i] = LoadTexture(contents.paths[i]);
			}
		}
		UnloadDirectoryFiles(contents);
	}

	return s;
}

void UnloadSprite(Sprite sprite)
{
	for (int i = 0; i < sprite.numFrames; ++i)
		UnloadTexture(sprite.frames[i]);
	MemFree(sprite.frames);
}
