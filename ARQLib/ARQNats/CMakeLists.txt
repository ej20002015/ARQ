ARQ_define_dynalib(ARQNats)
append_link_libraries(ARQNats ARQCore nats::nats_static)
openssl_setup_runtime(ARQNats)