cmake_minimum_required(VERSION 3.23)

message(STATUS "CMake Generator: ${CMAKE_GENERATOR}")

# Project name
project(ARQ)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options("/Zc:__cplusplus")
    option(ENABLE_VCPKG OFF)
endif()

#-lstdc++exp

# Allow for folders in the IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Linux with default to prepending with prefix "lib" - don't want that
set(CMAKE_SHARED_LIBRARY_PREFIX "")

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# For non-install builds, set the RPATH so executables can find local .so files.
# $ORIGIN is a special linker variable that means "the directory of the executable".
set(CMAKE_BUILD_RPATH "$ORIGIN:${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1) # hack to prevent CTest added targets

include(FetchContent)

# Handle ARQLib etc files

set(CONFIG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/ARQLib/etc")
set(CONFIG_BUILD_DEST "${CMAKE_BINARY_DIR}/etc/ARQLib")

# Create target to perform copy of config files
add_custom_target(arq_config_copy
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CONFIG_BUILD_DEST}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CONFIG_SOURCE_DIR}" "${CONFIG_BUILD_DEST}"
    COMMENT "Updating build-tree config files..."
    VERBATIM
)
set_target_properties(arq_config_copy PROPERTIES FOLDER "custom_targets")

install(DIRECTORY "${CONFIG_SOURCE_DIR}/"
    DESTINATION etc/ARQLib
)

# Download pre-built binaries / packages from ARQ_dependencies repo

set(PACKAGES_DIR ${CMAKE_BINARY_DIR}/packages)

function(add_prebuilt_dependency NAME URL)
    message(STATUS "Downloading pre-built dependency: ${NAME}")

    set(LOCAL_SOURCE_DIR "${PACKAGES_DIR}/${NAME}")

    FetchContent_Declare(
        ${NAME}
        URL ${URL}
        SOURCE_DIR "${LOCAL_SOURCE_DIR}"
        DOWNLOAD_EXTRACT_TIMESTAMP true # Stamp extracted files with download time rather than original time
    )

    # This performs the download and extraction.
    FetchContent_MakeAvailable(${NAME})
    message(STATUS "Downloaded and extracted ${NAME} to location: ${LOCAL_SOURCE_DIR}")

    # Set directory of extracted package variable in the PARENT_SCOPE, so the main script can access it.
    set(FETCHCONTENT_EXTRACTED_DIR_${NAME} ${LOCAL_SOURCE_DIR} PARENT_SCOPE)

    # ------------- INSTALL -------------

    # --- LINUX / UNIX (.so files in lib) ---
    if(NOT WIN32)
        # 1. Install DEBUG libraries (Only when building Debug)
        if(EXISTS "${LOCAL_SOURCE_DIR}/debug/lib")
            install(DIRECTORY "${LOCAL_SOURCE_DIR}/debug/lib/"
                DESTINATION lib
                CONFIGURATIONS Debug
                USE_SOURCE_PERMISSIONS
                FILES_MATCHING PATTERN "*.so*"
                PATTERN "*.a" EXCLUDE
            )
        endif()

        # 2. Install RELEASE libraries (For Release, RelWithDebInfo, MinSizeRel)
        if(EXISTS "${LOCAL_SOURCE_DIR}/release/lib")
            install(DIRECTORY "${LOCAL_SOURCE_DIR}/release/lib/"
                DESTINATION lib
                CONFIGURATIONS Release RelWithDebInfo MinSizeRel
                USE_SOURCE_PERMISSIONS
                FILES_MATCHING PATTERN "*.so*"
                PATTERN "*.a" EXCLUDE
            )
        endif()

    # --- WINDOWS (.dll in bin, .lib in lib) ---
    else()
        # 1. Debug (DLLs + Libs)
        if(EXISTS "${LOCAL_SOURCE_DIR}/debug/bin")
            install(DIRECTORY "${LOCAL_SOURCE_DIR}/debug/bin/" 
                DESTINATION bin 
                CONFIGURATIONS Debug
                FILES_MATCHING PATTERN "*.dll"
            )
        endif()
        if(EXISTS "${LOCAL_SOURCE_DIR}/debug/lib")
            install(DIRECTORY "${LOCAL_SOURCE_DIR}/debug/lib/" 
                DESTINATION lib 
                CONFIGURATIONS Debug
                FILES_MATCHING PATTERN "*.lib"
            )
        endif()

        # 2. Release (DLLs + Libs)
        if(EXISTS "${LOCAL_SOURCE_DIR}/release/bin")
            install(DIRECTORY "${LOCAL_SOURCE_DIR}/release/bin/" 
                DESTINATION bin 
                CONFIGURATIONS Release RelWithDebInfo MinSizeRel
                FILES_MATCHING PATTERN "*.dll"
            )
        endif()
        if(EXISTS "${LOCAL_SOURCE_DIR}/release/lib")
            install(DIRECTORY "${LOCAL_SOURCE_DIR}/release/lib/" 
                DESTINATION lib 
                CONFIGURATIONS Release RelWithDebInfo MinSizeRel
                FILES_MATCHING PATTERN "*.lib"
            )
        endif()
    endif()

