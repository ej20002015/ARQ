ARQ_define_dynalib(ARQRedis)
append_link_libraries(ARQRedis ARQCore ARQMarket redis++::redis++)

ARQ_define_dynalib_tests(ARQRedis)
