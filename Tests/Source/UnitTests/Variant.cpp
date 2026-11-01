#include <RmlUi/Core/Types.h>
#include <RmlUi/Core/DecorationTypes.h>
#include <RmlUi/Core/TypeConverter.h>
#include <limits>
#include <RmlUi/Core/Variant.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Variant.ScopedEnum")
{
	enum class X : uint64_t {
		A = 1,
		B = 5,
		C = UINT64_MAX,
	};

	X e1 = X::A;
	const X e2 = X::B;
	const X& e3 = e1;

	Variant v1 = Variant(X::A);
	Variant v2(X::B);
	Variant v3(X::C);

	Variant v4(e1);
	Variant v5(e2);
	Variant v6(e3);

	CHECK(v1.Get<X>() == X::A);
	CHECK(v2.Get<X>() == X::B);
	CHECK(v3.Get<X>() == X::C);

	CHECK(v4.Get<X>() == X::A);
	CHECK(v5.Get<X>() == X::B);
	CHECK(v6.Get<X>() == X::A);

	CHECK(v1 != v2);
	CHECK(v1 == v4);

	Variant v7 = v5;
	CHECK(v7.Get<X>() == X::B);

	CHECK(v1.Get<int>() == 1);
	CHECK(v2.Get<int>() == 5);
	CHECK(v3.Get<uint64_t>() == UINT64_MAX);
	CHECK(v3.Get<int64_t>() == static_cast<int64_t>(UINT64_MAX));
}

TEST_CASE("Variant.UnscopedEnum")
{
	enum X : uint64_t {
		A = 1,
		B = 5,
		C = UINT64_MAX,
	};

	X e1 = X::A;
	const X e2 = X::B;
	const X& e3 = e1;

	Variant v1 = Variant(X::A);
	Variant v2(X::B);
	Variant v3(X::C);

	Variant v4(e1);
	Variant v5(e2);
	Variant v6(e3);

	CHECK(v1.Get<X>() == X::A);
	CHECK(v2.Get<X>() == X::B);
	CHECK(v3.Get<X>() == X::C);

	CHECK(v4.Get<X>() == X::A);
	CHECK(v5.Get<X>() == X::B);
	CHECK(v6.Get<X>() == X::A);

	CHECK(v1 != v2);
	CHECK(v1 == v4);

	Variant v7 = v5;
	CHECK(v7.Get<X>() == X::B);

	CHECK(v1.Get<int>() == 1);
	CHECK(v2.Get<int>() == 5);
	CHECK(v3.Get<uint64_t>() == UINT64_MAX);
	CHECK(v3.Get<int64_t>() == static_cast<int64_t>(UINT64_MAX));
}

TEST_CASE("Variant assignment from stored reference")
{
	Variant value(String(128, 'x'));
	value = value.GetReference<String>();
	CHECK(value.Get<String>() == String(128, 'x'));
}

namespace {
String IncrementDecimal(String value)
{
	for (size_t i = value.size(); i > 0; --i)
	{
		if (value[i - 1] != '9')
		{
			++value[i - 1];
			return value;
		}
		value[i - 1] = '0';
	}
	return "1" + value;
}

template <typename T>
void CheckNumericIntegerConversion()
{
	const T minimum = std::numeric_limits<T>::min();
	const T maximum = std::numeric_limits<T>::max();
	const String min_string = std::numeric_limits<T>::is_signed ? std::to_string(static_cast<long long>(minimum)) : "0";
	const String max_string = std::to_string(static_cast<unsigned long long>(maximum));
	T value = 42;
	CHECK((TypeConverter<String, T>::Convert(min_string, value)));
	CHECK(value == minimum);
	CHECK((TypeConverter<String, T>::Convert(max_string, value)));
	CHECK(value == maximum);
	value = 42;
	CHECK_FALSE((TypeConverter<String, T>::Convert(IncrementDecimal(max_string), value)));
	CHECK(value == 42);
	if (std::numeric_limits<T>::is_signed)
	{
		CHECK_FALSE((TypeConverter<String, T>::Convert("-" + IncrementDecimal(min_string.substr(1)), value)));
		CHECK(value == 42);
	}
	for (const String& input : {String(""), String("12x"), String("1.0"), String("999999999999999999999999999999999999"),
		String("-999999999999999999999999999999999999"), String("1\0x", 3)})
	{
		value = 42;
		CHECK_FALSE((TypeConverter<String, T>::Convert(input, value)));
		CHECK(value == 42);
	}
	value = 42;
	CHECK((TypeConverter<String, T>::Convert(" \t+12 \n", value)));
	CHECK(value == 12);
	if (!std::numeric_limits<T>::is_signed)
	{
		value = 42;
		CHECK_FALSE((TypeConverter<String, T>::Convert("-1", value)));
		CHECK(value == 42);
	}
}

template <typename T>
void CheckNumericFloatConversion()
{
	T value = 42;
	CHECK((TypeConverter<String, T>::Convert(" \t-1.25e2 \n", value)));
	CHECK(value == T(-125));
	for (const String& input : {String(""), String("no"), String("1e"), String("1px"), String("nan"), String("-inf"),
		String("1e9999"), String("1e-9999"), String("1\0x", 3)})
	{
		value = 42;
		CHECK_FALSE((TypeConverter<String, T>::Convert(input, value)));
		CHECK(value == T(42));
	}
}
} // namespace