endfunction()

set(ARQ_Dependencies_download_path "https://github.com/ej20002015/ARQ_dependencies/releases/download/1.0.0-alpha")

if(WIN32)
    add_prebuilt_dependency(openssl-v3.6.0 "${ARQ_Dependencies_download_path}/openssl-v3.6.0-windows-x64.zip")
    add_prebuilt_dependency(zlib-v1.3.1 "${ARQ_Dependencies_download_path}/zlib-v1.3.1-windows-x64.zip")
    add_prebuilt_dependency(librdkafka_v2.12.0 "${ARQ_Dependencies_download_path}/librdkafka-v2.12.0-windows-x64.zip")
    add_prebuilt_dependency(SWIG_4.3.1 "${ARQ_Dependencies_download_path}/SWIG-4.3.1-windows-x64.zip")
    add_prebuilt_dependency(grpc-v1.76.0 "${ARQ_Dependencies_download_path}/grpc-v1.76.0-windows-x64.zip")
    add_prebuilt_dependency(natsc-v3.11.0 "${ARQ_Dependencies_download_path}/nats.c-v3.11.0-windows-x64.zip")
else()
    add_prebuilt_dependency(openssl-v3.6.0 "${ARQ_Dependencies_download_path}/openssl-3.6.0-linux-x64.tar.gz")
    add_prebuilt_dependency(zlib-v1.3.1 "${ARQ_Dependencies_download_path}/zlib-v1.3.1-linux-x64.tar.gz")
    add_prebuilt_dependency(librdkafka_v2.12.0 "${ARQ_Dependencies_download_path}/librdkafka-v2.12.0-linux-x64.tar.gz")
    add_prebuilt_dependency(SWIG_4.3.1 "${ARQ_Dependencies_download_path}/SWIG-4.3.1-linux-x64.tar.gz")
    add_prebuilt_dependency(grpc-v1.76.0 "${ARQ_Dependencies_download_path}/grpc-v1.76.0-linux-x64.tar.gz")
    add_prebuilt_dependency(natsc-v3.11.0 "${ARQ_Dependencies_download_path}/nats.c-v3.11.0-linux-x64.tar.gz")
endif()

# Include cmake standard files that define cmake targets and functions for pre-built binaries
include("${FETCHCONTENT_EXTRACTED_DIR_openssl-v3.6.0}/cmake/openssl.cmake")
include("${FETCHCONTENT_EXTRACTED_DIR_zlib-v1.3.1}/cmake/zlib.cmake")
include("${FETCHCONTENT_EXTRACTED_DIR_librdkafka_v2.12.0}/cmake/librdkafka.cmake")
include("${FETCHCONTENT_EXTRACTED_DIR_SWIG_4.3.1}/cmake/SWIG.cmake")
include("${FETCHCONTENT_EXTRACTED_DIR_grpc-v1.76.0}/cmake/grpc.cmake")
include("${FETCHCONTENT_EXTRACTED_DIR_natsc-v3.11.0}/cmake/nats.c.cmake")

# gtest setup
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

# Function to create symlinks in the 'inc/' directory for all ARQ* libraries' 'inc' directories
function(create_master_inc_dir)
    set(INC_DIR ${MASTER_INC_DIR})
    if (NOT EXISTS ${INC_DIR})
        file(MAKE_DIRECTORY ${INC_DIR})
    endif()

    file(GLOB_RECURSE ARQ_LIBS_DIRS LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/ARQLib/*ARQ*/inc")

    # Filter only directories that contain 'inc' directory
    set(ARQ_INC_DIRS)
    foreach(DIR IN LISTS ARQ_LIBS_DIRS)
        if(DIR MATCHES "inc$")
            list(APPEND ARQ_INC_DIRS ${DIR})
        endif()
    endforeach()

    foreach(INC_DIR_PATH IN LISTS ARQ_INC_DIRS)
        string(REGEX MATCH "ARQLib/([^/]+)/inc" LIBRARY_NAME ${INC_DIR_PATH})
        string(REGEX REPLACE "ARQLib/([^/]+)/inc" "\\1" LIBRARY_NAME ${LIBRARY_NAME})

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

function(ARQ_define_dynalib TARGET)
    file(GLOB SOURCES  "${CMAKE_SOURCE_DIR}/ARQLib/${TARGET}/src/*.cpp")
    file(GLOB HEADERS  "${CMAKE_SOURCE_DIR}/ARQLib/${TARGET}/src/*.h")
    file(GLOB INCLUDES "${CMAKE_SOURCE_DIR}/ARQLib/${TARGET}/inc/*.h")

    add_library(${TARGET}
                SHARED
                ${SOURCES}
                ${HEADERS}
                ${INCLUDES})

    # Specify the library's include directories
    target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${MASTER_INC_DIR})

    target_compile_definitions(${TARGET} PRIVATE "${TARGET}_EXPORTS")

    if(NOT WIN32)
        target_link_libraries(${TARGET} PUBLIC stdc++exp) # To enable usage of some c++23 features (e.g. stacktrace)
    endif()

    set_target_properties(${TARGET} PROPERTIES FOLDER "shared_libs")
    source_group("src" FILES ${SOURCES} FILES ${HEADERS})
    source_group("inc" FILES ${INCLUDES})

    # ------------- INSTALL -------------

    install(TARGETS ${TARGET}
        EXPORT ARQTargets
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )

    # Setup RPATH for the library itself
    # If this library loads OTHER libraries, it also needs to look in the same directory ($ORIGIN)
    set_target_properties(${TARGET} PROPERTIES INSTALL_RPATH "$ORIGIN" )

