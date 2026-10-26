add_executable(prism_math_tests
    test_vec.cpp
    test_mat.cpp
    test_transform.cpp
    test_quaternion.cpp
    test_geometry.cpp
    test_intersection.cpp
)

target_link_libraries(prism_math_tests PRIVATE
    prism_math
    GTest::gtest_main
)

prism_apply_warnings(prism_math_tests)

include(GoogleTest)
gtest_discover_tests(prism_math_tests)
