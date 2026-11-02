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

if(UNIX AND NOT EMSCRIPTEN)
	find_package(Python3 COMPONENTS Interpreter QUIET)
	if(Python3_Interpreter_FOUND)
		add_test(NAME rmlui_tga_loader_mock COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/Backends/tests/tga_loader_mock.py")
		add_test(NAME rmlui_tga_upload_mock COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/Backends/tests/tga_upload_mock.py")
		add_test(NAME rmlui_gradient_regressions COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/Backends/tests/gradient_regression.py")
		add_test(NAME rmlui_sdl_gpu_frame_mock COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/Backends/tests/sdl_gpu_frame_mock.py")
		set_tests_properties(rmlui_sdl_gpu_frame_mock PROPERTIES ENVIRONMENT "CXX=${CMAKE_CXX_COMPILER}")
		if(CMAKE_OSX_SYSROOT)
			set_property(TEST rmlui_sdl_gpu_frame_mock APPEND PROPERTY ENVIRONMENT "SDKROOT=${CMAKE_OSX_SYSROOT}")
		endif()
		set_tests_properties(rmlui_tga_loader_mock rmlui_tga_upload_mock rmlui_gradient_regressions PROPERTIES ENVIRONMENT "CXX=${CMAKE_CXX_COMPILER}")
		if(CMAKE_OSX_SYSROOT)
			set_property(TEST rmlui_tga_loader_mock rmlui_tga_upload_mock rmlui_gradient_regressions APPEND PROPERTY ENVIRONMENT "SDKROOT=${CMAKE_OSX_SYSROOT}")
		endif()
	endif()
endif()

if(NOT EMSCRIPTEN)

	# These standalone regressions use assert for both checks and side effects.
	function(rmlui_regression_assertions target)
		if(MSVC)
			target_compile_options(${target} PRIVATE /UNDEBUG)
		else()
			target_compile_options(${target} PRIVATE -UNDEBUG)
		endif()
	endfunction()

	# This regression requires exceptions in both the test and the core library.
	include(CheckCXXSourceCompiles)
	check_cxx_source_compiles("#if !defined(__cpp_exceptions) && !defined(_CPPUNWIND)
#error Exceptions disabled
#endif
int main() { try { throw 1; } catch (...) {} }" RMLUI_TEST_HAS_EXCEPTIONS)
	if(RMLUI_TEST_HAS_EXCEPTIONS)
		add_executable(rmlui_inventory_failure_regressions
			"${PROJECT_SOURCE_DIR}/Samples/basic/drag/tests/InventoryFailure.cpp"
			"${PROJECT_SOURCE_DIR}/Samples/basic/drag/src/Inventory.cpp"
			"${PROJECT_SOURCE_DIR}/Samples/basic/drag/src/DragListener.cpp")
		target_compile_features(rmlui_inventory_failure_regressions PRIVATE cxx_std_17)
		target_compile_definitions(rmlui_inventory_failure_regressions PRIVATE NDEBUG INVENTORY_FAILURE_TEST)
		target_link_libraries(rmlui_inventory_failure_regressions PRIVATE rmlui_core)
		add_test(NAME rmlui_inventory_failure_regressions COMMAND rmlui_inventory_failure_regressions)
	endif()

	if(RMLUI_TEST_HAS_EXCEPTIONS AND NOT RMLUI_CUSTOM_CONFIGURATION_FILE AND NOT WIN32)
		add_executable(rmlui_variant_allocation_regressions "Source/VariantAllocation/main.cpp")
		target_link_libraries(rmlui_variant_allocation_regressions PRIVATE rmlui_core)
		target_compile_features(rmlui_variant_allocation_regressions PRIVATE cxx_std_17)
		rmlui_regression_assertions(rmlui_variant_allocation_regressions)
		add_test(NAME rmlui_variant_allocation_regressions COMMAND rmlui_variant_allocation_regressions)
	endif()

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

	foreach(name text_create_allocation text_constructor_allocation)
		set(target "rmlui_sheenbidi_${name}")
		add_executable(${target}
			"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/tests/${name}.c"
			"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Source/SheenBidi.c")
		target_include_directories(${target} PRIVATE
			"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Headers"
			"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Source")
		target_compile_definitions(${target} PRIVATE SB_CONFIG_UNITY SB_CONFIG_EXPERIMENTAL_TEXT_API)
		rmlui_regression_assertions(${target})
		add_test(NAME ${target} COMMAND ${target})
	endforeach()

	add_executable(rmlui_sheenbidi_attribute_copy_allocation
		"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/tests/attribute_copy_allocation.c")
	target_include_directories(rmlui_sheenbidi_attribute_copy_allocation PRIVATE
		"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Headers"
		"${PROJECT_SOURCE_DIR}/Dependencies/SheenBidi/Source")
	target_compile_definitions(rmlui_sheenbidi_attribute_copy_allocation PRIVATE SB_CONFIG_UNITY SB_CONFIG_EXPERIMENTAL_TEXT_API)
	rmlui_regression_assertions(rmlui_sheenbidi_attribute_copy_allocation)
	add_test(NAME rmlui_sheenbidi_attribute_copy_allocation COMMAND rmlui_sheenbidi_attribute_copy_allocation)

	add_executable(rmlui_lodepng_encoder_size_regressions
		"${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/lodepng/tests/encoder_size_regression.cpp")
	target_compile_features(rmlui_lodepng_encoder_size_regressions PRIVATE cxx_std_11)
	rmlui_regression_assertions(rmlui_lodepng_encoder_size_regressions)
	add_test(NAME rmlui_lodepng_encoder_size_regressions COMMAND rmlui_lodepng_encoder_size_regressions)

	if(TARGET RmlUi::External::Lua)
		foreach(name colourb colourf)
			set(target "rmlui_lua_${name}_regressions")
			add_executable(${target} "Lua/${name}.cpp"
				"${PROJECT_SOURCE_DIR}/Source/Lua/Colourb.cpp"
				"${PROJECT_SOURCE_DIR}/Source/Lua/Colourf.cpp"
				"${PROJECT_SOURCE_DIR}/Source/Lua/LuaType.cpp")
			target_compile_features(${target} PRIVATE cxx_std_17)
			target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/Include")
			target_link_libraries(${target} PRIVATE RmlUi::External::Lua)
			rmlui_regression_assertions(${target})
			add_test(NAME ${target} COMMAND ${target} "${CMAKE_CURRENT_SOURCE_DIR}/Lua/${name}.lua")
		endforeach()
	endif()

	find_path(RMLUI_TEST_VULKAN_INCLUDE_DIR vulkan/vulkan.h)
	if(RMLUI_TEST_VULKAN_INCLUDE_DIR)
		add_executable(rmlui_vulkan_linear_virtual_statistics "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Vulkan/tests/linear_virtual_statistics.cpp")
		target_compile_features(rmlui_vulkan_linear_virtual_statistics PRIVATE cxx_std_11)
		target_include_directories(rmlui_vulkan_linear_virtual_statistics PRIVATE "${RMLUI_TEST_VULKAN_INCLUDE_DIR}")
		rmlui_regression_assertions(rmlui_vulkan_linear_virtual_statistics)
		add_test(NAME rmlui_vulkan_linear_virtual_statistics COMMAND rmlui_vulkan_linear_virtual_statistics)
		find_package(Threads REQUIRED)
		add_executable(rmlui_vulkan_block_mapping_concurrency "${PROJECT_SOURCE_DIR}/Backends/RmlUi_Vulkan/tests/block_mapping_concurrency.cpp")
		target_compile_features(rmlui_vulkan_block_mapping_concurrency PRIVATE cxx_std_11)
		target_include_directories(rmlui_vulkan_block_mapping_concurrency PRIVATE "${RMLUI_TEST_VULKAN_INCLUDE_DIR}")
		target_link_libraries(rmlui_vulkan_block_mapping_concurrency PRIVATE Threads::Threads)
		rmlui_regression_assertions(rmlui_vulkan_block_mapping_concurrency)
		add_test(NAME rmlui_vulkan_block_mapping_concurrency COMMAND rmlui_vulkan_block_mapping_concurrency)
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
		add_test(NAME rmlui_gl3_framebuffer_regressions
			COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/Backends/tests/gl3_framebuffer_mock.py")
		set_tests_properties(rmlui_gl3_framebuffer_regressions PROPERTIES ENVIRONMENT "CXX=${CMAKE_CXX_COMPILER}")
		if(APPLE AND CMAKE_OSX_SYSROOT)
			set_property(TEST rmlui_gl3_framebuffer_regressions APPEND PROPERTY ENVIRONMENT "SDKROOT=${CMAKE_OSX_SYSROOT}")
		endif()
	endif()
endif()
