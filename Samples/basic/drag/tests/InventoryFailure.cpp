// Regression coverage for inventory initialization and item creation.
#include "../src/Inventory.h"
#include <RmlUi/Core.h>
#include <cstdio>
#include <stdexcept>
#include <sstream>

class Renderer : public Rml::RenderInterface {
public:
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override { return 0; }
	void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
	void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
	Rml::TextureHandle LoadTexture(Rml::Vector2i&, const Rml::String&) override { return 0; }
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override { return 0; }
	void ReleaseTexture(Rml::TextureHandle) override {}
	void EnableScissorRegion(bool) override {}
	void SetScissorRegion(Rml::Rectanglei) override {}
};

class FailingIcon : public Rml::ElementInstancer {
public:
	bool called = false;
	Rml::ElementPtr InstanceElement(Rml::Element*, const Rml::String&, const Rml::XMLAttributes&) override
	{
		called = true;
		return nullptr;
	}
	void ReleaseElement(Rml::Element*) override { throw std::runtime_error("Unexpected ReleaseElement"); }
};

#define CHECK(condition) do { if (!(condition)) throw std::runtime_error("Check failed: " #condition); } while (false)

class MemoryFiles : public Rml::FileInterface {
public:
	Rml::String contents;
	Rml::FileHandle Open(const Rml::String& path) override
	{
		if (path != "inventory.rml") return 0;
		return reinterpret_cast<Rml::FileHandle>(new std::istringstream(contents));
	}
	void Close(Rml::FileHandle file) override { delete reinterpret_cast<std::istringstream*>(file); }
	size_t Read(void* buffer, size_t size, Rml::FileHandle file) override
	{
		auto& stream = *reinterpret_cast<std::istringstream*>(file);
		stream.read(static_cast<char*>(buffer), static_cast<std::streamsize>(size));
		return static_cast<size_t>(stream.gcount());
	}
	bool Seek(Rml::FileHandle file, long offset, int origin) override
	{
		auto& stream = *reinterpret_cast<std::istringstream*>(file);
		stream.clear();
		stream.seekg(offset, origin == SEEK_SET ? std::ios::beg : origin == SEEK_CUR ? std::ios::cur : std::ios::end);
		return !stream.fail();
	}
	size_t Tell(Rml::FileHandle file) override { return static_cast<size_t>(reinterpret_cast<std::istringstream*>(file)->tellg()); }
};

static int fail_stage = 0;
static int fail_occurrence = 0;
struct InjectedSetupFailure {};
void InventorySetupCheckpoint(int stage)
{
	if (stage == fail_stage && --fail_occurrence == 0) throw InjectedSetupFailure{};
}

int main()
{
	try
	{
		const Rml::String path = "inventory.rml", missing = "missing.rml";
		MemoryFiles files;
		Rml::SetFileInterface(&files);
		Renderer renderer;
		FailingIcon failing_icon;
		Rml::SetRenderInterface(&renderer);
		CHECK(Rml::Initialise());
		auto* context = Rml::CreateContext("test", {100, 100});
		CHECK(context);
		std::unique_ptr<Inventory> first, second;
		CHECK(!InitializeInventories(context, first, second, missing, missing));
		CHECK(!first && !second && !context->GetDocument(0));
		files.contents = "<rml><body><div id='title'></div><div id='content'></div></body></rml>";
		for (int stage : {1, 2})
		{
			for (int occurrence : {1, 2})
			{
				fail_stage = stage; fail_occurrence = occurrence;
				bool threw = false;
				try { InitializeInventories(context, first, second, path, path); }
				catch (const InjectedSetupFailure&) { threw = true; }
				CHECK(threw && !first && !second && !context->GetDocument(0));
				context->Update();
			}
		}
		fail_stage = 0;
		CHECK(!InitializeInventories(nullptr, first, second, path, path));
		CHECK(!InitializeInventories(context, first, first, path, path));
		CHECK(!InitializeInventories(context, first, second, path, missing));
		CHECK(!first && !second && !context->GetDocument(0));
		context->Update();
		files.contents = "<rml><body><div id='content'></div></body></rml>";
		{
			Inventory missing_title("missing title", {}, context, path);
			CHECK(missing_title.IsInitialized());
			missing_title.AddItem("item");
			CHECK(context->GetDocument(0)->GetElementById("content")->GetNumChildren() == 1);
		}
		context->Update();
		CHECK(InitializeInventories(context, first, second, path, path));
		CHECK(first->IsInitialized() && second->IsInitialized());
		auto* content = context->GetDocument(0)->GetElementById("content");
		CHECK(content);
		first->AddItem("item");
		CHECK(content->GetNumChildren() == 1);
		Rml::Factory::RegisterElementInstancer("icon", &failing_icon);
		first->AddItem("failed");
		CHECK(failing_icon.called && content->GetNumChildren() == 1);
		first.reset(); second.reset();
		context->Update();
		files.contents = "<rml><body></body></rml>";
		{
			Inventory missing_elements("missing elements", {}, context, path);
			CHECK(missing_elements.IsInitialized());
			missing_elements.AddItem("item");
		}
		context->Update();
		Rml::Shutdown();
	}
	catch (const std::exception& error)
	{
		std::fprintf(stderr, "%s\n", error.what());
		return 1;
	}
	return 0;
}
