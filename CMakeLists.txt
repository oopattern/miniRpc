
cmake_minimum_required (VERSION 2.8)

# CXXFLAG: -Wall should be add
# CXXFLAG: ubuntu can not use -lpthread, instead, use -pthread
# Link   : ubuntu use target_link_libraries method to add system lib to target
# *.proto: "protoc --cpp_out=./ *.proto" to generate *.pb.cc and *.pb.h files
# *.proto: option cc_generic_services = true; use for generate rpc code
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O0 -g -ggdb -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

# auto generate *.pb.cc and *.pb.h from proto
# command: # proto --cpp_out=out_dir --proto_path=in_dir path/to/.proto
# --cpp_out: for output dir, *.pb.cc and *.pb.h
# --proto_path: for input dir, where input proto dir
execute_process(COMMAND protoc --cpp_out=${CMAKE_CURRENT_BINARY_DIR}/rpc/ 
                               --proto_path=${CMAKE_CURRENT_BINARY_DIR}/rpc 
                               ${CMAKE_CURRENT_BINARY_DIR}/rpc/echo.proto 
                               ${CMAKE_CURRENT_BINARY_DIR}/rpc/std_rpc_meta.proto)

add_executable( demo 
                main.cc
                net/buffer.cc
                net/acceptor.cc
                net/channel.cc
                net/epoll.cc
                net/event_loop.cc
                net/tcp_connection.cc
                net/tcp_server.cc
                net/tcp_client.cc
                rpc/echo.pb.cc
                rpc/std_rpc_meta.pb.cc
                rpc/rpc_service.cc
                rpc/rpc_channel.cc
                shm/shm_alloc.cc
                shm/shm_hash.cc
                shm/shm_queue.cc
                base/block_queue.cc
                base/sort_merge.cc
                base/thread.cc
                base/public.cc )

# LINKFLAG: -lrt: shm_open
# need after the target
# pthread use for mutli pthread
# protobuf user for protobuf and rpc
target_link_libraries(demo rt pthread protobuf)
