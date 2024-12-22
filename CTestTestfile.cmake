# CMake generated Testfile for 
# Source directory: /Users/nipunhedaoo/Documents/GitHub/sentry-native
# Build directory: /Users/nipunhedaoo/Documents/GitHub/sentry-native
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(sentry_example "/Users/nipunhedaoo/Documents/GitHub/sentry-native/sentry_example")
set_tests_properties(sentry_example PROPERTIES  _BACKTRACE_TRIPLES "/Users/nipunhedaoo/Documents/GitHub/sentry-native/CMakeLists.txt;603;add_test;/Users/nipunhedaoo/Documents/GitHub/sentry-native/CMakeLists.txt;0;")
subdirs("src")
subdirs("crashpad_build")
subdirs("tests/unit")
