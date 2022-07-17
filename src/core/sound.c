#include "../core.h"

static List(Sound *) temporarySounds;

void PlayTemporarySound(const char *path)
{
	PlayTemporarySoundEx(path, 1, 1);
}

void PlayTemporarySoundEx(const char *path, float volume, float pitch)
{
	Sound *sound = AcquireSound(path);
	if (not sound)
	{
		if (not FileExists(path))
			LogError("Couldn't play temporary sound '%s' because the file doesn't exist.", path);
		else
			LogError("Couldn't play temporary sound '%s'.", path);
		return;
	}

	SetSoundVolume(*sound, volume);
	SetSoundPitch(*sound, pitch);
	PlaySound(*sound);
	ListAdd(&temporarySounds, sound);
}

void UpdateTemporarySounds(void)
{
	for (int i = 0; i < ListCount(temporarySounds); ++i)
	{
		Sound *sound = temporarySounds[i];
		if (not IsSoundPlaying(*sound))
		{
			ReleaseAsset(sound);
			ListSwapRemove(&temporarySounds, i);
			--i;
		}
	}
}
