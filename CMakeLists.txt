cmake_minimum_required(VERSION 3.17)
project(percentile_finder)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(src)

include_directories("C:/Program Files (x86)/Intel/oneAPI/tbb/2021.4.0/include")
link_directories("C:/Program Files (x86)/Intel/oneAPI/tbb/2021.4.0/lib/intel64/vc14")

add_executable(percentile_finder
        src/percentile_finder.cpp
        src/default_config.h
        src/file_reader.cpp
        src/number_masker_serial.cpp
        src/resolver.cpp
        src/resolver.h
        src/resolver_parallel.cpp
        src/resolver_parallel.h
        src/resolver_serial.cpp
        src/resolver_serial.h
        src/resource.h
        src/watchdog.cpp
        src/watchdog.h)