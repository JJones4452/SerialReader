cmake_minimum_required(VERSION 3.12)
project(SerialReader)

# Compiler-specific configuration
if(MSVC)
    # MSVC-specific settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
else()
    # GCC-specific settings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
endif()

# Add the include directories
include_directories(include)

# Add the source files for SerialReader
set(SERIAL_READER_SRCS
    src/SerialC.cpp
)

# Add the header files for SerialReader
set(SERIAL_READER_HDRS
    include/SerialReader.hpp
    include/SerialC.h
)

# Create the library target
add_library(SerialReader ${SERIAL_READER_SRCS} ${SERIAL_READER_HDRS})

# Set compiler-specific properties for SerialReader
if(MSVC)
    target_compile_definitions(SerialReader PRIVATE -DMSVC_COMPILER)
endif()

# Add an executable for testing
# add_executable(main main.c)
# target_link_libraries(main SerialReader)
