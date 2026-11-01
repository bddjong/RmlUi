#include "PropertyParserRatio.h"
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cctype>

namespace Rml {

PropertyParserRatio::PropertyParserRatio() {}

PropertyParserRatio::~PropertyParserRatio() {}

bool PropertyParserRatio::ParseValue(Property& property, const String& value, const ParameterMap& /*parameters*/) const
{
	const char* cursor = value.c_str();
	auto parse_number = [&cursor](float& result) {
		while (std::isspace(static_cast<unsigned char>(*cursor))) ++cursor;
		char* end = nullptr;
		errno = 0;
		result = std::strtof(cursor, &end);
		if (end == cursor || errno == ERANGE || !std::isfinite(result) || result <= 0) return false;
		cursor = end;
		while (std::isspace(static_cast<unsigned char>(*cursor))) ++cursor;
		return true;
	};
	float first_value, second_value;
	if (!parse_number(first_value) || *cursor != '/') return false;
	++cursor;
	if (!parse_number(second_value) || *cursor != '\0') return false;

	property.value = Variant(Vector2f(first_value, second_value));
	property.unit = Unit::RATIO;

	return true;
}

} // namespace Rml
