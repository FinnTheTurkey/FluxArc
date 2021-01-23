cmake_minimum_required(VERSION 3.0.0)
project(FluxArc VERSION 0.1.0)

add_library(AsyncFiles STATIC Src/AsyncFiles.cc Include/AsyncFiles/AsyncFiles.hh)
target_include_directories(AsyncFiles PUBLIC Include)
