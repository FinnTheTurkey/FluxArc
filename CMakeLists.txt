cmake_minimum_required(VERSION 3.0.0)
project(FluxArc VERSION 0.1.0)

include(CTest)
enable_testing()

option(FLUXARC_BUILD_DEMO "Whether or not to build the test/demo" OFF)

add_library(FluxArc STATIC Src/FluxArc.cc Include/FluxArc/FluxArc.hh)
target_include_directories(FluxArc PUBLIC Include)

# LZ4
# Not using it's CMake because it's overcomplicated
add_library(lz4 STATIC ThirdParty/lz4/lib/lz4.c ThirdParty/lz4/lib/lz4hc.c)
target_include_directories(lz4 PUBLIC ThirdParty/lz4/lib)
target_link_libraries(FluxArc PUBLIC lz4)


# Tests
if (FLUXARC_BUILD_DEMO)
    add_executable(FluxArcTest Test/Test.cc)
    target_include_directories(FluxArcTest PUBLIC Include)
    target_link_libraries(FluxArcTest PUBLIC FluxArc)
endif()