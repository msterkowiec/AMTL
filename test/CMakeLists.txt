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

add_executable(AMTTests "AMTTests_all_features_ON.cpp") 
target_link_libraries(AMTTests gtest_main)

add_executable(AMTTests_only_main_feature_ON "AMTTests_only_main_feature_ON.cpp")
target_link_libraries(AMTTests_only_main_feature_ON gtest_main)

add_executable(AMTTests_only_overflow_check_on "AMTTests_only_overflow_check_on.cpp")
target_link_libraries(AMTTests_only_overflow_check_on gtest_main)

add_executable(AMTTests_all_features_off "AMTTests_all_features_off.cpp") 
target_link_libraries(AMTTests_all_features_off gtest_main)

add_executable(AMTTests_all_features_on_PODTypesDifferentSize "AMTTests_all_features_ON__trivial_types_different_size.cpp")
target_link_libraries(AMTTests_all_features_on_PODTypesDifferentSize gtest_main)

gtest_discover_tests(AMTTests)
gtest_discover_tests(AMTTests_only_main_feature_ON)
gtest_discover_tests(AMTTests_only_overflow_check_on)
gtest_discover_tests(AMTTests_all_features_off)
gtest_discover_tests(AMTLTest_AllFeatures_PODTypesDifferentSize)
