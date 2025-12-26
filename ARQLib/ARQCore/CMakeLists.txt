ARQ_define_dynalib(ARQCore)
target_link_libraries(ARQCore PUBLIC ARQUtils ${SPDLOG_LINK_LIB})
target_include_directories(ARQCore PUBLIC ${SPDLOG_INC_PATHS})

ARQ_define_dynalib_tests(ARQCore)