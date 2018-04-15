
cmake_minimum_required (VERSION 2.8)

# CXXFLAG: -Wall should be add
# CXXFLAG: ubuntu can not use -lpthread, instead, use -pthread
# Link   : ubuntu use target_link_libraries method to add system lib to target
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O0 -g -ggdb -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

add_executable( demo 
                main.cc
                shmHash.cc
                sort_merge.cc
                thread.cc
                public.cc )

# LINKFLAG: -lrt: shm_open
# need after the target
# pthread
target_link_libraries(demo rt pthread)
