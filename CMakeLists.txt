cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(NamedParams)

enable_language(CXX)
enable_testing()

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_STANDARD 17)

add_executable(TestNamedParamsExe TestNamedParams.cpp)

add_test(
  NAME TestNamedParams        
  COMMAND ${CMAKE_BINARY_DIR}/TestNamedParamsExe 3)
