# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(destTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(dest_test source/dest_test.cpp)
target_link_libraries(
    dest_test PRIVATE
    dest_lib
    Catch2::Catch2WithMain
)
target_compile_features(dest_test PRIVATE cxx_std_17)

catch_discover_tests(dest_test)

# ---- End-of-file commands ----

add_folders(Test)
