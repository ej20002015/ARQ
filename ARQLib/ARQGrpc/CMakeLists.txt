ARQ_define_dynalib(ARQGrpc)
append_link_libraries(ARQGrpc ARQCore ${PROTO_GRPC_LINK_LIB})

ARQ_define_dynalib_tests(ARQGrpc)