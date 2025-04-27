cmake_minimum_required(VERSION 3.23)

message(STATUS "CMake Generator: ${CMAKE_GENERATOR}")

# Project name
project(TMQLib)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow for folders in the IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Linux with default to prepending with prefix "lib" - don't want that
set(CMAKE_SHARED_LIBRARY_PREFIX "")

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
set_target_properties( gmock PROPERTIES FOLDER "vendor/gtest_gmock")
set_target_properties( gmock_main PROPERTIES FOLDER "vendor/gtest_gmock")
set_target_properties( gtest PROPERTIES FOLDER "vendor/gtest_gmock")
set_target_properties( gtest_main PROPERTIES FOLDER "vendor/gtest_gmock")

# Google benchmark setup
set(BENCHMARK_ENABLE_TESTING OFF)  # Disable Google Benchmark's own tests
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
)
FetchContent_MakeAvailable(benchmark)
set_target_properties( benchmark PROPERTIES FOLDER "vendor/gbenchmark")
set_target_properties( benchmark_main PROPERTIES FOLDER "vendor/gbenchmark")

# Utility functions

# Not using clang-tidy for now
# function(set_clang_tidy_properties TARGET)
#     set(CLANG_TIDY_CFG_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy")

#     set_target_properties(${TARGET} PROPERTIES
#         VS_GLOBAL_RunCodeAnalysis false

#         # Use visual studio core guidelines
#         VS_GLOBAL_EnableMicrosoftCodeAnalysis false
#         #VS_GLOBAL_CodeAnalysisRuleSet ${CMAKE_CURRENT_SOURCE_DIR}/foo.ruleset
#         #VS_GLOBAL_CodeAnalysisRuleSet ${CMAKE_CURRENT_SOURCE_DIR}/foo.ruleset

#         # Use clangtidy
#         VS_GLOBAL_EnableClangTidyCodeAnalysis true
#         VS_GLOBAL_ClangTidyChecks "-* \"\"--config-file=${CLANG_TIDY_CFG_PATH} --header-filter=.*" 
#     )
# endfunction()

set(MASTER_INC_DIR "${CMAKE_BINARY_DIR}/inc")

