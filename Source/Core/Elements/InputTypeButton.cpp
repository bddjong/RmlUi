#include "InputTypeButton.h"
#include "../../../Include/RmlUi/Core/Elements/ElementForm.h"
#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"
#include "../../../Include/RmlUi/Core/Factory.h"

namespace Rml {

InputTypeButton::InputTypeButton(ElementFormControlInput* element) : InputType(element) {}

InputTypeButton::~InputTypeButton()
{
	if (value_element)
		element->RemoveChild(value_element);
}

bool InputTypeButton::IsSubmitted()
{
	// Buttons are never submitted.
	return false;
}

bool InputTypeButton::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	if (changed_attributes.find("value") != changed_attributes.end())
	{
		auto value = element->GetAttribute<String>("value", "");
		if (!value.empty() && !value_element)
		{
			ElementPtr text = Factory::InstanceElement(element, "#text", "", XMLAttributes());
			value_element = rmlui_dynamic_cast<ElementText*>(text.get());
			if (value_element)
				element->AppendChild(std::move(text), true);
		}

		if (value_element)
			value_element->SetText(value);

		return false;
	}
	return true;
}

void InputTypeButton::ProcessDefaultAction(Event& /*event*/) {}

bool InputTypeButton::GetIntrinsicDimensions(Vector2f& /*dimensions*/, float& /*ratio*/)
{
	return false;
}

} // namespace Rml
