add_executable(kernel entry.S init.c console.c printf.c ../lib/printfmt.c ../lib/string.c)
target_link_libraries(kernel gcc)