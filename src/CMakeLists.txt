cmake_minimum_required (VERSION 3.0)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREAD ON)

find_package(Boost COMPONENTS system REQUIRED)  

# Boost
include_directories("${Boost_INCLUDE_DIRS}")
link_directories("${Boost_LIBRARY_DIRS}")
    
# Client
add_executable(Client client.cpp) 
target_link_libraries(Client ${Boost_LIBRARIES})
	
# Server
add_executable(Server server.cpp base64.cpp base64.h strings.h) 
target_link_libraries(Server ${Boost_LIBRARIES})