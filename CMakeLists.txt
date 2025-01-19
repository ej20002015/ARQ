cmake_minimum_required(VERSION 3.15)

# Project name
project(TMQLib)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow for folders in the IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# gtest setup
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)
enable_testing()
include(GoogleTest)
set_target_properties( gmock PROPERTIES FOLDER "vendor")
set_target_properties( gmock_main PROPERTIES FOLDER "vendor")
set_target_properties( gtest PROPERTIES FOLDER "vendor")
set_target_properties( gtest_main PROPERTIES FOLDER "vendor")

# Add subdirectories
add_subdirectory(TMQCore)
