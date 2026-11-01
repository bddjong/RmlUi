#include "../include/RendererExtensions.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/Traits.h>
#include <limits>

#if defined RMLUI_RENDERER_GL2

	#if defined RMLUI_PLATFORM_WIN32
		#include <RmlUi_Include_Windows.h>
		#include <gl/Gl.h>
	#elif defined RMLUI_PLATFORM_MACOSX
		#include <OpenGL/gl.h>
		#include <OpenGL/glext.h>
	#elif defined RMLUI_PLATFORM_UNIX
		#include <GL/gl.h>
		#include <GL/glext.h>
		#include <GL/glx.h>
		#include <RmlUi_Include_Xlib.h>
	#endif

#elif defined RMLUI_RENDERER_GL3 && !defined RMLUI_PLATFORM_EMSCRIPTEN

	#include <RmlUi_Include_GL3.h>

#elif defined RMLUI_RENDERER_GL3 && defined RMLUI_PLATFORM_EMSCRIPTEN

	#include <GLES3/gl3.h>

#elif defined RMLUI_RENDERER_DX12

	#include <RmlUi_Renderer_DX12.h>

#elif defined RMLUI_RENDERER_VK

	#include <RmlUi_Renderer_VK.h>

#endif

#if defined RMLUI_RENDERER_GL2 || defined RMLUI_RENDERER_GL3

RendererExtensions::Image RendererExtensions::CaptureScreen()
{
	int viewport[4] = {}; // x, y, width, height
	glGetIntegerv(GL_VIEWPORT, viewport);

	Image image;
	image.num_components = 3;
	image.width = viewport[2];
	image.height = viewport[3];

	if (image.width < 1 || image.height < 1)
		return Image();

	const size_t row_size = size_t(image.width) * size_t(image.num_components);
	if (row_size / size_t(image.num_components) != size_t(image.width) ||
		size_t(image.height) > std::numeric_limits<size_t>::max() / row_size)
		return Image();
	image.data = Rml::UniquePtr<Rml::byte[]>(new Rml::byte[row_size * size_t(image.height)]);

	GLint alignment, row_length, skip_rows, skip_pixels;
	glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
	glGetIntegerv(GL_PACK_ROW_LENGTH, &row_length);
	glGetIntegerv(GL_PACK_SKIP_ROWS, &skip_rows);
	glGetIntegerv(GL_PACK_SKIP_PIXELS, &skip_pixels);
#if defined RMLUI_RENDERER_GL3
	GLint pack_buffer;
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &pack_buffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#endif
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
	glReadPixels(viewport[0], viewport[1], image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.data.get());
	glPixelStorei(GL_PACK_ALIGNMENT, alignment);
	glPixelStorei(GL_PACK_ROW_LENGTH, row_length);
	glPixelStorei(GL_PACK_SKIP_ROWS, skip_rows);
	glPixelStorei(GL_PACK_SKIP_PIXELS, skip_pixels);
#if defined RMLUI_RENDERER_GL3
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pack_buffer);
#endif

	bool result = true;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		result = false;
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screenshot, got GL error: 0x%x", err);
	}

	if (!result)
		return Image();

	return image;
}

#elif defined RMLUI_RENDERER_DX12

RendererExtensions::Image RendererExtensions::CaptureScreen()
{
	auto* render_interface = rmlui_static_cast<RenderInterface_DX12*>(Rml::GetRenderInterface());
	if (!render_interface)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screen. RmlUi render interface not set, or unexpected type.");
		return {};
	}

	RendererExtensions::Image image;
	if (!render_interface->CaptureScreen(image.width, image.height, image.num_components, image.data))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "The DirectX 12 renderer was unable to capture screen.");
		return {};
	}

	return image;
}

#elif defined RMLUI_RENDERER_VK

RendererExtensions::Image RendererExtensions::CaptureScreen()
{
	auto* render_interface = rmlui_static_cast<RenderInterface_VK*>(Rml::GetRenderInterface());
	if (!render_interface)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Could not capture screen. RmlUi render interface not set, or unexpected type.");
		return {};
	}

	RendererExtensions::Image image;
	if (!render_interface->CaptureScreen(image.width, image.height, image.num_components, image.data))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "The Vulkan renderer was unable to capture screen.");
		return {};
	}

	return image;
}

#else

RendererExtensions::Image RendererExtensions::CaptureScreen()
{
	return Image();
}

#endif