# Function to create symlinks in the 'inc/' directory for all TMQ* libraries' 'inc' directories
function(create_master_inc_dir)
    set(INC_DIR ${MASTER_INC_DIR})
    if (NOT EXISTS ${INC_DIR})
        file(MAKE_DIRECTORY ${INC_DIR})
    endif()

    # file(GLOB_RECURSE TMQ_LIBS "${CMAKE_SOURCE_DIR}/TMQLib/*")

    # message("Checking directories and files found by GLOB_RECURSE:")
    # foreach(FILE_PATH IN LISTS TMQ_LIBS)
    #     message("  - ${FILE_PATH}")
    # endforeach()

    # # Filter only directories that contain 'inc' directory
    # set(TMQ_INC_DIRS)
    # foreach(DIR IN LISTS TMQ_LIBS)
    #     if(EXISTS "${DIR}/inc")
    #         list(APPEND TMQ_INC_DIRS "${DIR}/inc")
    #     endif()
    # endforeach()

    # message("Found the following 'inc' directories:")
    # foreach(INC_DIR_PATH IN LISTS TMQ_INC_DIRS)
    #     message("  - ${INC_DIR_PATH}")
    # endforeach()

    file(GLOB_RECURSE TMQ_LIBS_DIRS LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/TMQLib/TMQ*/inc")

    # Filter only directories that contain 'inc' directory
    set(TMQ_INC_DIRS)
    foreach(DIR IN LISTS TMQ_LIBS_DIRS)
        if(DIR MATCHES "inc$")
            list(APPEND TMQ_INC_DIRS ${DIR})
        endif()
    endforeach()

    foreach(INC_DIR_PATH IN LISTS TMQ_INC_DIRS)
        string(REGEX MATCH "TMQLib/([^/]+)/inc" LIBRARY_NAME ${INC_DIR_PATH})
        string(REGEX REPLACE "TMQLib/([^/]+)/inc" "\\1" LIBRARY_NAME ${LIBRARY_NAME})

        # Define the symlink path in the root 'inc' directory
        set(SYMLINK_PATH "${INC_DIR}/${LIBRARY_NAME}")

        # Convert paths to native format (for Windows, this uses backslashes)
        cmake_path(CONVERT "${INC_DIR_PATH}" TO_NATIVE_PATH_LIST INC_DIR_PATH)
        cmake_path(CONVERT "${SYMLINK_PATH}" TO_NATIVE_PATH_LIST SYMLINK_PATH)

        if (NOT EXISTS ${SYMLINK_PATH})
            message("Creating symlink from ${INC_DIR_PATH} to ${SYMLINK_PATH}")

            if(WIN32)
                # For Windows, use mklink (note: mklink needs administrative privileges)
                set(SYMLINK_COMMAND "cmd" "/c" "mklink" "/D" "${SYMLINK_PATH}" "${INC_DIR_PATH}")
            else()
                # For Unix-based systems, use 'ln -s' to create symlink
                set(SYMLINK_COMMAND "ln" "-s" "${INC_DIR_PATH}" "${SYMLINK_PATH}")
            endif()

            message("${SYMLINK_COMMAND}")

            # Execute the symlink creation command
            execute_process(
                COMMAND ${SYMLINK_COMMAND}
                RESULT_VARIABLE result
                OUTPUT_QUIET
            )
            if(result)
                message(WARNING "Failed to create symlink for: ${INCLUDE_DIRECTORY_PATH}")
            endif()
        endif()
    endforeach()
endfunction()

create_master_inc_dir()

function(TMQ_define_dynalib TARGET)
    file(GLOB SOURCES  "${CMAKE_SOURCE_DIR}/TMQLib/${TARGET}/src/*.cpp")
    file(GLOB HEADERS  "${CMAKE_SOURCE_DIR}/TMQLib/${TARGET}/src/*.h")
    file(GLOB INCLUDES "${CMAKE_SOURCE_DIR}/TMQLib/${TARGET}/inc/*.h")

    add_library(${TARGET}
                SHARED
                ${SOURCES}
                ${HEADERS}
                ${INCLUDES})

    # Specify the library's include directories
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${MASTER_INC_DIR})

    target_compile_definitions(${TARGET} PRIVATE "${TARGET}_EXPORTS")

    set_target_properties(${TARGET} PROPERTIES FOLDER "shared_libs")
    source_group("src" FILES ${SOURCES} FILES ${HEADERS})
    source_group("inc" FILES ${INCLUDES})
endfunction()

