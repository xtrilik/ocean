cmake_minimum_required(VERSION 3.10)
project(ClipboardModule LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the output directory for the library relative to the build directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(clipboard_module SHARED clipboard_module.cpp)

# Ensure ILauncherModule, ICoreAccess etc. are found.
# Assumes this CMakeLists.txt is part of a larger build for "wave".
# The includes in clipboard_module.cpp are like:
# #include "core/moduleloader/module_loader.hpp"
# #include "include/ICoreAccess.hpp" 
# These paths are relative to the "wave" root.
# If this CMake is run from wave/modules/clipboard/build, then ../../.. is wave root.
target_include_directories(clipboard_module PRIVATE 
    ../../.. # For wave root (e.g., wave/core, wave/include)
)

# Optional: Add compiler flags if needed
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    # Add flags like -fPIC if not automatically handled for SHARED libraries
    # set_target_properties(clipboard_module PROPERTIES COMPILE_FLAGS "-fvisibility=hidden") 
    # For exported symbols, __declspec(dllexport) is used in Windows,
    # and default visibility for extern "C" functions is usually sufficient on Linux/macOS.
endif()

# On Windows, clipboard operations link against User32.lib, typically part of default libs.
# No special linking for xclip/pbcopy as they are external commands.
if(WIN32)
    # target_link_libraries(clipboard_module PRIVATE User32.lib Shell32.lib Ole32.lib Gdi32.lib) 
    # Usually not needed explicitly for basic clipboard if using modern MinGW or MSVC toolchains
    # as they link common WinAPI libs by default. Let's keep it minimal.
endif()
