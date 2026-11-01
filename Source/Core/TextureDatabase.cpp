#include "TextureDatabase.h"
#include "../../Include/RmlUi/Core/Log.h"
#include "../../Include/RmlUi/Core/RenderInterface.h"

namespace Rml {

CallbackTextureDatabase::CallbackTextureDatabase()
{
	constexpr size_t reserve_callback_textures = 30;
	texture_list.reserve(reserve_callback_textures);
}

CallbackTextureDatabase::~CallbackTextureDatabase()
{
	if (!texture_list.empty())
	{
		Log::Message(Log::LT_ERROR, "TextureDatabase destroyed with outstanding callback textures. Will likely result in memory corruption.");
		RMLUI_ERROR;
	}
}

StableVectorIndex CallbackTextureDatabase::CreateTexture(CallbackTextureFunction&& callback)
{
	RMLUI_ASSERT(callback);
	auto entry = std::make_shared<CallbackTextureEntry>();
	entry->callback = std::move(callback);
	return texture_list.insert(std::move(entry));
}

void CallbackTextureDatabase::ReleaseTexture(RenderInterface* render_interface, StableVectorIndex callback_index)
{
	auto entry = texture_list.erase(callback_index);
	entry->alive = false;
	++entry->generation;
	auto handle = std::exchange(entry->texture_handle, {});
	entry->dimensions = {};
	if (handle)
		render_interface->ReleaseTexture(handle);
}

Vector2i CallbackTextureDatabase::GetDimensions(RenderManager* manager, RenderInterface* renderer, StableVectorIndex index)
{
	return EnsureLoaded(manager, renderer, index)->dimensions;
}

TextureHandle CallbackTextureDatabase::GetHandle(RenderManager* manager, RenderInterface* renderer, StableVectorIndex index)
{
	return EnsureLoaded(manager, renderer, index)->texture_handle;
}

auto CallbackTextureDatabase::EnsureLoaded(RenderManager* manager, RenderInterface* renderer, StableVectorIndex index)
	-> std::shared_ptr<CallbackTextureEntry>
{
	auto entry = texture_list[index];
	if (!entry->texture_handle && !entry->load_failed && !entry->loading)
	{
		entry->loading = true;
		const auto generation = entry->generation;
		TextureHandle handle = {};
		Vector2i dimensions = {};
		const bool success = entry->callback(CallbackTextureInterface(*manager, *renderer, handle, dimensions));
		entry->loading = false;
		if (entry->alive && entry->generation == generation && success)
		{
			entry->texture_handle = handle;
			entry->dimensions = dimensions;
		}
		else
		{
			if (entry->alive && entry->generation == generation)
				entry->load_failed = true;
			if (handle)
				renderer->ReleaseTexture(handle);
		}
	}
	return entry;
}

size_t CallbackTextureDatabase::size() const { return texture_list.size(); }

void CallbackTextureDatabase::ReleaseAllTextures(RenderInterface* renderer)
{
	Vector<TextureHandle> handles;
	texture_list.for_each([&](std::shared_ptr<CallbackTextureEntry>& entry) {
		++entry->generation;
		if (entry->texture_handle)
			handles.push_back(std::exchange(entry->texture_handle, {}));
		entry->dimensions = {};
	});
	for (auto handle : handles)
		renderer->ReleaseTexture(handle);
}

FileTextureDatabase::FileTextureDatabase() {}

FileTextureDatabase::~FileTextureDatabase()
{
#ifdef RMLUI_DEBUG
	for (const auto& texture : texture_list)
	{
		RMLUI_ASSERTMSG(!texture->texture_handle,
			"TextureDatabase destroyed without releasing all file textures first. Ensure that 'ReleaseAllTextures' is called before destruction.");
	}
#endif
}

TextureFileIndex FileTextureDatabase::InsertTexture(const String& source)
{
	auto it = texture_map.find(source);
	if (it != texture_map.end())
		return it->second;

	// The texture is not yet loaded from the render interface. That is deferred until the texture is needed, such as when it becomes visible.
	const auto index = TextureFileIndex(texture_list.size());
	texture_map[source] = index;
	auto entry = std::make_shared<FileTextureEntry>();
	entry->source = source;
	texture_list.push_back(std::move(entry));

	return index;
}

auto FileTextureDatabase::EnsureLoaded(RenderInterface* renderer, TextureFileIndex index) -> std::shared_ptr<FileTextureEntry>
{
	auto entry = texture_list[size_t(index)];
	if (!entry->texture_handle && !entry->load_texture_failed && !entry->loading)
	{
		entry->loading = true;
		const auto generation = entry->generation;
		Vector2i dimensions = {};
		auto handle = renderer->LoadTexture(dimensions, entry->source);
		entry->loading = false;
		if (entry->generation == generation)
		{
			entry->texture_handle = handle;
			entry->dimensions = dimensions;
			entry->load_texture_failed = !handle;
			if (!handle)
				Log::Message(Log::LT_WARNING, "Could not load texture: %s", entry->source.c_str());
		}
		else if (handle)
			renderer->ReleaseTexture(handle);
	}
	return entry;
}

TextureHandle FileTextureDatabase::GetHandle(RenderInterface* renderer, TextureFileIndex index)
{
	RMLUI_ASSERT(size_t(index) < texture_list.size());
	return EnsureLoaded(renderer, index)->texture_handle;
}

Vector2i FileTextureDatabase::GetDimensions(RenderInterface* renderer, TextureFileIndex index)
{
	RMLUI_ASSERT(size_t(index) < texture_list.size());
	return EnsureLoaded(renderer, index)->dimensions;
}

void FileTextureDatabase::GetSourceList(StringList& source_list) const
{
	source_list.reserve(source_list.size() + texture_list.size());
	for (const auto& texture : texture_map)
		source_list.push_back(texture.first);
}

bool FileTextureDatabase::ReleaseTexture(RenderInterface* renderer, const String& source)
{
	auto it = texture_map.find(source);
	if (it == texture_map.end())
		return false;
	auto entry = texture_list[size_t(it->second)];
	++entry->generation;
	auto handle = std::exchange(entry->texture_handle, {});
	entry->dimensions = {};
	if (handle)
		renderer->ReleaseTexture(handle);
	return bool(handle);
}

void FileTextureDatabase::ReleaseAllTextures(RenderInterface* renderer)
{
	Vector<TextureHandle> handles;
	for (const auto& entry : texture_list)
	{
		++entry->generation;
		if (entry->texture_handle)
			handles.push_back(std::exchange(entry->texture_handle, {}));
		entry->dimensions = {};
	}
	for (auto handle : handles)
		renderer->ReleaseTexture(handle);
}

} // namespace Rml
