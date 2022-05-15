# CMakeList.txt : CMake project for YouiTVHiringCmake, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.14)


# TODO: Add tests and install targets if needed.

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.10.x.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${AMT_SOURCE_DIR}/src)
include(GoogleTest)

if (MSVC)
  add_definitions(/bigobj)
endif ()

add_executable(AMTTests "AMTTests.cpp")

target_link_libraries(AMTTests gtest_main)


gtest_discover_tests(AMTTests)
