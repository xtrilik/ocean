cmake_minimum_required(VERSION 3.10)
project(DummyModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the output directory for the library relative to the build directory
# This helps in locating the library for tests.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_library(dummy_module SHARED dummy_module.cpp)

# Ensure the module_loader.hpp path is correctly found.
# This assumes the dummy_module's CMakeLists.txt is processed in a context
# where the root directory of the "wave" project is known, or that
# this CMakeLists.txt is added via add_subdirectory() from a parent CMakeLists.txt
# that sets up include directories.

# Option 1: If this CMakeLists.txt is part of a larger CMake build for "wave"
# The include path for module_loader.hpp should ideally be set by the parent CMakeLists.
# target_include_directories(dummy_module PRIVATE ../../) # Relative path to wave root

# Option 2: If building this module somewhat standalone (e.g. for testing setup)
# you might need to specify a more direct or absolute path to core includes.
# For now, assuming it's part of a larger build where include paths are managed.
# The include in dummy_module.cpp is #include "../../core/moduleloader/module_loader.hpp"
# So, the compiler needs to know where to find the "wave" root or "wave/core".
# If this CMake is run from wave/tests/dummy_module/build, then ../../.. would be wave root.
target_include_directories(dummy_module PRIVATE ../../..) # Adjust if needed based on actual build structure


# Optional: Add compiler flags if needed (e.g., for visibility on GCC/Clang)
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    set_target_properties(dummy_module PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")
    # For exported symbols, use __attribute__((visibility("default"))) in source,
    # or rely on extern "C" and non-static methods.
    # For Windows __declspec(dllexport) is used in the source code.
endif()

# Post-build step to copy the module to a known location for testing, if needed.
# Example:
# add_custom_command(TARGET dummy_module POST_BUILD
# COMMAND ${CMAKE_COMMAND} -E copy_if_different
# $<TARGET_FILE:dummy_module>
# ${CMAKE_BINARY_DIR}/../test_modules/ # A common directory for test modules
# COMMENT "Copying dummy_module to test_modules directory"
# )
# This usually means the main test_module_loader.cpp would know to look in CMAKE_BINARY_DIR/../test_modules.
# For now, tests will likely look in CMAKE_LIBRARY_OUTPUT_DIRECTORY.
