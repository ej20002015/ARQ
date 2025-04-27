TMQ_define_dynalib(TMQClickHouse)
append_link_libraries(TMQClickHouse TMQCore)

target_include_directories(TMQClickHouse PUBLIC ${CLICKHOUSE_CPP_INC_PATH} ${CLICKHOUSE_CPP_CONTRIB_INC_PATH})
append_link_libraries(TMQClickHouse ${CLICKHOUSE_CPP_LINK_LIB})

# target_link_libraries(clickhouse_test PRIVATE ${CLICKHOUSE_CPP_LINK_LIB})