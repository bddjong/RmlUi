#include "../Common/TestsInterface.h"
#include "../../../Source/Core/TextureDatabase.h"
#include <RmlUi/Core/CallbackTexture.h>
#include <RmlUi/Core/RenderManager.h>
#include <RmlUi/Core/Texture.h>
#include <doctest.h>
#include <functional>
#include <vector>

using namespace Rml;

namespace {
struct ReentrantRenderer : TestsRenderInterface {
	std::function<void()> on_load, on_release;
	std::vector<TextureHandle> released;
	TextureHandle LoadTexture(Vector2i& dimensions, const String&) override
	{
		if (on_load) on_load();
		dimensions = {4, 5};
		return 17;
	}
	void ReleaseTexture(TextureHandle handle) override
	{
		released.push_back(handle);
		if (on_release) on_release();
	}
};
}

TEST_CASE("texture callbacks survive nested invalidation and destruction")
{
	ReentrantRenderer renderer;
	RenderManager manager(&renderer);
	CallbackTexture owner;
	Texture view;
	int calls = 0;
	owner = manager.MakeCallbackTexture([&](const CallbackTextureInterface& api) {
		++calls;
		CHECK(view.GetDimensions() == Vector2i());
		std::vector<CallbackTexture> nested;
		for (int i = 0; i < 80; ++i)
		{
			manager.LoadTexture("?nested" + std::to_string(i));
			nested.push_back(manager.MakeCallbackTexture([](const CallbackTextureInterface&) { return true; }));
		}
		owner.Release();
		api.SetTextureHandle(31, {2, 3});
		return true;
	});
	view = owner;
	CHECK(view.GetDimensions() == Vector2i());
	CHECK(calls == 1);
	REQUIRE(renderer.released.size() == 1);
	CHECK(renderer.released[0] == 31);

	CallbackTextureDatabase db;
	auto index = db.CreateTexture([&](const CallbackTextureInterface& api) {
		db.ReleaseAllTextures(&renderer);
		api.SetTextureHandle(32, {2, 3});
		return true;
	});
	CHECK(db.GetDimensions(&manager, &renderer, index) == Vector2i());
	CHECK(renderer.released.back() == 32);
	db.ReleaseTexture(&renderer, index);
}

TEST_CASE("file textures survive nested loads and release callbacks")
{
	ReentrantRenderer renderer;
	FileTextureDatabase db;
	auto index = db.InsertTexture("?outer");
	renderer.on_load = [&] {
		CHECK(db.GetDimensions(&renderer, index) == Vector2i());
		for (int i = 0; i < 80; ++i)
			db.InsertTexture("?file" + std::to_string(i));
		db.ReleaseTexture(&renderer, "?outer");
	};
	CHECK(db.GetDimensions(&renderer, index) == Vector2i());
	REQUIRE(renderer.released.size() == 1);
	CHECK(renderer.released[0] == 17);
	renderer.on_load = {};
	CHECK(db.GetDimensions(&renderer, index) == Vector2i(4, 5));
	renderer.on_release = [&] { db.ReleaseAllTextures(&renderer); };
	db.ReleaseAllTextures(&renderer);
	CHECK(renderer.released.size() == 2);
}
