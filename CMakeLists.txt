
cmake_minimum_required (VERSION 3.14)

project ("AMT" CXX)

add_subdirectory ("src")

enable_testing()
add_subdirectory ("test")