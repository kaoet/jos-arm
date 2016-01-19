cmake_minimum_required (VERSION 2.6)
project (JOS C ASM)

set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")

# for language features
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror -Wno-unused-function -g -DJOS_KERNEL")

# for platform independency
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -nostdinc -ffreestanding")

# for target platform
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=vfp -mfloat-abi=soft")

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

option(VERSATILE_PB "Build for Versatile PB" ON)
configure_file (
  "${PROJECT_SOURCE_DIR}/inc/config.h.in"
  "${PROJECT_BINARY_DIR}/inc/config.h"
  )

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}")
add_subdirectory(kern)
