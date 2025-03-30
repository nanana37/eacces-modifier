cmake_minimum_required(VERSION 3.10)
project(Permod LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find LLVM and Clang once
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

# Include LLVM CMake helpers
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# Apply LLVM global settings
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Find our modules
include_directories(include)
add_subdirectory(Macker/macker)
add_subdirectory(Permod/permod)
add_subdirectory(Permod/rtlib)