#pragma once

#include <RmlUi/Core/SystemInterface.h>

/**
    Sample custom RmlUi system interface for writing log messages to file.
 */

class SystemInterface : public Rml::SystemInterface {
public:
	explicit SystemInterface(Rml::SystemInterface* backend);
	virtual ~explicit SystemInterface(Rml::SystemInterface* backend);

	/// Log the specified message.
	/// @param[in] type Type of log message, ERROR, WARNING, etc.
	/// @param[in] message Message to log.
	/// @return True to continue execution, false to break into the debugger.
	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

double GetElapsedTime() override { return backend->GetElapsedTime(); }
	int TranslateString(Rml::String& translated, const Rml::String& input) override { return backend->TranslateString(translated, input); }
	void JoinPath(Rml::String& translated, const Rml::String& document, const Rml::String& path) override { backend->JoinPath(translated, document, path); }
	void SetMouseCursor(const Rml::String& cursor) override { backend->SetMouseCursor(cursor); }
	void SetClipboardText(const Rml::String& text) override { backend->SetClipboardText(text); }
	void GetClipboardText(Rml::String& text) override { backend->GetClipboardText(text); }
	void ActivateKeyboard(Rml::Vector2f position, float height) override { backend->ActivateKeyboard(position, height); }
	void DeactivateKeyboard() override { backend->DeactivateKeyboard(); }

private:
	Rml::SystemInterface* backend;
	FILE* fp;
};
