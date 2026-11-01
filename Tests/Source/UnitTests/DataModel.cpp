#include "../../../Source/Core/DataModel.cpp"
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>
#include <doctest.h>

using namespace Rml;

TEST_CASE("Data variables")
{
	{
		const DataAddress address = ParseAddress("data.items[12][0].value");
		REQUIRE(address.size() == 5);
		CHECK(address[0].name == "data");
		CHECK(address[1].name == "items");
		CHECK(address[2].index == 12);
		CHECK(address[3].index == 0);
		CHECK(address[4].name == "value");
		CHECK(ParseAddress("data.items[]").empty());
		CHECK(ParseAddress("data.items[12x]").empty());
		CHECK(ParseAddress("data.items[-1]").empty());
	}

	using IntVector = Vector<int>;

	struct FunData {
		int i = 99;
		String x = "hello";
		IntVector magic = {3, 5, 7, 11, 13};
	};

	using FunArray = Array<FunData, 3>;

	struct SmartData {
		bool valid = true;
		FunData fun;
		FunArray more_fun;
	};

	DataTypeRegister types;
	DataModel model(&types);

	DataModelConstructor handle(&model);

	// Setup data type register
	{
		handle.RegisterArray<IntVector>();

		if (auto fun_handle = handle.RegisterStruct<FunData>())
		{
			fun_handle.RegisterMember("i", &FunData::i);
			fun_handle.RegisterMember("x", &FunData::x);
			fun_handle.RegisterMember("magic", &FunData::magic);
		}

		handle.RegisterArray<FunArray>();

		if (auto smart_handle = handle.RegisterStruct<SmartData>())
		{
			smart_handle.RegisterMember("valid", &SmartData::valid);
			smart_handle.RegisterMember("fun", &SmartData::fun);
			smart_handle.RegisterMember("more_fun", &SmartData::more_fun);
		}
	}

	SmartData data;
	data.fun.x = "Hello, we're in SmartData!";

	handle.Bind("data", &data);

	// Test data addresses, setters, and assignments
	{
		Vector<String> test_addresses = {"data.more_fun[1].magic[3]", "data.more_fun[1].magic.size", "data.fun.x", "data.valid"};
		Vector<String> expected_results = {ToString(data.more_fun[1].magic[3]), ToString(int(data.more_fun[1].magic.size())), ToString(data.fun.x),
			ToString(data.valid)};

		Vector<String> results;

		for (auto& str_address : test_addresses)
		{
			DataAddress address = ParseAddress(str_address);

			Variant result;
			if (model.GetVariableInto(address, result))
				results.push_back(result.Get<String>());
		}

		CHECK(results == expected_results);

		auto magic_entry = model.GetVariable(ParseAddress("data.more_fun[1].magic[1]"));
		REQUIRE(magic_entry);
		REQUIRE(magic_entry.Set(Variant(String("199"))));
		CHECK(data.more_fun[1].magic[1] == 199);

		data.fun.magic = {99, 190, 55, 2000, 50, 60, 70, 80, 90};

		Variant get_result;

		const int magic_size = int(data.fun.magic.size());
		REQUIRE(model.GetVariable(ParseAddress("data.fun.magic.size")).Get(get_result));
		CHECK(get_result.Get<String>() == ToString(magic_size));
		CHECK(model.GetVariable(ParseAddress("data.fun.magic")).Size() == magic_size);

		REQUIRE(model.GetVariable(ParseAddress("data.fun.magic[8]")).Get(get_result));
		CHECK(get_result.Get<String>() == "90");
	}
}

struct CustomArray {
	Vector<int> data;

	size_t GetNum() const { return data.size(); }
	int* GetData() { return data.data(); }

	int operator[](size_t i) const { return data[i]; }
};

size_t size(const CustomArray& self) { return self.GetNum(); }
int* begin(CustomArray& self) { return self.GetData(); }

TEST_CASE("Data variables.non_standard_array_interface")
{
	DataTypeRegister types;
	DataModel model(&types);

	DataModelConstructor handle(&model);

	CustomArray custom_array = {{1, 1, 2, 3, 5}};

	handle.RegisterArray<CustomArray>();
	handle.Bind("custom_array", &custom_array);

	{
		Variant result;
		model.GetVariableInto(ParseAddress("custom_array[3]"), result);
		CHECK(result.Get<int>() == custom_array[3]);
		model.GetVariableInto(ParseAddress("custom_array.size"), result);
		CHECK(result.Get<int>() == int(custom_array.GetNum()));

		auto entry = model.GetVariable(ParseAddress("custom_array[1]"));
		REQUIRE(entry);
		REQUIRE(entry.Set(Variant(String("199"))));
		CHECK(custom_array[1] == 199);
	}
}


TEST_CASE("Data variables.null_pointer")
{
	struct CountingDefinition : VariableDefinition {
		int calls = 0;
		CountingDefinition() : VariableDefinition(DataVariableType::Scalar) {}
		bool Get(void*, Variant&) override { ++calls; return true; }
		bool Set(void*, const Variant&) override { ++calls; return true; }
		int Size(void*) override { ++calls; return 1; }
		DataVariable Child(void*, const DataAddressEntry&) override { ++calls; return {}; }
	} underlying;
	PointerDefinition<int*> pointer(&underlying);
	int* value = nullptr;
	Variant result;
	CHECK_FALSE(pointer.Get(&value, result));
	CHECK_FALSE(pointer.Set(&value, result));
	CHECK(pointer.Size(&value) == 0);
	pointer.Child(&value, DataAddressEntry(0));
	CHECK(underlying.calls == 0);
	int target = 1;
	value = &target;
	CHECK(pointer.Get(&value, result));
	CHECK(underlying.calls == 1);
}
