# Register ARQ dynalib

ARQ_define_dynalib(ARQAdapt)
target_link_libraries(ARQAdapt PUBLIC ARQUtils ARQCore)

# Add SWIG files to ARQAdapt to browse them in VS
file(GLOB_RECURSE SWIG_ALL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/swig/*.i")
set_source_files_properties(${SWIG_ALL_FILES} PROPERTIES HEADER_FILE_ONLY TRUE) # Mark them as headers so the standard C++ compiler completely ignores them
target_sources(ARQAdapt PRIVATE ${SWIG_ALL_FILES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/swig" PREFIX "swig" FILES ${SWIG_ALL_FILES})

# SWIG code gen targets

find_package(SWIG 4.3.1 REQUIRED)
include(UseSWIG)

set(SWIG_LIB_NAME "ARQLib")
set(SWIG_MAIN_FILE "swig/${SWIG_LIB_NAME}.i")
set(SWIG_CSHARP_GEN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/dotnet/ARQ.NET/src/Interop")

function(ARQ_define_swig_wrapper LANGUAGE)
    if(${LANGUAGE} STREQUAL "python")
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        set(LANG_SPECIFIC_SWIG_OPTIONS "-threads")
        # In Python, the SWIG module target name must exactly match the %module name.
        # CMake's UseSWIG module will automatically prepend the required '_' to the final .pyd/.so file
        set(SWIG_MODULE_TARGET "${SWIG_LIB_NAME}")
        set(GENERATED_FILE_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/swig_${LANGUAGE}/$<CONFIG>")
    elseif(${LANGUAGE} STREQUAL "csharp")
        #set(LANG_SPECIFIC_SWIG_OPTIONS "-namespace;${SWIG_LIB_NAME}") # Define namespaces in the cpp
        # C# doesn't have the strict naming requirement - suffix with csharp to have a different name to the python one
        set(SWIG_MODULE_TARGET "${SWIG_LIB_NAME}_csharp")
        set(GENERATED_FILE_DIR "${SWIG_CSHARP_GEN_OUTPUT_DIR}")
    else()
        message(FATAL_ERROR "Unsupported SWIG language: ${LANGUAGE}")
    endif()

    set(CMAKE_SWIG_FLAGS -c++ ${LANG_SPECIFIC_SWIG_OPTIONS} -I${CMAKE_CURRENT_SOURCE_DIR})

    # If we are on 64-bit Linux, tell SWIG that int64_t is 'long', not 'long long'
    if(CMAKE_SYSTEM_NAME MATCHES "Linux" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        list(APPEND CMAKE_SWIG_FLAGS "-DSWIGWORDSIZE64")
    endif()
    
    set_source_files_properties(${SWIG_MAIN_FILE} PROPERTIES CPLUSPLUS ON)

    # Tell SWIG the logical module name matches the target
    if(${LANGUAGE} STREQUAL "python")
        set_source_files_properties(${SWIG_MAIN_FILE} PROPERTIES SWIG_MODULE_NAME ${SWIG_LIB_NAME})
    endif()

    swig_add_library(${SWIG_MODULE_TARGET}
        TYPE MODULE
        LANGUAGE ${LANGUAGE}
        SOURCES ${SWIG_MAIN_FILE}
        OUTPUT_DIR "${GENERATED_FILE_DIR}"
        OUTFILE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${LANGUAGE}/$<CONFIG>"
    )

    set_target_properties(${SWIG_MODULE_TARGET} PROPERTIES FOLDER "swig/${LANGUAGE}")
    set_target_properties(${SWIG_MODULE_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    
    # Target includes for the generated C++ wrapper file
    target_include_directories(${SWIG_MODULE_TARGET} PRIVATE ${MASTER_INC_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/inc)

    if(${LANGUAGE} STREQUAL "python")
        target_link_libraries(${SWIG_MODULE_TARGET} PUBLIC ARQAdapt Python3::Python)
    elseif(${LANGUAGE} STREQUAL "csharp")
        target_link_libraries(${SWIG_MODULE_TARGET} PUBLIC ARQAdapt)
        if(NOT WIN32)
            set_target_properties(${SWIG_MODULE_TARGET} PROPERTIES PREFIX "")
        endif()
    endif()

    ## INSTALL

    if(WIN32)
        set(MODULE_DESTINATION bin)
    else()
        set(MODULE_DESTINATION lib)
    endif()

    install(TARGETS ${SWIG_MODULE_TARGET}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${MODULE_DESTINATION}
        ARCHIVE DESTINATION lib
    )

endfunction()

ARQ_define_swig_wrapper(python)
ARQ_define_swig_wrapper(csharp)
