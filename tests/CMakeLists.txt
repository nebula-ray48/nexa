include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(nexa-test unit/test_compiler.cpp)

target_link_libraries(nexa-test PRIVATE nexa-core GTest::gtest_main)

set_nexa_warnings(nexa-test)

include(GoogleTest)
gtest_discover_tests(nexa-test)