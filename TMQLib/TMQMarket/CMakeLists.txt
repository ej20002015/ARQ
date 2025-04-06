TMQ_define_dynalib(TMQMarket)
append_link_libraries(TMQMarket TMQUtils)

TMQ_define_dynalib_tests(TMQMarket)
append_link_libraries(TMQMarket TMQUtils)

TMQ_define_dynalib_bench(TMQMarket)
append_link_libraries(TMQMarket TMQUtils)
add_custom_command(TARGET b_TMQMarket POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "b_TMQMarket built at: $<TARGET_FILE:b_TMQMarket>"
)