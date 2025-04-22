cmake_minimum_required(VERSION 3.14)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 发布者示例
add_executable(publisher_example
    publisher_example.cpp
)

target_link_libraries(publisher_example
    ${PROJECT_NAME}
    Qt::Core
    Qt::Network
)

# 订阅者示例
add_executable(subscriber_example
    subscriber_example.cpp
)

target_link_libraries(subscriber_example
    ${PROJECT_NAME}
    Qt::Core
    Qt::Network
)

# Broker示例
add_executable(broker_example
    broker_example.cpp
)

target_link_libraries(broker_example
    ${PROJECT_NAME}
    Qt::Core
    Qt::Network
)
