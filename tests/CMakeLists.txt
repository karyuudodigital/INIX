set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

add_executable(ini_editor_tests
    IniCoreTests.cpp
    ../src/domain/IniDocument.cpp
    ../src/services/IniParser.cpp
    ../src/services/IniSerializer.cpp
    ../src/services/IniDiffService.cpp
    ../src/services/IniMergeService.cpp
    ../src/services/IniSearchService.cpp
)

target_include_directories(ini_editor_tests PRIVATE ../src)
target_link_libraries(ini_editor_tests PRIVATE Qt6::Core Qt6::Test)
target_compile_definitions(ini_editor_tests PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

add_test(NAME ini_editor_tests COMMAND ini_editor_tests)
