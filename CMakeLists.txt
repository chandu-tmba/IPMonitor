cmake_minimum_required(VERSION 2.8.10)
project (IpMonitor)

set (MONITOR_LINK_LIBS -lsystemd stdc++fs sdbusplus)

add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)


set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -lstdc++fs \
    -Werror \
    -Wall \
    -Wextra \
    -Wnon-virtual-dtor \
    -Wold-style-cast \
    -Wunused \
    -Woverloaded-virtual \
    -Wpedantic \
    -Wmisleading-indentation \
    -Wduplicated-cond \
    -Wduplicated-branches \
    -Wlogical-op \
    -Wnull-dereference \
    -Wuseless-cast \
    -Wdouble-promotion \
    -Wformat=2 \
    -Wno-sign-compare \
    -Wno-reorder \
    -Wno-null-dereference \
    -Wno-unused-variable \
    -Wno-unused-but-set-variable \
    -Wno-unused-parameter \
    -Wno-cast-align \
    -Wno-return-type \
"
)



add_executable (ipmon src/IpMonitor.cpp ${HOSTINV_SRC_FILES})
add_dependencies (ipmon sdbusplus-project)
target_link_libraries (ipmon ${MONITOR_LINK_LIBS})



