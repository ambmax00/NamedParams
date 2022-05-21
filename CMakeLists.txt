cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(NamedParams)

enable_language(CXX)
enable_testing()

set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_CXX_STANDARD 20)

add_executable(TestNamedParamsExe test/TestNamedParams.cpp)

add_test(
  NAME TestNamedParams        
  COMMAND ${CMAKE_BINARY_DIR}/TestNamedParamsExe 3)

try_compile(
  SUCCESS 
  ${CMAKE_BINARY_DIR} 
  SOURCES ${CMAKE_SOURCE_DIR}/test/ThisWillNotCompile.cpp
  OUTPUT_VARIABLE COMPILER_OUTPUT
)

if (${SUCCESS})
	message(WARNING "test/ThisWillNotCompile.cpp should not compile!")
endif()

set(OUTPUT_FILENAME ${CMAKE_BINARY_DIR}/CompilerOutput.txt)

message(STATUS "Writing compiler output to ${OUTPUT_FILENAME}")

file(WRITE ${OUTPUT_FILENAME} ${COMPILER_OUTPUT})

configure_file(
	test/TestCompilationFail.cpp 
	${CMAKE_BINARY_DIR}/TestCompilationFail.cpp
	@ONLY
)

add_executable(TestCompilationFailExe ${CMAKE_BINARY_DIR}/TestCompilationFail.cpp)

add_test(
  NAME TestCompilationFail
  COMMAND ${CMAKE_BINARY_DIR}/TestCompilationFailExe
)
