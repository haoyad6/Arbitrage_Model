cmake_minimum_required(VERSION 3.16)
project(arbitrage)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Boost 和 OpenSSL
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

# 添加 ZeroMQ
find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_ZMQ REQUIRED libzmq)
include_directories(${PC_ZMQ_INCLUDE_DIRS})
link_directories(${PC_ZMQ_LIBRARY_DIRS})

# 源文件
set(SOURCES
    main.cpp
    arbitrage_monitor.cpp
    arbitrage_strategy.cpp
)

# 构建可执行文件
add_executable(arbitrage ${SOURCES})

# 添加头文件目录
target_include_directories(arbitrage PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${PC_ZMQ_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR} # 包含 base_wss_client.hpp, okx_wss_refactored.hpp, binanceus_wss_refactored.hpp
)

# 链接库
target_link_libraries(arbitrage
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${PC_ZMQ_LIBRARIES}
    pthread
)

# 协程支持
target_compile_definitions(arbitrage PRIVATE BOOST_ASIO_HAS_CO_AWAIT)
