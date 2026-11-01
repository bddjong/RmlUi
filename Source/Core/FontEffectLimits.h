#pragma once

#include "../../Include/RmlUi/Core/Types.h"
#include <cmath>
#include <cstdint>
#include <limits>

namespace Rml {
namespace FontEffectLimits {

// Bounds the quadratic outline kernel to about 258 KiB per effect.
constexpr int max_width = 128;

inline bool ConvertLength(float value, int& result)
{
	const double v = value;
	if (!std::isfinite(v) || v < double(std::numeric_limits<int>::min()) || v > double(std::numeric_limits<int>::max()))
		return false;
	result = int(value);
	return true;
}

inline bool ConvertWidth(float value, int& result)
{
	if (!std::isfinite(value) || value < -float(max_width) || value >= float(max_width + 1))
		return false;
	result = int(value);
	return true;
}

inline bool ExpandMetrics(Vector2i& origin, Vector2i& dimensions, int width, Vector2i offset = Vector2i(0))
{
	if (dimensions.x <= 0 || dimensions.y <= 0)
		return false;
	const int64_t x = int64_t(origin.x) + offset.x - width;
	const int64_t y = int64_t(origin.y) + offset.y - width;
	const int64_t w = int64_t(dimensions.x) + 2 * width;
	const int64_t h = int64_t(dimensions.y) + 2 * width;
	const int64_t lo = std::numeric_limits<int>::min(), hi = std::numeric_limits<int>::max();
	if (x < lo || x > hi || y < lo || y > hi || w > hi || h > hi)
		return false;
	origin = Vector2i(int(x), int(y));
	dimensions = Vector2i(int(w), int(h));
	return true;
}

} // namespace FontEffectLimits
} // namespace Rml
