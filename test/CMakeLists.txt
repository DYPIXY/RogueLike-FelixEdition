# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(m3Tests LANGUAGES CXX)

# ---- Tests ----

add_executable(m3_test source/m3_test.cpp)
target_link_libraries(m3_test PRIVATE m3_lib)
target_compile_features(m3_test PRIVATE cxx_std_17)

add_test(NAME m3_test COMMAND m3_test)

# ---- End-of-file commands ----

add_folders(Test)
