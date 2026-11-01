#include "BoxShadowCache.h"
#include "../../Include/RmlUi/Core/ComputedValues.h"
#include "../../Include/RmlUi/Core/MeshUtilities.h"
#include "../../Include/RmlUi/Core/Profiling.h"
#include "../../Include/RmlUi/Core/RenderManager.h"
#include "../Core/ControlledLifetimeResource.h"
#include "BoxShadowHash.h"
#include "GeometryBoxShadow.h"

namespace Rml {

struct BoxShadowCacheData {
	using Handles = StableUnorderedMap<BoxShadowGeometryInfo, WeakPtr<BoxShadowRenderable>>;
	SmallUnorderedMap<RenderManager*, Handles> managers;
};

static void ReleaseHandle(BoxShadowRenderable* handle);

BoxShadowRenderable::BoxShadowRenderable(RenderManager& manager, const BoxShadowGeometryInfo& geometry_info) :
	cache_key(&geometry_info), render_manager(&manager) {}

BoxShadowRenderable::~BoxShadowRenderable()
{
	ReleaseHandle(this);
}

static ControlledLifetimeResource<BoxShadowCacheData> shadow_cache_data;

void BoxShadowCache::Initialize()
{
	shadow_cache_data.Initialize();
}

void BoxShadowCache::Shutdown()
{
	while (!shadow_cache_data->managers.empty())
		ReleaseRenderManager(shadow_cache_data->managers.begin()->first);
	shadow_cache_data.Shutdown();
}

static SharedPtr<BoxShadowRenderable> GetOrCreateBoxShadow(RenderManager& render_manager, const BoxShadowGeometryInfo& info)
{
	RMLUI_ZoneScoped;
	auto& handles = shadow_cache_data->managers[&render_manager];
	auto it_handle = handles.find(info);
	if (it_handle != handles.end())
	{
		SharedPtr<BoxShadowRenderable> result = it_handle->second.lock();
		RMLUI_ASSERTMSG(result, "Failed to lock handle in Box Shadow cache");
		return result;
	}

	const auto iterator_inserted = handles.emplace(info, WeakPtr<BoxShadowRenderable>());
	RMLUI_ASSERTMSG(iterator_inserted.second, "Could not insert entry into the Box Shadow cache handle map, duplicate key.");
	const BoxShadowGeometryInfo& inserted_key = iterator_inserted.first->first;
	WeakPtr<BoxShadowRenderable>& inserted_weak_data_pointer = iterator_inserted.first->second;

	auto shadow_handle = MakeShared<BoxShadowRenderable>(render_manager, inserted_key);
	GeometryBoxShadow::GenerateTexture(shadow_handle->texture, shadow_handle->background_border_geometry, render_manager, inserted_key);

	Mesh mesh;
	const byte alpha = byte(info.opacity * 255.f);
	MeshUtilities::GenerateQuad(mesh, -info.element_offset_in_texture, Vector2f(info.texture_dimensions), ColourbPremultiplied(alpha, alpha));
	shadow_handle->geometry = render_manager.MakeGeometry(std::move(mesh));

	inserted_weak_data_pointer = shadow_handle;
	return shadow_handle;
}

static void ReleaseHandle(BoxShadowRenderable* handle)
{
	if (!handle->cache_key)
		return;
	auto manager_it = shadow_cache_data->managers.find(handle->render_manager);
	RMLUI_ASSERT(manager_it != shadow_cache_data->managers.end());
	auto& handles = manager_it->second;
	auto it = handles.find(*handle->cache_key);
	RMLUI_ASSERT(it != handles.end());
	handles.erase(it);
	if (handles.empty())
		shadow_cache_data->managers.erase(manager_it);
}

void BoxShadowCache::ReleaseRenderManager(RenderManager* render_manager)
{
	if (!shadow_cache_data)
		return;
	auto manager_it = shadow_cache_data->managers.find(render_manager);
	if (manager_it == shadow_cache_data->managers.end())
		return;
	for (auto& entry : manager_it->second)
	{
		if (auto handle = entry.second.lock())
		{
			handle->cache_key = nullptr;
			handle->texture.Release();
			handle->geometry.Release();
			handle->background_border_geometry.Release();
		}
	}
	shadow_cache_data->managers.erase(manager_it);
}

SharedPtr<BoxShadowRenderable> BoxShadowCache::GetHandle(Element* element, const ComputedValues& computed)
{
	RenderManager* render_manager = element->GetRenderManager();
	if (!render_manager)
		return {};

	ColourbPremultiplied background_color = computed.background_color().ToPremultiplied();
	Array<ColourbPremultiplied, 4> border_colors = {
		computed.border_top_color().ToPremultiplied(),
		computed.border_right_color().ToPremultiplied(),
		computed.border_bottom_color().ToPremultiplied(),
		computed.border_left_color().ToPremultiplied(),
	};
	const CornerSizes border_radius = computed.border_radius();
	BoxShadowGeometryInfo geom_info = GeometryBoxShadow::Resolve(element, border_radius, background_color, border_colors, computed.opacity());
	return GetOrCreateBoxShadow(*render_manager, geom_info);
}

} // namespace Rml
