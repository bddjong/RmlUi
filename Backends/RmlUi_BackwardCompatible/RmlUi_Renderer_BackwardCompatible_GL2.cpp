#include "RmlUi_Renderer_BackwardCompatible_GL2.h"
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/Platform.h>
#include <string.h>
#include <limits>
#include <new>
#include <cstdio>
#include <cstring>
#include <cstdint>

#if defined RMLUI_PLATFORM_WIN32
	#include "RmlUi_Include_Windows.h"
	#include <gl/Gl.h>
	#include <gl/Glu.h>
#elif defined RMLUI_PLATFORM_MACOSX
	#include <OpenGL/gl.h>
	#include <OpenGL/glext.h>
	#include <OpenGL/glu.h>
#elif defined RMLUI_PLATFORM_UNIX
	#include "RmlUi_Include_Xlib.h"
	#include <GL/gl.h>
	#include <GL/glext.h>
	#include <GL/glu.h>
	#include <GL/glx.h>
#endif

#define GL_CLAMP_TO_EDGE 0x812F

// GL 2.1 and ARB_pixel_buffer_object can reinterpret client pointers as offsets.
// Resolve buffer functions only when the active context supports pixel buffers.
#if defined RMLUI_PLATFORM_WIN32
using BindUnpackBuffer = void (APIENTRY*)(GLenum, GLuint);
#else
using BindUnpackBuffer = void (*)(GLenum, GLuint);
#endif
static bool GetUnpackBufferBinding(BindUnpackBuffer& bind_buffer)
{
	bind_buffer = nullptr;
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	int major = 0, minor = 0;
	if (!version || std::sscanf(version, "%d.%d", &major, &minor) != 2)
		return false;
	if (major < 2 || (major == 2 && minor < 1))
	{
		const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
		auto has_extension = [extensions](const char* name) {
			if (!extensions) return false;
			const size_t length = std::strlen(name);
			const char* ext = extensions;
			while ((ext = std::strstr(ext, name)))
			{
				if ((ext == extensions || ext[-1] == ' ') && (ext[length] == 0 || ext[length] == ' ')) return true;
				ext += length;
			}
			return false;
		};
		if (!has_extension("GL_ARB_pixel_buffer_object") && !has_extension("GL_EXT_pixel_buffer_object"))
			return true;
	}
#if defined RMLUI_PLATFORM_WIN32
	bind_buffer = reinterpret_cast<BindUnpackBuffer>(wglGetProcAddress("glBindBuffer"));
	if (!bind_buffer || reinterpret_cast<uintptr_t>(bind_buffer) <= 3 || reinterpret_cast<uintptr_t>(bind_buffer) == ~uintptr_t(0))
		bind_buffer = reinterpret_cast<BindUnpackBuffer>(wglGetProcAddress("glBindBufferARB"));
	if (reinterpret_cast<uintptr_t>(bind_buffer) <= 3 || reinterpret_cast<uintptr_t>(bind_buffer) == ~uintptr_t(0))
		bind_buffer = nullptr;
#elif defined RMLUI_PLATFORM_MACOSX
	bind_buffer = &glBindBuffer;
#elif defined RMLUI_PLATFORM_UNIX
	bind_buffer = reinterpret_cast<BindUnpackBuffer>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glBindBuffer")));
	if (!bind_buffer)
		bind_buffer = reinterpret_cast<BindUnpackBuffer>(glXGetProcAddressARB(reinterpret_cast<const GLubyte*>("glBindBufferARB")));
#endif
	return bind_buffer != nullptr;
}

RenderInterface_BackwardCompatible_GL2::RenderInterface_BackwardCompatible_GL2() {}

void RenderInterface_BackwardCompatible_GL2::SetViewport(int in_viewport_width, int in_viewport_height)
{
	viewport_width = in_viewport_width;
	viewport_height = in_viewport_height;
}

void RenderInterface_BackwardCompatible_GL2::BeginFrame()
{
	RMLUI_ASSERT(viewport_width >= 0 && viewport_height >= 0);
	glViewport(0, 0, viewport_width, viewport_height);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Rml::Matrix4f projection = Rml::Matrix4f::ProjectOrtho(0, (float)viewport_width, (float)viewport_height, 0, -10000, 10000);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection.data());
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	transform_enabled = false;
}

void RenderInterface_BackwardCompatible_GL2::EndFrame() {}

