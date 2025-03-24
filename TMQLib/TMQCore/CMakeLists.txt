TMQ_define_dynalib(TMQCore)
append_link_libraries(TMQCore TMQUtils TMQClickHouse)
target_include_directories(TMQCore PUBLIC ${CLICKHOUSE_CPP_INC_PATH} ${CLICKHOUSE_CPP_CONTRIB_INC_PATH})
append_link_libraries(TMQCore ${CLICKHOUSE_CPP_LINK_LIB})

TMQ_define_dynalib_tests(TMQCore)
append_link_libraries(t_TMQCore TMQClickHouse ${CLICKHOUSE_CPP_LINK_LIB})