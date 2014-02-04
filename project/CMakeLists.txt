# The build script for elevator project
# parts of this script are inspired by build-script of the DIVINE model-checker
# (DIVINE is licenced under BSD licence, so this should be ok).

project( elevator )

cmake_minimum_required(VERSION 2.8)

# from DIVINE
macro(appendFlag flags value)
  foreach( F ${flags} )
    set( CMAKE_${F}_FLAGS "${CMAKE_${F}_FLAGS} ${value}" )
  endforeach( F )
endmacro(appendFlag)


# setup compiler flags
set( CMAKE_REQUIRED_FLAGS "-std=c++11" ) # for the benefit of GCC
appendFlag( "CXX" "-std=c++11" )


#
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

# plug in unit test
add_custom_target( unit )
add_subdirectory( wibble )
include( wibble/test.cmake )

file( GLOB testh src/*.test.h )
wibble_add_test( elevator-test ${testh} )
wibble_check_target( elevator-test )
add_dependencies( unit elevator-test )
target_link_libraries( elevator-test wibble )
