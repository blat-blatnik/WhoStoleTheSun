#include "../core.h"

#include <unordered_map>
#include <stdio.h>

// We basically store all assets in a big table and reference count them.
// Whenever you call AcquireXXX(path), we check if an asset with that path 
// already exists in the table, and only load it if it's not already there.
// When you call ReleaseAsset(), we decrease the reference count, and only
// when that reaches 0, the asset gets unloaded from memory.
//
// This allows us to have a powerful editor where you can change the sprites
// and scripts of all objects without any performance loss / asset duplication.

ENUM(AssetKind)
{
	COLLISION_MAP,
	TEXTURE,
	SPRITE,
	SCRIPT,
	MUSIC,
	SOUND,
	ASSET_KIND_ENUM_COUNT,
};

STRUCT(Asset)
{
	union
	{
		Image collisionMap;
		Texture texture;
		Sprite sprite;
		Script script;
		Music music;
		Sound sound;
	};
	int referenceCount;
	AssetKind kind;
	char path[256];
	long lastModTime;
};

// Try to ignore this C++ bullshit :)
struct Hash { unsigned operator()(const char *string) const { return HashString(string); } };
struct Equal { bool operator()(const char *a, const char *b) const { return StringsEqual(a, b); } };

static std::unordered_map<const char *, Asset *, Hash, Equal> table;

static long GetDirectoryModTime(const char *path)
{
	FilePathList files = LoadDirectoryFiles(path);

	long result = LONG_MIN;
	for (int i = 0; i < (int)files.count; ++i)
	{
		long modTime = GetFileModTime(files.paths[i]);
		if (result < modTime)
			result = modTime;
	}
	return result;
}
static long GetFileOrDirectoryModTime(const char *path)
{
	if (IsPathFile(path))
		return GetFileModTime(path);
	else
		return GetDirectoryModTime(path);
}
static bool AcquireAsset(const char *path, AssetKind kind, Asset **outResult)
{
	*outResult = NULL;
	if (not path or not path[0])
		return false;

	auto iterator = table.find(path);
	if (iterator != table.end())
	{
		Asset *asset = iterator->second;
		ASSERT(asset->kind == kind);

		++asset->referenceCount;
		*outResult = asset;
		return true;
	}

	if (not FileExists(path))
		return false;

	Asset *asset = new Asset;
	asset->kind = kind;
	asset->referenceCount = 1;
	asset->lastModTime = GetFileOrDirectoryModTime(path);

	ASSERT(StringLength(path) < sizeof asset->path - 1);
	CopyString(asset->path, path, sizeof asset->path);

	table.insert({ asset->path, asset });

	*outResult = asset;
	return false;
}
static bool IsAsset(Asset *asset)
{
	return
		asset and
		asset->kind >= 0 and asset->kind < ASSET_KIND_ENUM_COUNT and
		asset->referenceCount >= 0 and
		table.find(asset->path) != table.end();
}

extern "C"
{
	Image *AcquireCollisionMap(const char *path)
	{
		Asset *asset;
		if (AcquireAsset(path, COLLISION_MAP, &asset))
			return &asset->collisionMap;
		if (not asset)
			return NULL;

		asset->collisionMap = LoadImage(path);
		ImageFormat(&asset->collisionMap, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
		return &asset->collisionMap;
	}

	Sprite *AcquireSprite(const char *path)
	{
		Asset *asset;
		if (AcquireAsset(path, SPRITE, &asset))
			return &asset->sprite;
		if (not asset)
			return NULL;

		asset->sprite = LoadSprite(path);
		return &asset->sprite;
	}

	Texture *AcquireTexture(const char *path)
	{
		Asset *asset;
		if (AcquireAsset(path, TEXTURE, &asset))
			return &asset->texture;
		if (not asset)
			return NULL;

		asset->texture = LoadTexture(path);
		return &asset->texture;
	}

	Script *AcquireScript(const char *path, Font regular, Font bold, Font italic, Font boldItalic)
	{
		Asset *asset;
		if (AcquireAsset(path, SCRIPT, &asset))
			return &asset->script;
		if (not asset)
			return NULL;

		asset->script = LoadScript(path, regular, bold, italic, boldItalic);
		return &asset->script;
	}

	Music *AcquireMusic(const char *path);

	Sound *AcquireSound(const char *path)
	{
		Asset *asset;
		if (AcquireAsset(path, SOUND, &asset))
			return &asset->sound;
		if (not asset)
			return NULL;

		asset->sound = LoadSound(path);
		return &asset->sound;
	}

	void ReleaseAsset(void *asset)
	{
		if (not asset)
			return;

		Asset *a = (Asset *)asset;
		ASSERT(IsAsset(a));

		if (a->referenceCount <= 0)
			return;

		--a->referenceCount;
		if (a->referenceCount > 0)
			return;

		switch (a->kind)
		{
			case COLLISION_MAP: UnloadImage(a->collisionMap); break;
			case TEXTURE:       UnloadTexture(a->texture);    break;
			case SCRIPT:        UnloadScript(&a->script);     break;
			case SOUND:         UnloadSound(a->sound);        break;
		}

		table.erase(a->path);
		delete a;
	}

	void *CloneAsset(void *asset)
	{
		Asset *a = (Asset *)asset;
		if (not IsAsset(a))
			return NULL;
		++a->referenceCount;
		return a;
	}

	const char *GetAssetPath(const void *asset)
	{
		Asset *a = (Asset *)asset;
		if (!IsAsset(a))
			return NULL;
		return a->path;
	}

	void UpdateAllChangedAssets(void)
	{
		for (auto &keyval : table)
		{
			Asset *asset = keyval.second;
			if (asset->kind == SOUND or asset->kind == MUSIC)
				continue; // We don't hot reload these.

			if (not FileExists(asset->path))
				continue;

			long modTime = GetFileModTime(asset->path);
			if (modTime == asset->lastModTime)
				continue;

			// Sometimes it takes while before the file being updated is completely written.
			// Until it's completely written, the program that's changing the file holds a lock on the file, so we can't open it.
			// If that happens, we just skip it for now, eventually it will release the lock and we will be able to open it.
			FILE *f = fopen(asset->path, "rb");
			if (!f)
				continue;

			switch (asset->kind)
			{
				case COLLISION_MAP:
				{
					UnloadImage(asset->collisionMap);
					asset->collisionMap = LoadImage(asset->path);
				} break;

				case TEXTURE:
				{
					UnloadTexture(asset->texture);
					asset->texture = LoadTexture(asset->path);
				} break;

				case SCRIPT:
				{
					Font regular    = asset->script.font;
					Font bold       = asset->script.boldFont;
					Font italic     = asset->script.italicFont;
					Font boldItalic = asset->script.boldItalicFont;
					UnloadScript(&asset->script);
					asset->script = LoadScript(asset->path, regular, bold, italic, boldItalic);
				} break;
			}

			fclose(f); // Aparently if you don't keep a file handle open the whole time we sometimes fail to load.. I have no clue why.
			asset->lastModTime = modTime;
		}
	}
}
