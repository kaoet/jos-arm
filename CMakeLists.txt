cmake_minimum_required (VERSION 2.6)
project (JOS C ASM)

set(CMAKE_ASM_COMPILER "arm-none-eabi-gcc")
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")

# for language features
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -fno-builtin")

# for platform independency
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdlib -nostdinc")

# for target platform
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6zk -mcpu=arm1176jzf-s -marm -mfpu=vfp -mfloat-abi=hard")

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

include_directories("${PROJECT_SOURCE_DIR}")
add_subdirectory(kern)