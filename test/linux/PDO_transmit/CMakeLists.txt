set(SOURCES PDO_transmit.c)
add_executable(PDO_transmit ${SOURCES})
target_link_libraries(PDO_transmit soem)
install(TARGETS PDO_transmit DESTINATION bin)