function(TMQ_define_dynalib_tests DYNALIB_NAME)
    file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/TMQLib/${DYNALIB_NAME}/test/*.cpp")
    file(GLOB HEADERS "${CMAKE_SOURCE_DIR}/TMQLib/${DYNALIB_NAME}/test/*.h")

    set(TESTS_TARGET_NAME "t_${DYNALIB_NAME}")

    add_executable(${TESTS_TARGET_NAME}
                   ${SOURCES}
                   ${HEADERS})

    target_link_libraries(${TESTS_TARGET_NAME} PRIVATE ${DYNALIB_NAME} GTest::gtest_main GTest::gmock_main)
    target_include_directories(${TESTS_TARGET_NAME} PRIVATE ${MASTER_INC_DIR})

    set_target_properties(${TESTS_TARGET_NAME} PROPERTIES FOLDER "tests")
    source_group("src" FILES ${SOURCES} FILES ${HEADERS})

    # gtest setup
    enable_testing()
    gtest_discover_tests(${TESTS_TARGET_NAME})
endfunction()

function(TMQ_define_dynalib_bench DYNALIB_NAME)
    file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/TMQLib/${DYNALIB_NAME}/bench/*.cpp")
    file(GLOB HEADERS "${CMAKE_SOURCE_DIR}/TMQLib/${DYNALIB_NAME}/bench/*.h")

    set(BENCH_TARGET_NAME "b_${DYNALIB_NAME}")

    add_executable(${BENCH_TARGET_NAME}
                   ${SOURCES}
                   ${HEADERS})

    if(WIN32)
        target_link_libraries(${BENCH_TARGET_NAME} ${DYNALIB_NAME} benchmark::benchmark)
        target_compile_definitions(${BENCH_TARGET_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_link_libraries(${BENCH_TARGET_NAME} ${DYNALIB_NAME} benchmark::benchmark pthread)
    endif()

    target_include_directories(${BENCH_TARGET_NAME} PRIVATE ${MASTER_INC_DIR})

    set_target_properties(${BENCH_TARGET_NAME} PROPERTIES FOLDER "benchmarks")
    source_group("src" FILES ${SOURCES} FILES ${HEADERS})

    enable_testing()
    add_test(
        NAME "Benchmarks.${BENCH_TARGET_NAME}"
        COMMAND ${CMAKE_BINARY_DIR}/bin/Release/${BENCH_TARGET_NAME} --benchmark_out=benchmark_results.json --benchmark_out_format=json
    )
endfunction()

function(TMQ_define_tools_exe EXE_NAME)
    TMQ_define_exe(${EXE_NAME} "tools/${EXE_NAME}")
endfunction()

function(TMQ_define_exe EXE_NAME EXE_ROOT_DIR)
    file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/${EXE_ROOT_DIR}/src/*.cpp")
    file(GLOB HEADERS "${CMAKE_SOURCE_DIR}/${EXE_ROOT_DIR}/src/*.h")

    add_executable(${EXE_NAME}
                   ${SOURCES}
                   ${HEADERS})

    set_target_properties(${EXE_NAME} PROPERTIES FOLDER "tools")
    source_group("src" FILES ${SOURCES} FILES ${HEADERS})
endfunction()

function(append_include_directories TARGET NEW_INCLUDES)
    # Get the current include directories for the target
    get_target_property(CURRENT_INCLUDES ${TARGET} PUBLIC_INCLUDE_DIRECTORIES)

    # If the target does not have any include directories set, initialize to an empty list
    if(NOT CURRENT_INCLUDES)
        set(CURRENT_INCLUDES "")
    endif()

    # Append the new include directories to the existing list
    list(APPEND CURRENT_INCLUDES ${NEW_INCLUDES})

    # Set the updated list of include directories back to the target
    target_include_directories(${TARGET} PUBLIC ${CURRENT_INCLUDES})
endfunction()

function(append_link_libraries TARGET)
    # Get the current linked libraries for the target (both private and public)
    get_target_property(CURRENT_LIBRARIES ${TARGET} LINK_LIBRARIES)

    # If no libraries are currently linked, initialize to an empty list
    if(NOT CURRENT_LIBRARIES)
        set(CURRENT_LIBRARIES "")
    endif()

    # Append the new libraries to the existing list
    foreach(NEW_LIBRARY IN ITEMS ${ARGN})
        list(APPEND CURRENT_LIBRARIES ${NEW_LIBRARY})
    endforeach()

    # Set the updated list of linked libraries back to the target
    target_link_libraries(${TARGET} PRIVATE ${CURRENT_LIBRARIES})
endfunction()

# Aeron set up (TODO: Move to it's own TMQAeron project)

set(CMAKE_FOLDER "vendor/aeron")
add_subdirectory(vendor/aeron)
unset(CMAKE_FOLDER)
set(AERON_CLIENT_CPP_WRAPPER_INC_PATH "${CMAKE_SOURCE_DIR}/vendor/aeron/aeron-client/src/main/cpp_wrapper")
set(AERON_CLIENT_C_INC_PATH "${CMAKE_SOURCE_DIR}/vendor/aeron/aeron-client/src/main/c")
set(AERON_CLIENT_CPP_WRAPPER_LINK_LIB "aeron_client_wrapper")
set(AERON_CLIENT_C_LINK_LIB "aeron")
add_definitions(-DCLIENT_SHARED)

set(AERON_FILENAME "aeron.dll")
# make copied file be dependent from one which is build.
# Note, that DEPENDS here creates dependencies both from the target
# and from the file it creates.
add_custom_command(OUTPUT
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${AERON_FILENAME}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:aeron>
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}
    DEPENDS aeron
)
# Create target which consume the command via DEPENDS.
add_custom_target(copy_aeron_dll ALL
    DEPENDS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/${AERON_FILENAME}
)

set_target_properties(copy_aeron_dll PROPERTIES FOLDER "custom_targets")

# ClickHouse setup

set(CMAKE_FOLDER "vendor/clickhouse-cpp")
if(UNIX)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "On linux build clickhouse as a shared lib" FORCE)
endif()
add_subdirectory(vendor/clickhouse-cpp)
unset(CMAKE_FOLDER)
if(UNIX)
    unset(BUILD_SHARED_LIBS)
endif()
set(CLICKHOUSE_CPP_INC_PATH "${CMAKE_SOURCE_DIR}/vendor/clickhouse-cpp")
set(CLICKHOUSE_CPP_CONTRIB_INC_PATH "${CMAKE_SOURCE_DIR}/vendor/clickhouse-cpp/contrib/absl")
set(CLICKHOUSE_CPP_LINK_LIB "clickhouse-cpp-lib")

# Flatbuffers setup

set(CMAKE_FOLDER "vendor/flatbuffers")
set(FLATBUFFERS_BUILD_SHAREDLIB ON CACHE BOOL "Build flatbuffers as shared library" FORCE)
set(FLATBUFFERS_BUILD_FLATLIB OFF CACHE BOOL "Don't build static flatbuffers library" FORCE)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON) # Export all symbols on Windows (dllexport not setup in flatbuffers library)
add_subdirectory(vendor/flatbuffers ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
unset(FLATBUFFERS_BUILD_SHAREDLIB)
unset(CMAKE_FOLDER)
set(FLATBUFFERS_INC_PATHS "${CMAKE_SOURCE_DIR}/vendor/flatbuffers/include")
set(FLATBUFFERS_LINK_LIB "flatbuffers_shared")

# spdlog setup
set(CMAKE_FOLDER "vendor/spdlog")
set(SPDLOG_BUILD_SHARED ON CACHE BOOL "Build spdlog as shared library" FORCE)
set(SPDLOG_BUILD_PIC ON CACHE BOOL "Use PIC when building spdlog as shared library" FORCE)
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "Use std::format for spdlog" FORCE)
add_subdirectory(vendor/spdlog)
set(SPDLOG_INC_PATHS "${CMAKE_SOURCE_DIR}/vendor/spdlog/include")
set(SPDLOG_LINK_LIB "spdlog")
unset(CMAKE_FOLDER)

# nlohmann json setup
set(CMAKE_FOLDER "vendor/nlohmann_json")
set(JSON_BuildTests OFF CACHE INTERNAL "" FORCE)
add_subdirectory(vendor/nlohmann_json)
set(JSON_LINK_LIB "nlohmann_json::nlohmann_json")
unset(CMAKE_FOLDER)

# toml++ setup
add_subdirectory(vendor/tomlplusplus)
set(TOMLPP_INC_PATHS "${CMAKE_SOURCE_DIR}/vendor/tomlplusplus/include")

# Add TMQLib subdirectories
add_subdirectory(TMQLib/TMQCore)
add_subdirectory(TMQLib/TMQUtils)
add_subdirectory(TMQLib/TMQMarket)
add_subdirectory(TMQLib/TMQSolace)
add_subdirectory(TMQLib/TMQMessaging)
add_subdirectory(TMQLib/TMQClickHouse)
add_subdirectory(TMQLib/TMQSerialisation)

# Add tools subdirectories
add_subdirectory(tools/aeron_publisher)
add_subdirectory(tools/aeron_subscriber)