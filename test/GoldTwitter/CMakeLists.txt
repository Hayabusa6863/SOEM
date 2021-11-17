add_executable(read_motor read_motor.c)
target_link_libraries(read_motor soem)
install(TARGETS read_motor DESTINATION bin)