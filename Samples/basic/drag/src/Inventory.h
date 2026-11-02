#pragma once

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Types.h>
#include <memory>

class Inventory {
public:
	/// Constructs a new inventory and opens its window.
	/// @param[in] title The title of the new inventory.
	/// @param[in] position The position of the inventory window.
	/// @param[in] context The context to open the inventory window in.
	Inventory(const Rml::String& title, const Rml::Vector2f& position, Rml::Context* context,
		const Rml::String& path = "basic/drag/data/inventory.rml");
	/// Destroys the inventory and closes its window.
	~Inventory();
	Inventory(const Inventory&) = delete;
	Inventory& operator=(const Inventory&) = delete;

	bool IsInitialized() const { return document != nullptr; }

	/// Adds a brand-new item into this inventory.
	/// @param[in] name The name of this item.
	void AddItem(const Rml::String& name);

private:
	Rml::ElementDocument* document;
};

// Creates both windows as one operation; a failed second load closes the first.
bool InitializeInventories(Rml::Context* context, std::unique_ptr<Inventory>& first, std::unique_ptr<Inventory>& second,
	const Rml::String& first_path = "basic/drag/data/inventory.rml",
	const Rml::String& second_path = "basic/drag/data/inventory.rml");
