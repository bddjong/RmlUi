#include <RmlUi/Core/Variant.h>
#include "../Common/TestsInterface.h"
#include <RmlUi/Core/CallbackTexture.h>
#include <RmlUi/Core/CompiledFilterShader.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/Texture.h>
#include <doctest.h>
#include <type_traits>
#include <utility>

using namespace Rml;

namespace {
template <typename Resource, typename Make, typename Releases>
void CheckResourceLifetime(Make make, Releases releases)
{
	static_assert(std::is_nothrow_move_constructible<Resource>::value, "Resource must remain movable");
	static_assert(std::is_nothrow_move_assignable<Resource>::value, "Resource must remain move assignable");
	static_assert(!std::is_copy_constructible<Resource>::value, "Resource must remain unique");
	const auto initial = releases();
	{
		Resource empty;
		Resource first = make();
		Resource second = make();
		REQUIRE(first);
		REQUIRE(second);
		Resource moved(std::move(first));
		CHECK_FALSE(first);
		CHECK(releases() == initial);
		second = std::move(moved);
		CHECK_FALSE(moved);
		CHECK(releases() == initial + 1);
		second = std::move(second);
		CHECK(second);
		CHECK(releases() == initial + 1);
		empty = std::move(second);
		CHECK_FALSE(second);
		CHECK(releases() == initial + 1);
	}
	CHECK(releases() == initial + 2);
	{
		Resource resource = make();
		resource.Release();
		CHECK_FALSE(resource);
		CHECK(releases() == initial + 3);
	}
	CHECK(releases() == initial + 3);
}
}

TEST_CASE("unique render resource lifetime and moves")
{
	TestsRenderInterface interface;
	RenderManager manager(&interface);
	CheckResourceLifetime<Geometry>([&] {
		Mesh mesh;
		mesh.vertices.resize(3);
		mesh.indices = {0, 1, 2};
		Geometry geometry = manager.MakeGeometry(std::move(mesh));
		geometry.Render({});
		return geometry;
	}, [&] { return interface.GetCounters().release_geometry; });
	CheckResourceLifetime<CallbackTexture>([&] {
		CallbackTexture texture = manager.MakeCallbackTexture([](const CallbackTextureInterface& callback) {
			callback.SetTextureHandle(1, {1, 1});
			return true;
		});
		Texture view = texture;
		view.GetDimensions();
		return texture;
	}, [&] { return interface.GetCounters().release_texture; });
	CheckResourceLifetime<CompiledFilter>([&] { return manager.CompileFilter("test", {}); },
		[&] { return interface.GetCounters().release_filter; });
	CheckResourceLifetime<CompiledShader>([&] { return manager.CompileShader("test", {}); },
		[&] { return interface.GetCounters().release_shader; });
}