TEST_CASE("TypeConverter.StringVectorFailurePreservesDestination")
{
	Vector3i value(7, 8, 9);
	CHECK_FALSE((TypeConverter<String, Vector3i>::Convert("1, bad, 3", value)));
	CHECK(value == Vector3i(7, 8, 9));
	CHECK((TypeConverter<String, Vector3i>::Convert("1, 2, 3", value)));
	CHECK(value == Vector3i(1, 2, 3));
}

TEST_CASE("TypeConverter.StringNumericBoundaries")
{
	CheckNumericIntegerConversion<int>();
	CheckNumericIntegerConversion<unsigned int>();
	CheckNumericIntegerConversion<long>();
	CheckNumericIntegerConversion<unsigned long>();
	CheckNumericIntegerConversion<long long>();
	CheckNumericIntegerConversion<unsigned long long>();
	CheckNumericIntegerConversion<byte>();
	CheckNumericFloatConversion<float>();
	CheckNumericFloatConversion<double>();
	byte small = 7;
	CHECK_FALSE((TypeConverter<String, byte>::Convert("256", small)));
	CHECK(small == 7);
	float f = 7;
	CHECK_FALSE((TypeConverter<String, float>::Convert("3.5e38", f)));
	CHECK(f == 7);
	double d = 7;
	CHECK((TypeConverter<String, double>::Convert("3.5e38", d)));
}

namespace {
template <typename T>
void CheckVariantLifetime(const T& input)
{
	Variant value(input);
	CHECK(reinterpret_cast<uintptr_t>(&value.GetReference<T>()) % alignof(T) == 0);
	Variant copy(value);
	CHECK(copy == value);
	Variant moved(std::move(copy));
	CHECK(moved == value);
	copy = value;
	CHECK(copy == value);
	copy = 17;
	copy = value;
	CHECK(copy == value);
	copy.Clear();
	CHECK(copy.GetType() == Variant::NONE);
}
}

TEST_CASE("Variant.StorageLifetimeAndAlignment")
{
	CheckVariantLifetime(bool(true));
	CheckVariantLifetime(byte(12));
	CheckVariantLifetime(char('a'));
	CheckVariantLifetime(float(1));
	CheckVariantLifetime(double(2));
	CheckVariantLifetime(int(3));
	CheckVariantLifetime(int64_t(4));
	CheckVariantLifetime(unsigned(5));
	CheckVariantLifetime(uint64_t(6));
	CheckVariantLifetime(String(128, 'x'));
	CheckVariantLifetime(Vector2f(1, 2));
	CheckVariantLifetime(Vector3f(1, 2, 3));
	CheckVariantLifetime(Vector4f(1, 2, 3, 4));
	CheckVariantLifetime(Colourf(1, 0, 1, 1));
	CheckVariantLifetime(Colourb(1, 2, 3, 4));
	CheckVariantLifetime(static_cast<ScriptInterface*>(nullptr));
	CheckVariantLifetime(static_cast<void*>(nullptr));
	CheckVariantLifetime(TransformPtr{});
	CheckVariantLifetime(TransitionList{});
	CheckVariantLifetime(AnimationList{});
	CheckVariantLifetime(DecoratorsPtr{});
	CheckVariantLifetime(FiltersPtr{});
	CheckVariantLifetime(FontEffectsPtr{});
	CheckVariantLifetime(ColorStopList{});
	CheckVariantLifetime(BoxShadowList{});
}
