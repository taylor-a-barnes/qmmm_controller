cmake_minimum_required(VERSION 3.5)

project(Controller
	LANGUAGES Fortran)

#set(MY_VARIABLE "This is a variable")

#message(STATUS "This is the MolSSI Hello Project")

#message(STATUS "MY_VARIABLE: ${MY_VARIABLE}")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w3")

#find_package(OpenMP)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

add_subdirectory(src)
