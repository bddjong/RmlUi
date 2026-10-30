set(SAMPLE_NAME "harfbuzz")
set(TARGET_NAME "${RMLUI_SAMPLE_PREFIX}${SAMPLE_NAME}")

set(HARFBUZZ_ENGINE_SOURCES
	src/ClusterRange.h
	src/FontEngineInterfaceHarfBuzz.cpp
	src/FontEngineInterfaceHarfBuzz.h
	src/FontFace.cpp
	src/FontFace.h
	src/FontFaceHandleHarfBuzz.cpp
	src/FontFaceHandleHarfBuzz.h
	src/FontFaceLayer.cpp
	src/FontFaceLayer.h
	src/FontFamily.cpp
	src/FontFamily.h
	src/FontGlyph.h
	src/FontProvider.cpp
	src/FontProvider.h
	src/FreeTypeInterface.cpp
	src/FreeTypeInterface.h
	src/LanguageData.h
	src/TextureLayout.cpp
	src/TextureLayout.h
	src/TextureLayoutRectangle.cpp
	src/TextureLayoutRectangle.h
	src/TextureLayoutRow.cpp
	src/TextureLayoutRow.h
	src/TextureLayoutTexture.cpp
	src/TextureLayoutTexture.h
)

add_executable(${TARGET_NAME} WIN32 ${HARFBUZZ_ENGINE_SOURCES} src/main.cpp)

set_common_target_options(${TARGET_NAME})

target_link_libraries(${TARGET_NAME} PRIVATE rmlui_shell Freetype::Freetype harfbuzz::harfbuzz)

# Use private Core headers as a workaround for now, until we merge the HarfBuzz font engine into Core.
target_include_directories(${TARGET_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/Source/Core ${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Headers)
# Remove this check once the HarfBuzz font engine has been merged into Core.
if(WIN32 AND BUILD_SHARED_LIBS)
	message(FATAL_ERROR "The HarfBuzz sample cannot be built when using shared libraries on Windows. Please disable either BUILD_SHARED_LIBS or RMLUI_HARFBUZZ_SAMPLE.")
endif()

install_sample_target(${TARGET_NAME})

if(BUILD_TESTING)
	add_executable(rmlui_harfbuzz_paragraph_test ${HARFBUZZ_ENGINE_SOURCES} src/ParagraphBidiTest.cpp)
	set_common_target_options(rmlui_harfbuzz_paragraph_test)
	target_link_libraries(rmlui_harfbuzz_paragraph_test PRIVATE rmlui_core Freetype::Freetype harfbuzz::harfbuzz)
	target_include_directories(rmlui_harfbuzz_paragraph_test PRIVATE ${PROJECT_SOURCE_DIR}/Source/Core ${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Headers)
	add_test(NAME harfbuzz_paragraph_layout COMMAND rmlui_harfbuzz_paragraph_test ${PROJECT_SOURCE_DIR}/Samples/basic/harfbuzz/data/Cairo-Regular.ttf)
endif()
