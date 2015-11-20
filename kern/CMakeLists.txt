add_executable(kernel entry.S init.c pmap.c console.c printf.c monitor.c ../lib/printfmt.c ../lib/readline.c ../lib/string.c)
target_link_libraries(kernel gcc)
set_target_properties(kernel PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/kernel.ld")
add_custom_command(TARGET kernel POST_BUILD
    COMMAND arm-none-eabi-objdump -S $<TARGET_FILE:kernel> > $<TARGET_FILE:kernel>.asm)