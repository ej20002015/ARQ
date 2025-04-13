TMQ_define_dynalib(TMQCore)
append_link_libraries(TMQCore TMQUtils TMQClickHouse TMQSerialisation)
target_include_directories(TMQCore PUBLIC ${SPDLOG_INC_PATHS})
append_link_libraries(TMQCore ${SPDLOG_LINK_LIB})

TMQ_define_dynalib_tests(TMQCore)
append_link_libraries(t_TMQCore TMQClickHouse TMQSerialisation)