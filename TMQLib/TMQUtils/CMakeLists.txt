TMQ_define_dynalib(TMQUtils)
append_link_libraries(TMQUtils PUBLIC ${JSON_LINK_LIB})

TMQ_define_dynalib_tests(TMQUtils)