TMQ_define_dynalib(TMQSerialisation)
target_include_directories(TMQSerialisation PUBLIC ${FLATBUFFERS_INC_PATHS})
append_link_libraries(TMQSerialisation TMQUtils ${FLATBUFFERS_LINK_LIB})

TMQ_define_dynalib_tests(TMQSerialisation)

## FlatBuffers code generation
set(FLATBUFFERS_SCHEMA_DIR "${CMAKE_SOURCE_DIR}/TMQLib/TMQSerialisation/fbs")
set(FLATBUFFERS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/fbs_generated/fbs_generated")
set(FLATBUFFERS_OUTPUT_INC_DIR "${CMAKE_BINARY_DIR}/fbs_generated")

file(MAKE_DIRECTORY ${FLATBUFFERS_OUTPUT_DIR})
file(GLOB FLATBUFFER_SCHEMAS "${FLATBUFFERS_SCHEMA_DIR}/*.fbs")

if (TARGET flatc)
    set(FLATC_EXECUTABLE $<TARGET_FILE:flatc>)
    add_dependencies(TMQSerialisation flatc)
else()
    find_program(FLATC_EXECUTABLE NAMES flatc REQUIRED)
endif()

# Ensure we found the executable
if(NOT FLATC_EXECUTABLE)
    message(FATAL_ERROR "Could not determine flatc executable path from FlatBuffers target")
endif()

set(FLATBUFFER_GENERATED_FILES)

foreach(SCHEMA ${FLATBUFFER_SCHEMAS})
    get_filename_component(SCHEMA_NAME ${SCHEMA} NAME_WE) # Extract filename without extension
    set(GENERATED_HEADER "${FLATBUFFERS_OUTPUT_DIR}/${SCHEMA_NAME}.h")
    
    add_custom_command(
        OUTPUT ${GENERATED_HEADER}
        COMMAND ${FLATC_EXECUTABLE} --cpp -o ${FLATBUFFERS_OUTPUT_DIR} ${SCHEMA} --filename-suffix ""
        DEPENDS ${SCHEMA}
        COMMENT "Generating FlatBuffer file: ${SCHEMA_NAME}"
        VERBATIM
    )

    list(APPEND FLATBUFFER_GENERATED_FILES ${GENERATED_HEADER})
endforeach()

# Move custom target definition *after* FLATBUFFER_GENERATED_FILES is populated
add_custom_target(generate_flatbuffers
    DEPENDS ${FLATBUFFER_GENERATED_FILES}
)
set_target_properties(generate_flatbuffers PROPERTIES FOLDER "custom_targets")

file(GLOB FBS_FILES "fbs/*.fbs")
target_sources(generate_flatbuffers PRIVATE ${FBS_FILES})

# Ensure TMQSerialisation builds only after FlatBuffers are generated
add_dependencies(TMQSerialisation generate_flatbuffers)

# Add the output directory to the include paths
target_include_directories(TMQSerialisation PUBLIC ${FLATBUFFERS_OUTPUT_INC_DIR})