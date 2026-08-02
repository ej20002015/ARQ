ARQ_define_tools_exe(
    kafka_example
    LIBRARIES rdkafka::rdkafkacpp
)
librdkafka_setup_runtime(kafka_example)
openssl_setup_runtime(kafka_example)
zlib_setup_runtime(kafka_example)
