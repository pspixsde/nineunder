add_executable(nu_tests
    src/main.cpp
    src/test_handles.cpp
    src/test_math.cpp
    src/test_time.cpp
)

target_link_libraries(nu_tests PRIVATE nu::engine doctest::doctest)
add_test(NAME nu_tests COMMAND nu_tests)
