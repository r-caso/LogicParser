include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST "OFF")
FetchContent_MakeAvailable(googletest)

add_executable(logic_parser_tests parser_tests.cpp)
target_link_libraries(logic_parser_tests PRIVATE LogicParser GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(logic_parser_tests)
