# Enable some extra testing code in the core library.
target_compile_definitions(rmlui_core PUBLIC RMLUI_TESTS_ENABLED)

include("${PROJECT_SOURCE_DIR}/CMake/DependenciesForTests.cmake")

if(EMSCRIPTEN)
	# Add data for tests to emscripten targets
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sLLD_REPORT_UNDEFINED")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${CMAKE_CURRENT_SOURCE_DIR}/Data/@/Tests/Data/\"")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file \"${PROJECT_SOURCE_DIR}/Samples/basic/demo/data/@/basic/demo/data/\"")
endif()

add_subdirectory("Source")

if(NOT EMSCRIPTEN)
	# These standalone regressions use assert for both checks and side effects.
	function(rmlui_regression_assertions target)
		if(MSVC)
			target_compile_options(${target} PRIVATE /UNDEBUG)
		else()
			target_compile_options(${target} PRIVATE -UNDEBUG)
		endif()
	endfunction()

	add_executable(rmlui_memory_allocator_regressions
		"${PROJECT_SOURCE_DIR}/Source/Core/Memory.cpp"
		"${PROJECT_SOURCE_DIR}/Source/Core/tests/memory_allocator_test.cpp")
	target_compile_features(rmlui_memory_allocator_regressions PRIVATE cxx_std_17)
	target_compile_definitions(rmlui_memory_allocator_regressions PRIVATE RMLUI_STATIC_LIB)
	rmlui_regression_assertions(rmlui_memory_allocator_regressions)
	add_test(NAME rmlui_memory_allocator_regressions COMMAND rmlui_memory_allocator_regressions)

	add_executable(rmlui_nanobench_export_regressions "Dependencies/nanobench/tests/export_regressions.cpp")
	target_compile_features(rmlui_nanobench_export_regressions PRIVATE cxx_std_11)
	rmlui_regression_assertions(rmlui_nanobench_export_regressions)
	add_test(NAME rmlui_nanobench_export_regressions COMMAND rmlui_nanobench_export_regressions)

	foreach(name regressions disabled_message_regressions)
		set(target "rmlui_doctest_${name}")
		add_executable(${target} "Dependencies/doctest/tests/${name}.cpp")
		target_compile_features(${target} PRIVATE cxx_std_11)
		rmlui_regression_assertions(${target})
		add_test(NAME ${target} COMMAND ${target})
	endforeach()

	add_executable(rmlui_sheenbidi_text_iterator_regressions
		"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/tests/text_iterator_regressions.c"
		"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Source/SheenBidi.c")
	target_include_directories(rmlui_sheenbidi_text_iterator_regressions PRIVATE
		"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Headers"
		"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Source")
	target_compile_definitions(rmlui_sheenbidi_text_iterator_regressions PRIVATE SB_CONFIG_UNITY SB_CONFIG_EXPERIMENTAL_TEXT_API)
	rmlui_regression_assertions(rmlui_sheenbidi_text_iterator_regressions)
	add_test(NAME rmlui_sheenbidi_text_iterator_regressions COMMAND rmlui_sheenbidi_text_iterator_regressions)

	find_path(RMLUI_TEST_VULKAN_INCLUDE_DIR vulkan/vulkan.h)
	if(RMLUI_TEST_VULKAN_INCLUDE_DIR)
		add_executable(rmlui_vulkan_linear_virtual_statistics "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Vulkan/tests/linear_virtual_statistics.cpp")
		target_compile_features(rmlui_vulkan_linear_virtual_statistics PRIVATE cxx_std_11)
		target_include_directories(rmlui_vulkan_linear_virtual_statistics PRIVATE "${RMLUI_TEST_VULKAN_INCLUDE_DIR}")
		rmlui_regression_assertions(rmlui_vulkan_linear_virtual_statistics)
		add_test(NAME rmlui_vulkan_linear_virtual_statistics COMMAND rmlui_vulkan_linear_virtual_statistics)
	else()
		message(STATUS "Vulkan headers not found; skipping rmlui_vulkan_linear_virtual_statistics")
	endif()
endif()

# The shader publication regression uses an executable fake compiler with a POSIX shebang.
if(UNIX AND NOT EMSCRIPTEN)
	find_package(Python3 COMPONENTS Interpreter QUIET)
	if(Python3_Interpreter_FOUND)
		add_test(NAME rmlui_sdl_gpu_shader_publication
			COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/Backends/RmlUi_SDL_GPU/tests/test_compile_shaders.py")
	endif()
endif()
