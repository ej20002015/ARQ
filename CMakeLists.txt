cmake_minimum_required(VERSION 3.23)

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

function(add_namespaced_include_directory TARGET NAMESPACE INCLUDE_DIRECTORY_PATH)
    # Get the build directory (this will be the one passed via the -B flag)
    set(BUILD_DIR ${CMAKE_BINARY_DIR})
    set(NAMESPACED_INCLUDES_DIR "${BUILD_DIR}/namespaced_includes")

    # Create "namespaced_includes" directory if it doesn't exist
    if (NOT EXISTS ${NAMESPACED_INCLUDES_DIR})
        file(MAKE_DIRECTORY ${NAMESPACED_INCLUDES_DIR})
    endif()

    # Create the subdirectory for the target if it doesn't exist
    set(TARGET_DIR "${NAMESPACED_INCLUDES_DIR}/${TARGET}")
    if (NOT EXISTS ${TARGET_DIR})
        file(MAKE_DIRECTORY ${TARGET_DIR})
    endif()

    set(NAMESPACE_DIR "${TARGET_DIR}/${NAMESPACE}")

    # Convert paths to native format (for Windows, this uses backslashes)
    #cmake_path(${INCLUDE_DIRECTORY_PATH} TO_NATIVE_PATH ${INCLUDE_DIRECTORY_PATH})
    cmake_path(CONVERT "${INCLUDE_DIRECTORY_PATH}" TO_NATIVE_PATH_LIST INCLUDE_DIRECTORY_PATH)
    cmake_path(CONVERT "${NAMESPACE_DIR}" TO_NATIVE_PATH_LIST NAMESPACE_DIR)

    if (NOT EXISTS ${NAMESPACE_DIR})
        # Define the symlink command based on the platform
        if(WIN32)
            # Windows uses cmd's mklink for creating symlinks
            set(SYMLINK_COMMAND "cmd" "/c" "mklink" "/D" "${NAMESPACE_DIR}" "${INCLUDE_DIRECTORY_PATH}")
        else()
            # Unix-like systems (Linux, macOS) use 'ln -s' for symlinks
            set(SYMLINK_COMMAND "ln" "-s" "${INCLUDE_DIRECTORY_PATH}" "${NAMESPACE_DIR}")
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

    # Add the target's main include directory to the target's include directories
    target_include_directories(${TARGET} PUBLIC "${TARGET_DIR}")
endfunction()

function(TMQ_add_lib_include_dir TARGET LIBNAME)
    # Define the include directory path based on the libname
    set(INCLUDE_DIRECTORY_PATH "${CMAKE_SOURCE_DIR}/TMQLib/${LIBNAME}/inc")

    # Call the previously defined ADD_NAMESPACED_INCLUDE_DIRECTORY function
    ADD_NAMESPACED_INCLUDE_DIRECTORY(${TARGET} ${LIBNAME} ${INCLUDE_DIRECTORY_PATH})
endfunction()

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
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    TMQ_add_lib_include_dir(${TARGET} ${TARGET})

    target_compile_definitions(${TARGET} PRIVATE "${TARGET}_EXPORTS")

    set_target_properties(${TARGET} PROPERTIES FOLDER "dlls")
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

    target_link_libraries(${TESTS_TARGET_NAME} ${DYNALIB_NAME} GTest::gtest_main)
    TMQ_add_lib_include_dir(${TESTS_TARGET_NAME} ${DYNALIB_NAME})

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

    TMQ_add_lib_include_dir(${BENCH_TARGET_NAME} ${DYNALIB_NAME})

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

function(append_link_libraries TARGET NEW_LIBRARIES)
    # Get the current linked libraries for the target (both private and public)
    get_target_property(CURRENT_LIBRARIES ${TARGET} LINK_LIBRARIES)

    # If no libraries are currently linked, initialize to an empty list
    if(NOT CURRENT_LIBRARIES)
        set(CURRENT_LIBRARIES "")
    endif()

    # Append the new libraries to the existing list
    list(APPEND CURRENT_LIBRARIES ${NEW_LIBRARIES})

    # Set the updated list of linked libraries back to the target
    target_link_libraries(${TARGET} PRIVATE ${CURRENT_LIBRARIES})
endfunction()

# Aeron set up

set(CMAKE_FOLDER "vendor/aeron")
add_subdirectory(vendor/aeron)
unset(CMAKE_FOLDER)
set(AERON_CLIENT_CPP_WRAPPER_SOURCE_PATH "${CMAKE_SOURCE_DIR}/vendor/aeron/aeron-client/src/main/cpp_wrapper")
set(AERON_CLIENT_C_SOURCE_PATH "${CMAKE_SOURCE_DIR}/vendor/aeron/aeron-client/src/main/c")
set(AERON_CLIENT_CPP_WRAPPER_LINK_LIB "aeron_client_wrapper")
set(AERON_CLIENT_C_LINK_LIB "aeron")
add_definitions(-DCLIENT_SHARED)

# Because OUTPUT option may not use generator expressions,
# extract name of file from target's properties.
# get_target_property(AERON_BASENAME aeron OUTPUT_NAME)
# get_target_property(AERON_SUFFIX aeron SUFFIX)
set(AERON_BASENAME "aeron")
set(AERON_SUFFIX ".dll")
set(AERON_FILENAME ${AERON_BASENAME}${AERON_SUFFIX})
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

# Add TMQLib subdirectories
add_subdirectory(TMQLib/TMQCore)
add_subdirectory(TMQLib/TMQUtils)
add_subdirectory(TMQLib/TMQMarket)
add_subdirectory(TMQLib/TMQSolace)
add_subdirectory(TMQLib/TMQMessaging)

# Add tools subdirectories
add_subdirectory(tools/aeron_publisher)
add_subdirectory(tools/aeron_subscriber)