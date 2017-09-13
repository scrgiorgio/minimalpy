cmake_minimum_required (VERSION 3.0)

project (tinypy)

set(CMAKE_CXX_STANDARD 11)

FIND_PACKAGE(PythonInterp   2.7 REQUIRED)
FIND_PACKAGE(PythonLibs     2.7 REQUIRED)

SET(ByteCodeFile ${CMAKE_CURRENT_SOURCE_DIR}/tinypy.bytecode.h)

SET(PyFile       ${CMAKE_CURRENT_SOURCE_DIR}/tinypy.py)

ADD_CUSTOM_TARGET(GenerateByteCode DEPENDS ${ByteCodeFile})

ADD_CUSTOM_COMMAND(
	OUTPUT ${ByteCodeFile} 
	COMMAND ${PYTHON_EXECUTABLE} ${PyFile} 
	DEPENDS ${PyFile}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB SRCS *.h *.cpp *.py)
SOURCE_GROUP("" FILES ${SRCS})
add_executable(tinypy ${SRCS} )
ADD_DEPENDENCIES(tinypy GenerateByteCode)