void RenderInterface_BackwardCompatible_GL2::Clear()
{
	glClearStencil(0);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RenderInterface_BackwardCompatible_GL2::RenderGeometry(Rml::Vertex* vertices, int /*num_vertices*/, int* indices, int num_indices,
	const Rml::TextureHandle texture, const Rml::Vector2f& translation)
{
	glPushMatrix();
	glTranslatef(translation.x, translation.y, 0);

	glVertexPointer(2, GL_FLOAT, sizeof(Rml::Vertex), &vertices[0].position);
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Rml::Vertex), &vertices[0].colour);

	if (!texture)
	{
		glDisable(GL_TEXTURE_2D);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	else
	{
		glEnable(GL_TEXTURE_2D);

		if (texture != TextureEnableWithoutBinding)
			glBindTexture(GL_TEXTURE_2D, (GLuint)texture);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Rml::Vertex), &vertices[0].tex_coord);
	}

	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, indices);

	glPopMatrix();
}

void RenderInterface_BackwardCompatible_GL2::EnableScissorRegion(bool enable)
{
	if (enable)
	{
		if (!transform_enabled)
		{
			glEnable(GL_SCISSOR_TEST);
			glDisable(GL_STENCIL_TEST);
		}
		else
		{
			glDisable(GL_SCISSOR_TEST);
			glEnable(GL_STENCIL_TEST);
		}
	}
	else
	{
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_STENCIL_TEST);
	}
}

void RenderInterface_BackwardCompatible_GL2::SetScissorRegion(int x, int y, int width, int height)
{
	if (!transform_enabled)
	{
		glScissor(x, viewport_height - (y + height), width, height);
	}
	else
	{
		// clear the stencil buffer
		glStencilMask(GLuint(-1));
		glClear(GL_STENCIL_BUFFER_BIT);

		// fill the stencil buffer
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		glStencilFunc(GL_NEVER, 1, GLuint(-1));
		glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

		float fx = (float)x;
		float fy = (float)y;
		float fwidth = (float)width;
		float fheight = (float)height;

		// draw transformed quad
		GLfloat vertices[] = {fx, fy, 0, fx, fy + fheight, 0, fx + fwidth, fy + fheight, 0, fx + fwidth, fy, 0};
		glDisableClientState(GL_COLOR_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vertices);
		GLushort indices[] = {1, 2, 0, 3};
		glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, indices);
		glEnableClientState(GL_COLOR_ARRAY);

		// prepare for drawing the real thing
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
		glStencilMask(0);
		glStencilFunc(GL_EQUAL, 1, GLuint(-1));
	}
}

// Set to byte packing, or the compiler will expand our struct, which means it won't read correctly from file
#pragma pack(1)
struct TGAHeader {
	char idLength;
	char colourMapType;
	char dataType;
	short int colourMapOrigin;
	short int colourMapLength;
	char colourMapDepth;
	short int xOrigin;
	short int yOrigin;
	short int width;
	short int height;
	char bitsPerPixel;
	char imageDescriptor;
};
// Restore packing
#pragma pack()

