cmake_minimum_required(VERSION 3.20)
project(haystack_proj LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(haystack main.cpp)
add_executable(unit_tests
  tests/test_basic.cpp
  tests/test_tokenizer.cpp
  tests/test_inverted_index.cpp
  tests/test_query_parser.cpp
  tests/test_search_service.cpp
  tests/test_bm25.cpp
  tests/test_snippet.cpp
  tests/test_thread_safety.cpp
  tests/test_persistence.cpp
  tests/test_persistence_determinism.cpp
  tests/test_persistence_errors.cpp
  tests/test_persistence_crash_safety.cpp
  tests/test_cli_persistence.cpp
  tests/test_cli_validation.cpp
  tests/test_cli_lifecycle.cpp
  tests/test_cli_error_messages.cpp
  tests/test_cli_exit_codes.cpp
)

target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)

enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)
add_library(search_core
  src/core/tokenizer.cpp
  src/core/inverted_index.cpp
  src/core/query_parser.cpp
  src/core/search_service.cpp
  src/core/snippet.cpp
)
target_include_directories(search_core PUBLIC src)
target_compile_features(search_core PUBLIC cxx_std_17)


target_link_libraries(haystack PRIVATE search_core)
target_link_libraries(unit_tests PRIVATE search_core)

find_package(Drogon CONFIG REQUIRED)
target_link_libraries(search_core PUBLIC Drogon::Drogon)
add_executable(searchd apps/searchd/main.cpp)
target_compile_definitions(searchd PRIVATE SEARCHD_PORT=8900)
target_link_libraries(searchd PRIVATE search_core Drogon::Drogon)



