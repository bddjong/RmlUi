#include "Inventory.h"
#include "DragListener.h"
#include <RmlUi/Core/Factory.h>

#ifdef INVENTORY_FAILURE_TEST
extern void InventorySetupCheckpoint(int stage);
#endif

Inventory::Inventory(const Rml::String& title, const Rml::Vector2f& position, Rml::Context* context, const Rml::String& path)
{
	document = nullptr;
	if (!context)
		return;
	auto close_document = [](Rml::ElementDocument* value) { if (value) value->Close(); };
	std::unique_ptr<Rml::ElementDocument, decltype(close_document)> pending(context->LoadDocument(path), close_document);
	auto* loaded = pending.get();
	if (loaded != nullptr)
	{
#ifdef INVENTORY_FAILURE_TEST
		InventorySetupCheckpoint(1);
#endif
		using Rml::PropertyId;
		if (auto* element = loaded->GetElementById("title"))
			element->SetInnerRML(title);
		loaded->SetProperty(PropertyId::Left, Rml::Property(position.x, Rml::Unit::DP));
		loaded->SetProperty(PropertyId::Top, Rml::Property(position.y, Rml::Unit::DP));
		loaded->Show();

		if (auto* content = loaded->GetElementById("content"))
			DragListener::RegisterDraggableContainer(content);
#ifdef INVENTORY_FAILURE_TEST
		InventorySetupCheckpoint(2);
#endif
		document = pending.release();
	}
}

Inventory::~Inventory()
{
	if (document != nullptr)
	{
		document->Close();
	}
}

void Inventory::AddItem(const Rml::String& name)
{
	if (document == nullptr)
		return;

	Rml::Element* content = document->GetElementById("content");
	if (content == nullptr)
		return;

	// Create the new 'icon' element.
	Rml::ElementPtr icon = Rml::Factory::InstanceElement(content, "icon", "icon", Rml::XMLAttributes());
	if (icon)
		content->AppendChild(std::move(icon))->SetInnerRML(name);
}

bool InitializeInventories(Rml::Context* context, std::unique_ptr<Inventory>& first, std::unique_ptr<Inventory>& second,
	const Rml::String& first_path, const Rml::String& second_path)
{
	if (&first == &second)
		return false;
	first.reset();
	second.reset();
	auto first_window = std::make_unique<Inventory>("Inventory 1", Rml::Vector2f(50, 200), context, first_path);
	if (!first_window->IsInitialized())
		return false;
	auto second_window = std::make_unique<Inventory>("Inventory 2", Rml::Vector2f(540, 240), context, second_path);
	if (!second_window->IsInitialized())
		return false;
	first = std::move(first_window);
	second = std::move(second_window);
	return true;
}
