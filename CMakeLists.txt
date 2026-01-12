cmake_minimum_required(VERSION 3.10)
project(CounterStrikeHelper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/leetify_client.cpp
    src/discord_client.cpp
    src/openai_client.cpp
    src/match_data.cpp
    src/config.cpp
    main.cpp
)

# Header files
set(HEADERS
    include/leetify_client.h
    include/discord_client.h
    include/openai_client.h
    include/match_data.h
    include/config.h
    include/json.hpp
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link OpenSSL for httplib
find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenSSL::SSL OpenSSL::Crypto)

# Compiler flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Copy .env file to build directory if it exists (optional - only copies if source exists)
if(EXISTS "${CMAKE_SOURCE_DIR}/.env")
    configure_file("${CMAKE_SOURCE_DIR}/.env" "${CMAKE_BINARY_DIR}/.env" COPYONLY)
    message(STATUS ".env file will be copied to build directory")
endif()
