find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

file(GLOB PROTO_FILES "*.proto")

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto_gen/proto_gen")
set(PROTO_IMPORT_DIRS ".")

# Generate protobuf C++ sources
protobuf_generate(
    LANGUAGE cpp
    OUT_VAR PROTO_SRCS
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PROTOS ${PROTO_FILES})

# Generate gRPC C++ sources
protobuf_generate(
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    OUT_VAR GRPC_SRCS
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}"
    IMPORT_DIRS ${PROTO_IMPORT_DIRS}
    PLUGIN "protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
    PROTOS ${PROTO_FILES})

# Bundle everything into one static library
add_library(proto-lib STATIC
    ${PROTO_SRCS}
    ${GRPC_SRCS})

set_target_properties(proto-lib PROPERTIES FOLDER "custom_targets")

# Consumers just need this include path
target_include_directories(proto-lib PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/proto_gen>")

# Hide Protobuf/gRPC dependencies inside proto-lib
target_link_libraries(proto-lib
    PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

set(PROTO_GRPC_LINK_LIB "proto-lib" PARENT_SCOPE)