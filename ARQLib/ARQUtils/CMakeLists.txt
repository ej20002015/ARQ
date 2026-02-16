ARQ_define_dynalib(ARQUtils)
append_link_libraries(ARQUtils PUBLIC ${JSON_LINK_LIB})
target_include_directories(ARQUtils PUBLIC ${TOMLPP_INC_PATHS} ${SPDLOG_INC_PATHS})

ARQ_define_dynalib_tests(ARQUtils)

if(MSVC)
    target_sources(ARQUtils PRIVATE "viz/UUID.natvis")
    set_target_properties(ARQUtils PROPERTIES VS_JUST_MY_CODE_SUPPORTING_FILES "viz/UUID.natvis")
endif()