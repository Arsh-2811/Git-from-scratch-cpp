cmake_minimum_required(VERSION 3.20)
project(mygit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${OPENSSL_INCLUDE_DIR})

add_subdirectory(src)