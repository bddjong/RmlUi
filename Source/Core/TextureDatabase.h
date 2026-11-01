#pragma once

#include "../../Include/RmlUi/Core/CallbackTexture.h"
#include "../../Include/RmlUi/Core/StableVector.h"
#include "../../Include/RmlUi/Core/Types.h"
#include <memory>

namespace Rml {

class RenderInterface;

class CallbackTextureDatabase : NonCopyMoveable {
public:
	CallbackTextureDatabase();
	~CallbackTextureDatabase();

	StableVectorIndex CreateTexture(CallbackTextureFunction&& callback);
	void ReleaseTexture(RenderInterface* render_interface, StableVectorIndex callback_index);

	Vector2i GetDimensions(RenderManager* render_manager, RenderInterface* render_interface, StableVectorIndex callback_index);
	TextureHandle GetHandle(RenderManager* render_manager, RenderInterface* render_interface, StableVectorIndex callback_index);

	size_t size() const;

	void ReleaseAllTextures(RenderInterface* render_interface);

private:
	struct CallbackTextureEntry {
		CallbackTextureFunction callback;
		TextureHandle texture_handle = {};
		Vector2i dimensions;
		bool load_failed = false;
		bool loading = false;
		bool alive = true;
		unsigned generation = 0;
	};

	std::shared_ptr<CallbackTextureEntry> EnsureLoaded(RenderManager* render_manager, RenderInterface* render_interface, StableVectorIndex callback_index);

	StableVector<std::shared_ptr<CallbackTextureEntry>> texture_list;
};

class FileTextureDatabase : NonCopyMoveable {
public:
	FileTextureDatabase();
	~FileTextureDatabase();

	TextureFileIndex InsertTexture(const String& source);

	TextureHandle GetHandle(RenderInterface* render_interface, TextureFileIndex index);
	Vector2i GetDimensions(RenderInterface* render_interface, TextureFileIndex index);

	void GetSourceList(StringList& source_list) const;

	bool ReleaseTexture(RenderInterface* render_interface, const String& source);
	void ReleaseAllTextures(RenderInterface* render_interface);

private:
	struct FileTextureEntry {
		TextureHandle texture_handle = {};
		Vector2i dimensions;
		bool load_texture_failed = false;
		bool loading = false;
		unsigned generation = 0;
		String source;
	};

	std::shared_ptr<FileTextureEntry> EnsureLoaded(RenderInterface* render_interface, TextureFileIndex index);

	Vector<std::shared_ptr<FileTextureEntry>> texture_list;
	UnorderedMap<String, TextureFileIndex> texture_map; // key: source, value: index into 'texture_list'
};

class TextureDatabase {
public:
	FileTextureDatabase file_database;
	CallbackTextureDatabase callback_database;
};

} // namespace Rml
