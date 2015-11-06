cmake_minimum_required (VERSION 3.0)
project (CSGames) 

if(NOT WIN32)
	set(CMAKE_CXX_FLAGS "-std=c++14 -pthread -g" CACHE STRING "" FORCE)
endif()

if(WIN32)
    file(COPY ${PROJECT_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${PROJECT_SOURCE_DIR}/start_comp.py DESTINATION ${CMAKE_BINARY_DIR})
else()
    file(COPY ${PROJECT_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
    file(COPY ${PROJECT_SOURCE_DIR}/start_comp.py DESTINATION ${CMAKE_BINARY_DIR})
endif()

add_subdirectory (src)

cmake_policy(SET CMP0015 NEW)
