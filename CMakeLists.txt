
cmake_minimum_required (VERSION 2.8)

# boost setting
set(Boost_INCLUDE_DIR /usr/include/)
set(Boost_LIBRARY_DIR /usr/lib/x86_64-linux-gnu/)
find_package(Boost COMPONENTS system filesystem program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

# CXXFLAG: -Wall should be add
# CXXFLAG: ubuntu can not use -lpthread, instead, use -pthread
# Link   : ubuntu use target_link_libraries method to add system lib to target
# *.proto: "protoc --cpp_out=./ *.proto" to generate *.pb.cc and *.pb.h files
# *.proto: option cc_generic_services = true; use for generate rpc code
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O0 -g -ggdb -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

# assembly language support in CMake
set_property(SOURCE third_party/libco/coctx_swap.S PROPERTY LANGUAGE C)

# print variable
# MESSAGE (STATUS ${CMAKE_CURRENT_SOURCE_DIR})

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
                third_party/libco/co_epoll.cpp
                third_party/libco/co_routine.cpp
                third_party/libco/co_hook_sys_call.cpp
                third_party/libco/coctx.cpp
                third_party/libco/coctx_swap.S
                net/buffer.cc
                net/acceptor.cc
                net/channel.cc
                net/epoll.cc
                net/event_loop.cc
                net/tcp_connection.cc
                net/http_server.cc
                net/tcp_server.cc
                net/tcp_client.cc
                net/packet_codec.cc
                net/timer_queue.cc
                rpc/echo.pb.cc
                rpc/std_rpc_meta.pb.cc
                rpc/rpc_service.cc
                rpc/rpc_channel.cc
                rpc/rpc_coroutine.cc
                shm/shm_alloc.cc
                shm/shm_hash.cc
                shm/shm_queue.cc
                base/sort_merge.cc
                base/thread.cc
                base/public.cc )

# LINKFLAG:
# need after the target
# -lrt: use for shm_open
# -lpthread: use for mutli pthread
# -lprotobuf: use for protobuf and rpc
# -ldl: use for dlsym 
target_link_libraries(demo rt pthread protobuf dl cppkafka rdkafka Boost::program_options)
