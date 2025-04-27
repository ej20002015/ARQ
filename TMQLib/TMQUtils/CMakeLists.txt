TMQ_define_dynalib(TMQUtils)
append_link_libraries(TMQUtils PUBLIC ${JSON_LINK_LIB})
target_include_directories(TMQUtils PUBLIC ${TOMLPP_INC_PATHS})

TMQ_define_dynalib_tests(TMQUtils)