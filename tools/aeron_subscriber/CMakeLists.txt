TMQ_define_tools_exe(aeron_subscriber)

# Aeron
target_include_directories(aeron_subscriber PRIVATE ${AERON_CLIENT_CPP_WRAPPER_SOURCE_PATH} ${AERON_CLIENT_C_SOURCE_PATH})
target_link_libraries(aeron_subscriber PRIVATE ${AERON_CLIENT_CPP_WRAPPER_LINK_LIB} ${AERON_CLIENT_C_LINK_LIB})