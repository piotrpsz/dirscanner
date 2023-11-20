cmake_minimum_required(VERSION 3.26)
project(dirscanner)

set(CMAKE_CXX_STANDARD 20)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-Og")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

find_package(range-v3 REQUIRED)
find_package(TBB REQUIRED)
find_package(fmt REQUIRED)
find_package(Threads REQUIRED)
find_package(date REQUIRED)

add_executable(dirscanner main.cpp)
add_subdirectory(share)
add_subdirectory(clap)

target_link_libraries(dirscanner PUBLIC
        fmt::fmt
        Threads::Threads
        date::date date::date-tz
        TBB::tbb TBB::tbbmalloc TBB::tbbmalloc_proxy
        range-v3::meta range-v3::concepts range-v3::range-v3
        share
        clap
)
