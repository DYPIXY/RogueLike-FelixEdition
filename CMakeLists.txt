cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    dest
    VERSION 0.1.0
    DESCRIPTION "Short description"
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----
add_subdirectory(
  lib/sfml
  ${PROJECT_SOURCE_DIR}/include
)

add_library(
    dest_lib OBJECT
    source/Game.cpp
    source/MapManager.cpp
)

target_link_libraries(dest_lib
    PUBLIC
        SFML::Graphics
        SFML::Window
        SFML::System
)

project(sfml NONE)


target_include_directories(
    dest_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(dest_lib PUBLIC cxx_std_17)

find_package(fmt REQUIRED)
target_link_libraries(dest_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(dest_exe source/main.cpp)
add_executable(dest::exe ALIAS dest_exe)

set_property(TARGET dest_exe PROPERTY OUTPUT_NAME dest)

target_compile_features(dest_exe PRIVATE cxx_std_17)

target_link_libraries(dest_exe PRIVATE dest_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT dest_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of dest"
  )
endif()

include(cmake/dev-mode.cmake)
