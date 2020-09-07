cmake_minimum_required(VERSION 3.15)

project(Chat)

#*****************************************************************

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#*****************************************************************

set(SERVER_SOURCES 
	src/configuration.h
	server/server.h
	server/server.cpp
	server/main.cpp
)

set(CLIENT_SOURCES 
	src/configuration.h
	client/client.h
	client/client.cpp
	client/main.cpp
)

#*****************************************************************

add_executable(${PROJECT_NAME}_SERVER)
add_executable(${PROJECT_NAME}_Client)

#*****************************************************************

target_sources(${PROJECT_NAME}_SERVER PRIVATE ${SERVER_SOURCES})
target_sources(${PROJECT_NAME}_Client PRIVATE ${CLIENT_SOURCES})