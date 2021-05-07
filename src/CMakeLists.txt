#
# Assertive MultiThreading Library
#
#  Copyright Marcin Sterkowiec, Piotr Tracz, 2021. Use, modification and
#  distribution is subject to license (see accompanying file license.txt)
#

cmake_minimum_required (VERSION 3.14)


# Add source to this project's executable.
add_library (AMT "amt_pod.h" "amt_config.h" "amtinternal_hashmap.h" "amt_types.h" "amtinternal_utils.h" "amt_compat.h" "amt_vector.h" "amt_rawdatadebugchecker.h" "amt_map.h" )
set_target_properties(AMT PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries (AMT)

# TODO: Add tests and install targets if needed.
