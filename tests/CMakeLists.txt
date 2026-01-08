add_executable(test_ceim_core
    test_ceim_core.cpp
)

target_link_libraries(test_ceim_core
    PRIVATE ceim_core
)

add_test(NAME ceim_core_basic COMMAND test_ceim_core)