endfunction()

function(ARQ_define_dynalib_tests DYNALIB_NAME)
    file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/ARQLib/${DYNALIB_NAME}/test/*.cpp")
    file(GLOB HEADERS "${CMAKE_SOURCE_DIR}/ARQLib/${DYNALIB_NAME}/test/*.h")

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

function(ARQ_define_dynalib_bench DYNALIB_NAME)
    file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/ARQLib/${DYNALIB_NAME}/bench/*.cpp")
    file(GLOB HEADERS "${CMAKE_SOURCE_DIR}/ARQLib/${DYNALIB_NAME}/bench/*.h")

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

function(ARQ_define_tools_exe EXE_NAME)
    ARQ_define_exe(${EXE_NAME} "tools/${EXE_NAME}" "tools")
endfunction()

function(ARQ_define_services_exe EXE_NAME SUB_FOLDER)
    ARQ_define_exe(${EXE_NAME} "services/${SUB_FOLDER}/${EXE_NAME}" "services/${SUB_FOLDER}")
endfunction()

function(ARQ_define_exe EXE_NAME EXE_ROOT_DIR FOLDER_NAME)
    file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/${EXE_ROOT_DIR}/src/*.cpp")
    file(GLOB HEADERS "${CMAKE_SOURCE_DIR}/${EXE_ROOT_DIR}/src/*.h")

    add_executable(${EXE_NAME}
                   ${SOURCES}
                   ${HEADERS})

    if(NOT WIN32)
        target_link_libraries(${EXE_NAME} PRIVATE stdc++exp) # To enable usage of some c++23 features (e.g. stacktrace)
    endif()

    set_target_properties(${EXE_NAME} PROPERTIES FOLDER ${FOLDER_NAME})
    source_group("src" FILES ${SOURCES} FILES ${HEADERS})

    # ------------- INSTALL -------------

    install(TARGETS ${EXE_NAME}
        RUNTIME DESTINATION bin
    )

    # Set RPATH
    if(NOT WIN32)
        set_target_properties(${EXE_NAME} PROPERTIES
            BUILD_WITH_INSTALL_RPATH FALSE
            INSTALL_RPATH "$ORIGIN/../lib"
            INSTALL_RPATH_USE_LINK_PATH FALSE
        )
    endif()

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
    target_link_libraries(${TARGET} PUBLIC ${CURRENT_LIBRARIES})
endfunction()

# Create protobuf/grpc files
add_subdirectory(proto)

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

# spdlog setup (just use the header - much simpler and calls are abstracted behind ARQ logger anyway so build times won't be too bad)
set(SPDLOG_INC_PATHS "${CMAKE_SOURCE_DIR}/vendor/spdlog/include")

# nlohmann json setup
set(CMAKE_FOLDER "vendor/nlohmann_json")
set(JSON_BuildTests OFF CACHE INTERNAL "" FORCE)
add_subdirectory(vendor/nlohmann_json)
set(JSON_LINK_LIB "nlohmann_json::nlohmann_json")
unset(CMAKE_FOLDER)

# toml++ setup
add_subdirectory(vendor/tomlplusplus)
set(TOMLPP_INC_PATHS "${CMAKE_SOURCE_DIR}/vendor/tomlplusplus/include")

# modern-cpp-kafka set up
set(MODERN_CPP_KAFKA_INC_PATHS "${CMAKE_SOURCE_DIR}/vendor/modern-cpp-kafka/include")

# Add ARQLib subdirectories
add_subdirectory(ARQLib/ARQCore)
add_subdirectory(ARQLib/ARQUtils)
add_subdirectory(ARQLib/ARQMarket)
add_subdirectory(ARQLib/ARQClickHouse)
add_subdirectory(ARQLib/ARQGrpc)
add_subdirectory(ARQLib/ARQProtobuf)
add_subdirectory(ARQLib/ARQNats)
add_subdirectory(ARQLib/ARQKafka)

# Add services subdirectories
add_subdirectory(services/RefData/RefDataCmdExecutor)
add_subdirectory(services/RefData/RefDataAuditProjector)

# Add tools subdirectories
add_subdirectory(tools/kafka_example)
add_subdirectory(tools/nats_publisher)
add_subdirectory(tools/nats_subscriber)
add_subdirectory(tools/rd_cmd_publisher)

# Add SWIG wrappers
add_subdirectory(ARQLib/SWIG)
