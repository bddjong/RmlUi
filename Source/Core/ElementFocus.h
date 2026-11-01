#pragma once

#include "../../Include/RmlUi/Core/Elements/ElementFormControl.h"

namespace Rml {

// Disabled controls suppress focus throughout their subtree without changing authored styles.
inline bool IsDisabledForFocus(Element* element)
{
	for (; element; element = element->GetParentNode())
	{
		if (auto* control = rmlui_dynamic_cast<ElementFormControl*>(element))
			if (control->IsDisabled())
				return true;
	}
	return false;
}

} // namespace Rml
