cmake_minimum_required(VERSION 3.17)
project(percentile_finder)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(DCMAKE_GENERATOR_PLATFORM x64)

set(CMAKE_CXX_FLAGS_DEBUG )
set(CMAKE_CXX_FLAGS_RELEASE "/analyze /sdl /GS /guard:cf /Ox /GL /Gv /arch:AVX2 /EHsc")

include_directories(src)

set (DCMAKE_TOOLCHAIN_FILE C:/Soft/vcpkg/scripts/buildsystems/vcpkg.cmake)

include_directories("C:/Program Files (x86)/Intel/oneAPI/tbb/2021.4.0/include")
link_directories("C:/Program Files (x86)/Intel/oneAPI/tbb/2021.4.0/lib/intel64/vc14")
include_directories("C:/Soft/vcpkg/packages/opencl_x64-windows/include")
link_directories("C:/Soft/vcpkg/packages/opencl_x64-windows/lib")

add_executable(percentile_finder
        src/percentile_finder.cpp
        src/default_config.h
        src/utils.cpp
        src/number_masker_serial.cpp
        src/resolver.cpp
        src/resolver.h
        src/resolver_parallel.cpp
        src/resolver_parallel.h
        src/resolver_serial.cpp
        src/resolver_serial.h
        src/watchdog.cpp
        src/bit_masker.h
        src/open_cl_default_header.h
        src/resolver_opencl.cpp
        src/resolver_opencl.h)