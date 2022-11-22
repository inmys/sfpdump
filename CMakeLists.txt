cmake_minimum_required(VERSION 3.16)
project(sfpdump)

set(CMAKE_CXX_STANDARD 14)

#
add_executable(sfpdump 
	sfpdump.cpp
        )

add_executable(qsfpdump
        qsfpdump.cpp
	)

