file(GLOB_RECURSE MYGIT_SRC CONFIGURE_DEPENDS *.cpp)

add_executable(mygit ${MYGIT_SRC})

target_include_directories(mygit PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(mygit PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto
    ZLIB::ZLIB
)