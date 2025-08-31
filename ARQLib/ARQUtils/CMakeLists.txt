ARQ_define_dynalib(ARQUtils)
append_link_libraries(ARQUtils PUBLIC ${JSON_LINK_LIB})
target_include_directories(ARQUtils PUBLIC ${TOMLPP_INC_PATHS})

ARQ_define_dynalib_tests(ARQUtils)