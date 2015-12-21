cmake_minimum_required (VERSION 2.6)
project (JOS C ASM)

set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")

# for language features
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Werror -g -DJOS_KERNEL")

# for platform independency
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -nostdinc -ffreestanding")

# for target platform
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6zk -mcpu=arm1176jzf-s -marm -mfpu=vfp -mfloat-abi=hard")

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

set(TARGET_MACHINE, "VERSATILE_PB")
configure_file (
  "${PROJECT_SOURCE_DIR}/inc/config.h.in"
  "${PROJECT_BINARY_DIR}/inc/config.h"
  )

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}")
add_subdirectory(kern)