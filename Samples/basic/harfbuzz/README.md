# HarfBuzz text shaping sample

The sample installs `FontEngineInterfaceHarfBuzz` before `Rml::Initialise()`.
It shapes text with HarfBuzz and uses paragraph-level bidirectional layout from
RmlUi core, including styled runs and wrapped text. Load an appropriate font
for each script, and register fallback faces when text can contain scripts the
primary face does not cover.

The default FreeType font engine does not shape Arabic; applications must
install a shaping font engine to obtain contextual forms and ligatures. Merely
setting `dir="rtl"` is not sufficient. This sample engine is an example that
applications can adapt; it is not installed automatically by RmlUi.

Enable `RMLUI_SAMPLES` and `RMLUI_HARFBUZZ_SAMPLE` with HarfBuzz and FreeType
available to build the sample. With `BUILD_TESTING` enabled, the
`harfbuzz_paragraph_layout` CTest runs a headless layout and geometry check for
mixed-direction spans, an inline object and a ligature across a colour change.
