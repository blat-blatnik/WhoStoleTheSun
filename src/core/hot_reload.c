#include "../core.h"
#include "../containers.h"
#include <stdio.h>

ENUM(ItemKind)
{
	FILE_DATA,
	TEXTURE,
};

STRUCT(TrackedItem)
{
	union
	{
		FileData file;
		Texture texture;
	};
	ItemKind kind;
	long lastModTime;
	char path[256];
};

List(TrackedItem) items;

FileData *LoadFileAndTrackChanges(const char *path)
{
	if (not FileExists(path))
		return NULL;

	unsigned bytesRead;
	void *data = LoadFileData(path, &bytesRead);

	TrackedItem *item = ListReserveOneItem(&items);
	item->kind = FILE_DATA;
	item->lastModTime = GetFileModTime(path);
	item->file.bytes = data;
	item->file.size = (int)bytesRead;
	CopyString(item->path, path, sizeof item->path);
	ASSERT(StringLength(path) < sizeof item->path);

	return &item->file;
}

Texture *LoadTextureAndTrackChanges(const char *path)
{
	if (not FileExists(path))
		return NULL;

	TrackedItem *item = ListReserveOneItem(&items);
	item->texture = LoadTexture(path);
	item->kind = TEXTURE;
	item->lastModTime = GetFileModTime(path);
	CopyString(item->path, path, sizeof item->path);
	ASSERT(StringLength(path) < sizeof item->path);

	return &item->texture;
}

void UnloadTrackedFile(FileData **data)
{
	if (not *data)
		return;

	intptr_t index = (TrackedItem *)(*data) - items;
	ASSERT(index >= 0 and index < ListCount(items)); // Is this actually a tracked item?
	ASSERT(items[index].kind == FILE_DATA);

	UnloadFileData(items[index].file.bytes);
	ListSwapRemove(&items, index);
	*data = NULL;
}

void UnloadTrackedTexture(Texture **texture)
{
	if (not *texture)
		return;

	intptr_t index = (TrackedItem *)(*texture) - items;
	ASSERT(index >= 0 and index < ListCount(items)); // Is this actually a tracked item?
	ASSERT(items[index].kind == TEXTURE);

	UnloadTexture(items[index].texture);
	ListSwapRemove(&items, index);
	*texture = NULL;
}

void HotReloadAllTrackedItems(void)
{
	for (int i = 0; i < ListCount(items); ++i)
	{
		TrackedItem *item = &items[i];
		if (not FileExists(item->path))
			continue;

		long modTime = GetFileModTime(item->path);
		if (modTime == item->lastModTime)
			continue;

		// Sometimes it takes while before the file being updated is completely written.
		// Until it's completely written, the program that's changing the file holds a lock on the file, so we can't open it.
		// If that happens, we just skip it for now, eventually it will release the lock and we will be able to open it.
		FILE *f = fopen(item->path, "rb");
		if (!f)
			continue;
		fclose(f);

		switch (item->kind)
		{
			case FILE_DATA:
			{
				UnloadFileData(item->file.bytes);
				unsigned size;
				item->file.bytes = LoadFileData(item->path, &size);
				item->file.size = (int)size;
			} break;

			case TEXTURE:
			{
				UnloadTexture(item->texture);
				item->texture = LoadTexture(item->path);
			} break;
		}

		item->lastModTime = modTime;
	}
}