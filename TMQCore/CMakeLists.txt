#
# LIBRARY
#

# TMQ_core library project
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "src/*.h")
file(GLOB INCLUDES "inc/*.h")

add_library(TMQCore
            SHARED
            ${SOURCES}
            ${HEADERS}
            ${INCLUDES})

# Specify the library's public include directories
target_include_directories(TMQCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(TMQCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_definitions(TMQCore PRIVATE TMQCORE_EXPORTS)

set_target_properties(TMQCore PROPERTIES FOLDER "dlls")
source_group("src" FILES ${SOURCES} FILES ${HEADERS})
source_group("inc" FILES ${INCLUDES})

#
# TESTS
#

# Set up project to test library
file(GLOB SOURCES_t "test/*.cpp")
file(GLOB HEADERS_t "test/*.h")
add_executable(TMQCore_t
               ${SOURCES_t}
               ${HEADERS_t})

target_link_libraries(TMQCore_t TMQCore GTest::gtest_main)
target_include_directories(TMQCore_t PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

set_target_properties(TMQCore_t PROPERTIES FOLDER "tests")
source_group("src" FILES ${SOURCES_t} FILES ${HEADERS_t})

# gtest setup
enable_testing()
gtest_discover_tests(TMQCore_t)