#include <RmlUi/Core/RenderInterfaceCompatibility.h>
#include <RmlUi/Core/Vertex.h>
#include <doctest.h>

using namespace Rml;

namespace {
class LegacyClipRecorder : public RenderInterfaceCompatibility {
public:
	bool enabled = false;
	Rectanglei region = Rectanglei::FromSize(Vector2i(0));
	void RenderGeometry(Vertex*, int, int*, int, TextureHandle, const Vector2f&) override {}
	void EnableScissorRegion(bool value) override { enabled = value; }
	void SetScissorRegion(int x, int y, int width, int height) override
	{
		region = Rectanglei::FromPositionSize({x, y}, {width, height});
	}
};
}

TEST_CASE("compatibility independent scissor and clip mask")
{
	LegacyClipRecorder legacy;
	RenderInterface* adapter = legacy.GetAdaptedInterface();
	Vertex vertices[2] = {};
	vertices[0].position = {5, 5};
	vertices[1].position = {15, 15};
	const CompiledGeometryHandle geometry = adapter->CompileGeometry({vertices, 2}, {});
	const Rectanglei scissor = Rectanglei::FromPositionSize({0, 0}, {10, 10});
	const Rectanglei mask = Rectanglei::FromPositionSize({5, 5}, {10, 10});

	adapter->SetScissorRegion(scissor);
	adapter->RenderToClipMask(ClipMaskOperation::Set, geometry, {});
	CHECK_FALSE(legacy.enabled);
	adapter->EnableScissorRegion(true);
	CHECK(legacy.enabled);
	CHECK(legacy.region == scissor);
	adapter->EnableClipMask(true);
	CHECK(legacy.region == scissor.Intersect(mask));
	adapter->EnableScissorRegion(false);
	CHECK(legacy.enabled);
	CHECK(legacy.region == mask);
	adapter->EnableScissorRegion(true);
	adapter->EnableClipMask(false);
	CHECK(legacy.enabled);
	CHECK(legacy.region == scissor);
	adapter->EnableScissorRegion(false);
	CHECK_FALSE(legacy.enabled);

	adapter->RenderToClipMask(ClipMaskOperation::Intersect, geometry, {8, 0});
	adapter->EnableClipMask(true);
	CHECK(legacy.region == mask.Intersect(mask.Translate({8, 0})));
	adapter->RenderToClipMask(ClipMaskOperation::Set, geometry, {30, 0});
	CHECK(legacy.region == mask.Translate({30, 0}));
	adapter->RenderToClipMask(ClipMaskOperation::Intersect, geometry, {});
	CHECK(legacy.region.Width() == 0);
	adapter->RenderToClipMask(ClipMaskOperation::SetInverse, geometry, {});
	CHECK_FALSE(legacy.enabled);
	adapter->EnableScissorRegion(true);
	CHECK(legacy.enabled);
	CHECK(legacy.region == scissor);
	adapter->RenderToClipMask(ClipMaskOperation::Intersect, geometry, {});
	CHECK(legacy.region == scissor.Intersect(mask));
	adapter->ReleaseGeometry(geometry);
}
