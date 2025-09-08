find_package(SWIG 4.3.1 REQUIRED)
include(UseSWIG)

set(SWIG_MAIN_FILE "ARQ.i")
set(SWIG_LINK_LIBS ARQUtils ARQMarket)

function(ARQ_define_swig_wrapper LANGUAGE)
    if(${LANGUAGE} STREQUAL "python")
        find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
        #set(Python3_LIBRARY_DEBUG "${Python3_LIBRARY_RELEASE}")
        set(LANG_SPECIFIC_SWIG_OPTIONS "-threads")
    elseif(${LANGUAGE} STREQUAL "csharp")
	set(LANG_SPECIFIC_SWIG_OPTIONS "-namespace;ARQLib") 
    else()
        message(FATAL_ERROR "Unsupported SWIG language: ${LANGUAGE}")
    endif()

    set(CMAKE_SWIG_FLAGS -c++ ${LANG_SPECIFIC_SWIG_OPTIONS})
    set_source_files_properties(${SWIG_MAIN_FILE} PROPERTIES CPLUSPLUS ON)
    #set_property(SOURCE ${SWIG_FILE} PROPERTY SWIG_OUTPUT_EXTENSION ".cxx")
    set(SWIG_MODULE_TARGET "ARQLib_${LANGUAGE}")
    swig_add_library(${SWIG_MODULE_TARGET}
        TYPE MODULE
        LANGUAGE ${LANGUAGE}
        SOURCES ${SWIG_MAIN_FILE}
        OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>"
        OUTFILE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${LANGUAGE}/$<CONFIG>"
    )
    set_target_properties(${SWIG_MODULE_TARGET} PROPERTIES FOLDER "swig/${LANGUAGE}")
    set_target_properties(${SWIG_MODULE_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    target_include_directories(${SWIG_MODULE_TARGET} PRIVATE ${MASTER_INC_DIR})

    if(${LANGUAGE} STREQUAL "python")
        swig_link_libraries(${SWIG_MODULE_TARGET} PUBLIC ${SWIG_LINK_LIBS} Python3::Python)
        # Workaround: Undefine _DEBUG for the SWIG Python extension to avoid python38_d.lib link errors
        if (MSVC)
            target_compile_definitions(${SWIG_MODULE_TARGET} PRIVATE SWIG_PYTHON_INTERPRETER_NO_DEBUG)
        endif()
    elseif(${LANGUAGE} STREQUAL "csharp")
        swig_link_libraries(${SWIG_MODULE_TARGET} PUBLIC ${SWIG_LINK_LIBS})
    endif()
endfunction()

ARQ_define_swig_wrapper(python)
ARQ_define_swig_wrapper(csharp)
