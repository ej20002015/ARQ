ARQ_define_dynalib(ARQClickHouse)
append_link_libraries(ARQClickHouse ARQCore)

target_include_directories(ARQClickHouse PUBLIC ${CLICKHOUSE_CPP_INC_PATH} ${CLICKHOUSE_CPP_CONTRIB_INC_PATH})
append_link_libraries(ARQClickHouse ${CLICKHOUSE_CPP_LINK_LIB})

# target_link_libraries(clickhouse_test PRIVATE ${CLICKHOUSE_CPP_LINK_LIB})