cmake_minimum_required (VERSION 3.0)
project (CSGames) 

add_subdirectory (src)

if(NOT WIN32)
	set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -g" CACHE STRING "" FORCE)
endif()

if(WIN32)
    message("TODO")
else()
    file(COPY ${PROJECT_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${PROJECT_SOURCE_DIR}/start_comp.sh DESTINATION ${CMAKE_BINARY_DIR})
endif()

cmake_policy(SET CMP0015 NEW)
