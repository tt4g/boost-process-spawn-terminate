cmake_minimum_required(VERSION 3.4.0)

set(CMAKE_USER_MAKE_RULES_OVERRIDE
  "${CMAKE_CURRENT_LIST_DIR}/CMakeRulesOverride.cmake")

project(spawn_and_terminate VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

################################################################################
# cmake-conan
# https://github.com/conan-io/cmake-conan

# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.14/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_run(
  CONANFILE conanfile.txt
  BASIC_SETUP CMAKE_TARGETS
  BUILD missing)

# cmake-conan
# https://github.com/conan-io/cmake-conan
################################################################################

function(init_options target)
  target_include_directories(${target}
    PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      # $<INSTALL_INTERFACE:include/spawn_and_terminate>
  )

  target_compile_options(${target}
    PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/W4>
      $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<VERSION_GREATER_EQUAL:${MSVC_VERSION},1900>>:/permissive->
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic -Werror>)
endfunction()

add_executable(echo_and_sleep echo_and_sleep.cpp)
init_options(echo_and_sleep)

add_executable(spawn_and_terminate spawn_and_terminate.cpp)
init_options(spawn_and_terminate)

target_link_libraries(spawn_and_terminate
  PUBLIC
    CONAN_PKG::boost)

# Boost.Asio
# Disable autolink: https://www.boost.org/doc/libs/1_70_0/doc/html/boost_asio/using.html#boost_asio.using.dependencies
target_compile_definitions(spawn_and_terminate
  PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:BOOST_DATE_TIME_NO_LIB BOOST_REGEX_NO_LIB>
)

# Fixes MSVC 2017 warning bug: https://github.com/boostorg/asio/issues/109
target_compile_definitions(spawn_and_terminate
  PUBLIC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<VERSION_GREATER_EQUAL:${MSVC_VERSION},1900>>:_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING>)
