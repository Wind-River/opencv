cmake_minimum_required(VERSION 3.0)

file(GLOB source_list "*.c" "*.h" "*.cpp" "*.hpp")

MESSAGE(${source_list})

#add_executable(cvvxdisplay ${source_list})
add_library(cvvxdisplay ${source_list})