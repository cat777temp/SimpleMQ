cmake_minimum_required(VERSION 3.14)

project(MyMQ VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动包含当前目录中的cmake子模块
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找Qt包
find_package(Qt6 COMPONENTS Core Network REQUIRED)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Core Network REQUIRED)
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 源文件
set(SOURCES
    src/broker.cpp
    src/publisher.cpp
    src/subscriber.cpp
    src/topic.cpp
    src/message.cpp
    src/logger.cpp
)

# 头文件
set(HEADERS
    include/broker.h
    include/publisher.h
    include/subscriber.h
    include/topic.h
    include/message.h
    include/logger.h
)

# 创建库
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

# 链接Qt库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Network
)

# 示例程序
add_subdirectory(examples)

# 测试程序
add_subdirectory(tests)
