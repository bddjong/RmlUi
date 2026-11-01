// Standalone Windows regression: link the DX11 renderer, RmlUi, d3d11 and d3dcompiler.
#include "../../Backends/RmlUi_Renderer_DX11.h"
#include <type_traits>

class DerivedRenderer : public RenderInterface_DX11 {
public:
	using RenderInterface_DX11::RenderInterface_DX11;
};

static_assert(!std::is_copy_constructible<RenderInterface_DX11>::value, "COM ownership must not be copied");

template <typename Renderer>
static bool RejectsNullDevice()
{
	try
	{
		Renderer renderer(nullptr);
		return false;
	}
	catch (const DX11InitializationError&)
	{
		return true;
	}
}

int main()
{
	if (!RejectsNullDevice<RenderInterface_DX11>() || !RejectsNullDevice<DerivedRenderer>())
		return 1;

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	D3D_FEATURE_LEVEL level;
	if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, &level, &context)))
		return 2;
	bool constructed = false;
	try
	{
		DerivedRenderer renderer(device);
		constructed = true;
	}
	catch (const DX11InitializationError&) {}
	context->Release();
	device->Release();
	return constructed ? 0 : 3;
}
