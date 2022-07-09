#include "../core.h"
#include <stdio.h>

ENUM(ItemKind)
{
	FILE_DATA,
	TEXTURE,
	IMAGE,
	SCRIPT,
};

STRUCT(TrackedItem)
{
	union
	{
		FileData file;
		Texture texture;
		Image image;
		Script script;
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

	TrackedItem *item = ListAllocateItem(&items);
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

	TrackedItem *item = ListAllocateItem(&items);
	item->texture = LoadTexture(path);
	item->kind = TEXTURE;
	item->lastModTime = GetFileModTime(path);
	CopyString(item->path, path, sizeof item->path);
	ASSERT(StringLength(path) < sizeof item->path);

	return &item->texture;
}

Image *LoadImageAndTrackChanges(const char *path)
{
	if (not FileExists(path))
		return NULL;

	TrackedItem *item = ListAllocateItem(&items);
	item->image = LoadImage(path);
	item->kind = IMAGE;
	item->lastModTime = GetFileModTime(path);
	CopyString(item->path, path, sizeof item->path);
	ASSERT(StringLength(path) < sizeof item->path);

	return &item->image;
}

Script *LoadScriptAndTrackChanges(const char *path, Font regular, Font bold, Font italic, Font boldItalic)
{
	if (not FileExists(path))
		return NULL;

	TrackedItem *item = ListAllocateItem(&items);
	item->kind = SCRIPT;
	item->lastModTime = GetFileModTime(path);
	item->script = LoadScript(path, regular, bold, italic, boldItalic);
	CopyString(item->path, path, sizeof item->path);
	ASSERT(StringLength(path) < sizeof item->path);

	return &item->script;
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

void UnloadTrackedImage(Image **image)
{
	if (not *image)
		return;

	intptr_t index = (TrackedItem *)(*image) - items;
	ASSERT(index >= 0 and index < ListCount(items)); // Is this actually a tracked item?
	ASSERT(items[index].kind == IMAGE);

	UnloadImage(items[index].image);
	ListSwapRemove(&items, index);
	*image = NULL;
}

void UnloadTrackedScript(Script **script)
{
	if (not *script)
		return;

	intptr_t index = (TrackedItem *)(*script) - items;
	ASSERT(index >= 0 and index < ListCount(items)); // Is this actually a tracked item?
	ASSERT(items[index].kind == SCRIPT);

	UnloadScript(&items[index].script);
	ListSwapRemove(&items, index);
	*script = NULL;
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

		switch (item->kind)
		{
			case FILE_DATA:
			{
				UnloadFileData(item->file.bytes);
				unsigned size;
				item->file.bytes = LoadFileData(item->path, &size);
				item->file.size = (int)size;
			} break;

			case IMAGE:
			{
				UnloadImage(item->image);
				item->image = LoadImage(item->path);
			} break;

			case TEXTURE:
			{
				UnloadTexture(item->texture);
				item->texture = LoadTexture(item->path);
			} break;

			case SCRIPT:
			{
				Font regular    = item->script.font;
				Font bold       = item->script.boldFont;
				Font italic     = item->script.italicFont;
				Font boldItalic = item->script.boldItalicFont;
				UnloadScript(&item->script);
				item->script = LoadScript(item->path, regular, bold, italic, boldItalic);
			} break;
		}

		fclose(f); // Aparently if you don't keep a file handle open the whole time we sometimes fail to load.. I have no clue why.
		item->lastModTime = modTime;
	}
}