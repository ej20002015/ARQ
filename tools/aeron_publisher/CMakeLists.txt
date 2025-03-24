TMQ_define_tools_exe(aeron_publisher)

# Aeron
target_include_directories(aeron_publisher PUBLIC ${AERON_CLIENT_C_INC_PATH} ${AERON_CLIENT_CPP_WRAPPER_INC_PATH})
target_link_libraries(aeron_publisher ${AERON_CLIENT_CPP_WRAPPER_LINK_LIB} ${AERON_CLIENT_C_LINK_LIB})