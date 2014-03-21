# The build script for elevator project
# parts of this script are inspired by build-script of the DIVINE model-checker
# (DIVINE is licenced under BSD licence, so this should be ok).

cmake_minimum_required(VERSION 2.8)

SET( CMAKE_CXX_FLAGS "-Wall" CACHE STRING "" )
SET( CMAKE_C_FLAGS "-Wall" CACHE STRING "" )

# this must be set BEFORE project
SET( CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "" )
SET( CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "" )

SET( CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG" CACHE STRING "" )
SET( CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG" CACHE STRING "" )

SET( CMAKE_CXX_FLAGS_DEBUG "-O0 -g" CACHE STRING "" )
SET( CMAKE_C_FLAGS_DEBUG "-O0 -g" CACHE STRING "" )

project( elevator )

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
        "Choose the type of build options are: Debug Release RelWithDebInfo. Defaulting to Debug!"
         FORCE)
     message( "-- WARNING: Debug chosen for build type, please specify with CMAKE_BUILD_TYPE" )
endif()

# from DIVINE
macro(appendFlag flags value)
  foreach( F ${flags} )
    set( CMAKE_${F}_FLAGS "${CMAKE_${F}_FLAGS} ${value}" )
  endforeach( F )
endmacro(appendFlag)

macro( disabling name reason )
    message( "-- WARNING: ${name} DISABLED. ${reason}" )
    set( DISABLED "${DISABLED} ${name}" )
endmacro()


# setup compiler flags
set( CMAKE_REQUIRED_FLAGS "-std=c++11" ) # for the benefit of GCC
appendFlag( "CXX" "-std=c++11" )


# allow compilation even without libComedi
option( LIBCOMEDI "Whether to use real libComedi driver" ON )
option( WARNING "Enable more warnings and some werror" OFF )
option( INTERACTIVE_UNITS "Enable even interactive unit tests" OFF )

find_path( LIBCOMEDI_PATH comedilib.h )
if ( NOT LIBCOMEDI_PATH )
    set( LIBCOMEDI OFF )
    disabling( "LIBCOMEDI" "" "libcomedi.h not found" )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set( WARNING ON )
    set( INTERACTIVE_UNITS ON )
endif()

if( WARNING )
    appendFlag( "C;CXX" "-Wextra -Wold-style-cast -Werror=sign-compare -Werror=return-type" )
endif()

if( INTERACTIVE_UNITS )
    add_definitions( -DO_INTERACTIVE_UNIT_TESTS )
endif()

#
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

# the target itself
file( GLOB ELEV_SRC elevator/*.cpp )
add_library( libelevator STATIC ${ELEV_SRC} )

if ( LIBCOMEDI )
    add_definitions( -DO_HAVE_LIBCOMEDI )
    target_link_libraries( libelevator comedi )
endif()

# plug in unit test
add_custom_target( unit ALL )
add_definitions( -DPOSIX ) # required by wibble
add_subdirectory( wibble )
include( wibble/test.cmake )

file( GLOB testh elevator/*.test.h )
wibble_add_test( elevator-test ${testh} )
wibble_check_target( elevator-test )
add_dependencies( unit elevator-test )
target_link_libraries( elevator-test libelevator pthread wibble )

add_executable( stop tools/stop.cpp )
target_link_libraries( stop libelevator wibble )

add_executable( elevator tools/main.cpp )
target_link_libraries( elevator libelevator pthread wibble )

