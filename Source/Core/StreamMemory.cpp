#include "../../Include/RmlUi/Core/StreamMemory.h"
#include <stdio.h>
#include <string.h>

namespace Rml {

const int DEFAULT_BUFFER_SIZE = 256;
const int BUFFER_INCREMENTS = 256;

StreamMemory::StreamMemory()
{
	buffer = nullptr;
	buffer_ptr = nullptr;
	buffer_size = 0;
	buffer_used = 0;
	owns_buffer = true;
	Reallocate(DEFAULT_BUFFER_SIZE);
}

StreamMemory::StreamMemory(size_t initial_size)
{
	buffer = nullptr;
	buffer_ptr = nullptr;
	buffer_size = 0;
	buffer_used = 0;
	owns_buffer = true;
	Reallocate(initial_size);
}

StreamMemory::StreamMemory(const byte* _buffer, size_t _buffer_size)
{
	buffer = (byte*)_buffer;
	buffer_size = _buffer_size;
	buffer_used = _buffer_size;
	owns_buffer = false;
	buffer_ptr = buffer;
}

StreamMemory::~StreamMemory()
{
	if (owns_buffer)
		free(buffer);
}

void StreamMemory::Close()
{
	Stream::Close();
}

bool StreamMemory::IsEOS() const
{
	return buffer_ptr >= buffer + buffer_used;
}

size_t StreamMemory::Tell() const
{
	return buffer_ptr - buffer;
}

size_t StreamMemory::Length() const
{
	return buffer_used;
}

size_t StreamMemory::Read(void* _buffer, size_t bytes) const
{
	bytes = Math::Min(bytes, (size_t)(buffer + buffer_used - buffer_ptr));

	memcpy(_buffer, buffer_ptr, bytes);

	buffer_ptr += bytes;

	return bytes;
}

size_t StreamMemory::Peek(void* _buffer, size_t bytes) const
{
	bytes = Math::Min(bytes, (size_t)(buffer + buffer_used - buffer_ptr));

	memcpy(_buffer, buffer_ptr, bytes);

	return bytes;
}

size_t StreamMemory::Write(const void* _buffer, size_t bytes)
{
	if (!owns_buffer)
		return 0;
	if (buffer_ptr + bytes > buffer + buffer_size)
		if (!Reallocate(bytes + BUFFER_INCREMENTS))
			return 0;

	memcpy(buffer_ptr, _buffer, bytes);

	buffer_ptr += bytes;
	buffer_used = Math::Max((size_t)(buffer_ptr - buffer), buffer_used);

	return bytes;
}

size_t StreamMemory::Truncate(size_t bytes)
{
	if (bytes > buffer_used)
		return 0;

	size_t old_size = buffer_used;
	buffer_used = bytes;
	buffer_ptr = buffer + bytes;
	return old_size - buffer_used;
}

bool StreamMemory::Seek(long offset, int origin) const
{
	size_t base;
	switch (origin)
	{
	case SEEK_SET: base = 0; break;
	case SEEK_END: base = buffer_used; break;
	case SEEK_CUR: base = Tell(); break;
	default: return false;
	}

	size_t position;
	if (offset < 0)
	{
		const size_t magnitude = size_t(-(offset + 1)) + 1;
		if (magnitude > base)
			return false;
		position = base - magnitude;
	}
	else
	{
		if (size_t(offset) > buffer_used - base)
			return false;
		position = base + size_t(offset);
	}
	buffer_ptr = buffer + position;
	return true;
}

size_t StreamMemory::PushFront(const void* _buffer, size_t bytes)
{
	if (!owns_buffer)
		return 0;
	if (buffer_used + bytes > buffer_size)
		if (!Reallocate(bytes + BUFFER_INCREMENTS))
			return 0;

	memmove(&buffer[bytes], &buffer[0], buffer_used);
	memcpy(buffer, _buffer, bytes);
	buffer_used += bytes;
	buffer_ptr += bytes;
	return bytes;
}

size_t StreamMemory::PopFront(size_t bytes)
{
	if (!owns_buffer)
		return 0;
	bytes = Math::Min(bytes, buffer_used);
	size_t position = Tell();
	Erase(0, bytes);
	buffer_ptr = buffer + (position > bytes ? position - bytes : 0);
	return bytes;
}

const byte* StreamMemory::RawStream() const
{
	return buffer;
}

void StreamMemory::Erase(size_t offset, size_t bytes)
{
	if (!owns_buffer || offset > buffer_used)
		return;
	bytes = Math::Min(bytes, buffer_used - offset);
	memmove(&buffer[offset], &buffer[offset + bytes], buffer_used - offset - bytes);
	buffer_used -= bytes;
}

bool StreamMemory::IsReadReady()
{
	return !IsEOS();
}

bool StreamMemory::IsWriteReady()
{
	return owns_buffer;
}

void StreamMemory::SetSourceURL(const URL& url)
{
	SetStreamDetails(url, Stream::MODE_READ | (owns_buffer ? Stream::MODE_WRITE : 0));
}

bool StreamMemory::Reallocate(size_t size)
{
	RMLUI_ASSERT(owns_buffer);
	if (!owns_buffer)
		return false;

	byte* new_buffer = (byte*)realloc(buffer, buffer_size + size);
	if (new_buffer == nullptr)
		return false;

	buffer_ptr = new_buffer + (buffer_ptr - buffer);

	buffer = new_buffer;
	buffer_size += size;

	return true;
}

} // namespace Rml
