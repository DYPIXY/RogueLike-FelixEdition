cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    m3
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    m3_lib OBJECT
    source/lib.cpp
)

add_subdirectory(
  lib/sfml
)


target_include_directories(
    m3_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(m3_lib PUBLIC cxx_std_17)

# ---- Declare executable ----

add_executable(m3_exe source/main.cpp)
add_executable(m3::exe ALIAS m3_exe)

set_property(TARGET m3_exe PROPERTY OUTPUT_NAME m3)

target_compile_features(m3_exe PRIVATE cxx_std_17)

target_link_libraries(m3_exe PRIVATE m3_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT m3_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of m3"
  )
endif()

include(cmake/dev-mode.cmake)
