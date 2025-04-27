TMQ_define_dynalib(TMQCore)
target_link_libraries(TMQCore TMQUtils TMQSerialisation)
target_link_libraries(TMQCore ${SPDLOG_LINK_LIB})
target_include_directories(TMQCore PUBLIC ${SPDLOG_INC_PATHS})

TMQ_define_dynalib_tests(TMQCore)