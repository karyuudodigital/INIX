# CMake generated Testfile for 
# Source directory: C:/Users/Hunter/Documents/GitHub/INIX/tests
# Build directory: C:/Users/Hunter/Documents/GitHub/INIX/build-msvc/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[ini_editor_tests]=] "C:/Users/Hunter/Documents/GitHub/INIX/build-msvc/tests/Debug/ini_editor_tests.exe")
  set_tests_properties([=[ini_editor_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Hunter/Documents/GitHub/INIX/tests/CMakeLists.txt;19;add_test;C:/Users/Hunter/Documents/GitHub/INIX/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[ini_editor_tests]=] "C:/Users/Hunter/Documents/GitHub/INIX/build-msvc/tests/Release/ini_editor_tests.exe")
  set_tests_properties([=[ini_editor_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Hunter/Documents/GitHub/INIX/tests/CMakeLists.txt;19;add_test;C:/Users/Hunter/Documents/GitHub/INIX/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[ini_editor_tests]=] "C:/Users/Hunter/Documents/GitHub/INIX/build-msvc/tests/MinSizeRel/ini_editor_tests.exe")
  set_tests_properties([=[ini_editor_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Hunter/Documents/GitHub/INIX/tests/CMakeLists.txt;19;add_test;C:/Users/Hunter/Documents/GitHub/INIX/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[ini_editor_tests]=] "C:/Users/Hunter/Documents/GitHub/INIX/build-msvc/tests/RelWithDebInfo/ini_editor_tests.exe")
  set_tests_properties([=[ini_editor_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/Hunter/Documents/GitHub/INIX/tests/CMakeLists.txt;19;add_test;C:/Users/Hunter/Documents/GitHub/INIX/tests/CMakeLists.txt;0;")
else()
  add_test([=[ini_editor_tests]=] NOT_AVAILABLE)
endif()
