# This file is part of the Bus Pirate project
# (http://code.google.com/p/the-bus-pirate/).
#
# Written and maintained by the Bus Pirate project.
#
# To the extent possible under law, the project has
# waived all copyright and related or neighboring rights to Bus Pirate. This
# work is published from United States.
#
# For details see: http://creativecommons.org/publicdomain/zero/1.0/.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(pirate-loader)
set (SOURCE_FILES pirate-loader.c)
set_property (SOURCE ${SOURCE_FILES} PROPERTY COMPILE_DEFINITIONS OS=${CMAKE_SYSTEM_NAME})

# A stand-alone flasher is nicer to carry around statically linked, but the
# flags are toolchain-specific: GCC/Clang take -static[-libgcc], MSVC takes
# /MT.  Do not hand GCC flags to MSVC (and vice versa).
option (PIRATE_LOADER_STATIC "Link against static runtimes where supported" ON)
if (PIRATE_LOADER_STATIC)
  if (MSVC)
    # MSVC has no -static; use the static CRT (replace the default /MD).
    foreach (flag CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                  CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
      string (REPLACE "/MD" "/MT" ${flag} "${${flag}}")
    endforeach ()
  elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc")
  elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  endif ()
endif ()

add_executable (pirate-loader ${SOURCE_FILES})
