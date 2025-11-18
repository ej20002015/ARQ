ARQ_define_dynalib(ARQProtobuf)
append_link_libraries(ARQProtobuf ARQCore ${PROTO_GRPC_LINK_LIB})

ARQ_define_dynalib_tests(ARQProtobuf)