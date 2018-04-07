
cmake_minimum_required (VERSION 2.8)

# CXXFLAG: -Wall should be add
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++11 -O0 -g -ggdb -pthread -lrt")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

# LINKFLAG: -lrt: shm_open
# target_link_libraries(rt)

add_executable( demo 
                main.cc
                shmHash.cc
                thread.cc
                public.cc )