bool RenderInterface_BackwardCompatible_GL2::LoadTexture(Rml::TextureHandle& texture_handle, Rml::Vector2i& texture_dimensions,
	const Rml::String& source)
{
	Rml::FileInterface* file_interface = Rml::GetFileInterface();
	Rml::FileHandle file_handle = file_interface->Open(source);
	if (!file_handle)
	{
		return false;
	}

	if (!file_interface->Seek(file_handle, 0, SEEK_END))
	{
		file_interface->Close(file_handle);
		return false;
	}
	size_t buffer_size = file_interface->Tell(file_handle);
	if (!file_interface->Seek(file_handle, 0, SEEK_SET))
	{
		file_interface->Close(file_handle);
		return false;
	}

	if (buffer_size <= sizeof(TGAHeader))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Texture file size is smaller than TGAHeader, file is not a valid TGA image.");
		file_interface->Close(file_handle);
		return false;
	}

	char* buffer = new (std::nothrow) char[buffer_size];
	if (!buffer)
	{
		file_interface->Close(file_handle);
		return false;
	}
	const size_t bytes_read = file_interface->Read(buffer, buffer_size, file_handle);
	file_interface->Close(file_handle);
	if (bytes_read != buffer_size)
	{
		delete[] buffer;
		return false;
	}

	TGAHeader header;
	memcpy(&header, buffer, sizeof(TGAHeader));

	int color_mode = header.bitsPerPixel / 8;
	// TGA dimensions are little-endian, independent of host byte order.
	const auto* header_bytes = reinterpret_cast<const unsigned char*>(buffer);
	const size_t width = size_t(header_bytes[12]) | (size_t(header_bytes[13]) << 8);
	const size_t height = size_t(header_bytes[14]) | (size_t(header_bytes[15]) << 8);
	const size_t id_length = static_cast<unsigned char>(header.idLength);
	const size_t pixel_count = width * height;
	const unsigned int alpha_bits = static_cast<unsigned char>(header.imageDescriptor) & 0x0f;

	if (header.dataType != 2 || header.colourMapType != 0 || !width || !height ||
		(static_cast<unsigned char>(header.imageDescriptor) & 0xc0) != 0 ||
		(alpha_bits != 0 && (header.bitsPerPixel != 32 || alpha_bits != 8)))
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24/32bit uncompressed TGAs are supported.");
		delete[] buffer;
		return false;
	}

	// Ensure we have at least 3 colors
	if (header.bitsPerPixel != 24 && header.bitsPerPixel != 32)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Only 24 and 32bit textures are supported.");
		delete[] buffer;
		return false;
	}

	const size_t data_offset = sizeof(TGAHeader) + id_length;
	if (pixel_count > std::numeric_limits<size_t>::max() / 4 || data_offset > buffer_size ||
		pixel_count > (buffer_size - data_offset) / color_mode)
	{
		delete[] buffer;
		return false;
	}
	const char* image_src = buffer + data_offset;
	unsigned char* image_dest = new (std::nothrow) unsigned char[pixel_count * 4];
	if (!image_dest)
	{
		delete[] buffer;
		return false;
	}

	// Targa is BGR, swap to RGB and flip Y axis
	for (size_t y = 0; y < height; y++)
	{
		size_t read_index = y * width * color_mode;
		const size_t row = (header.imageDescriptor & 32) ? y : height - y - 1;
		for (size_t x = 0; x < width; x++)
		{
			const size_t column = (header.imageDescriptor & 16) ? width - x - 1 : x;
			const size_t write_index = (row * width + column) * 4;
			image_dest[write_index] = image_src[read_index + 2];
			image_dest[write_index + 1] = image_src[read_index + 1];
			image_dest[write_index + 2] = image_src[read_index];
			if (color_mode == 4 && alpha_bits == 8)
				image_dest[write_index + 3] = image_src[read_index + 3];
			else
				image_dest[write_index + 3] = 255;

			read_index += color_mode;
		}
	}

	texture_dimensions.x = static_cast<int>(width);
	texture_dimensions.y = static_cast<int>(height);

	bool success = GenerateTexture(texture_handle, image_dest, texture_dimensions);

	delete[] image_dest;
	delete[] buffer;

	return success;
}

bool RenderInterface_BackwardCompatible_GL2::GenerateTexture(Rml::TextureHandle& texture_handle, const Rml::byte* source,
	const Rml::Vector2i& source_dimensions)
{
	GLint max_size = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	if (source_dimensions.x <= 0 || source_dimensions.y <= 0 || source_dimensions.x > max_size || source_dimensions.y > max_size)
		return false;
	BindUnpackBuffer bind_unpack_buffer;
	if (!GetUnpackBufferBinding(bind_unpack_buffer))
		return false;
	GLuint texture_id = 0;
	glGenTextures(1, &texture_id);
	if (texture_id == 0)
	{
		Rml::Log::Message(Rml::Log::LT_ERROR, "Failed to generate texture.");
		return false;
	}

	GLint previous_texture = 0, alignment = 0, row_length = 0, skip_rows = 0, skip_pixels = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &row_length);
	glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skip_rows);
	glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skip_pixels);
	GLint unpack_buffer = 0;
	if (bind_unpack_buffer)
	{
		glGetIntegerv(0x88EF, &unpack_buffer); // GL_PIXEL_UNPACK_BUFFER_BINDING
		bind_unpack_buffer(0x88EC, 0); // GL_PIXEL_UNPACK_BUFFER
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, source_dimensions.x, source_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
	if (bind_unpack_buffer)
		bind_unpack_buffer(0x88EC, unpack_buffer);
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, skip_rows);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, skip_pixels);
	GLint uploaded_width = 0, uploaded_height = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &uploaded_width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &uploaded_height);
	if (uploaded_width != source_dimensions.x || uploaded_height != source_dimensions.y)
	{
		glBindTexture(GL_TEXTURE_2D, previous_texture);
		glDeleteTextures(1, &texture_id);
		return false;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, previous_texture);
	texture_handle = (Rml::TextureHandle)texture_id;

	return true;
}

void RenderInterface_BackwardCompatible_GL2::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	glDeleteTextures(1, (GLuint*)&texture_handle);
}

void RenderInterface_BackwardCompatible_GL2::SetTransform(const Rml::Matrix4f* transform)
{
	transform_enabled = (transform != nullptr);

	if (transform)
	{
		if (std::is_same<Rml::Matrix4f, Rml::ColumnMajorMatrix4f>::value)
			glLoadMatrixf(transform->data());
		else if (std::is_same<Rml::Matrix4f, Rml::RowMajorMatrix4f>::value)
			glLoadMatrixf(transform->Transpose().data());
	}
	else
		glLoadIdentity();
}
