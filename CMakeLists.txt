cmake_minimum_required (VERSION 3.0)
project (CSGames) 

add_subdirectory (src)

if(NOT WIN32)
	set(CMAKE_CXX_FLAGS "-std=c++11 -pthread" CACHE STRING "" FORCE)
endif()

cmake_policy(SET CMP0015 NEW)
