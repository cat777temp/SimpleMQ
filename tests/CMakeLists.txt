cmake_minimum_required(VERSION 3.14)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 查找Qt Test包
find_package(Qt6 COMPONENTS Test REQUIRED)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Test REQUIRED)
endif()

# 消息测试
add_executable(message_test
    message_test.cpp
)

target_link_libraries(message_test
    ${PROJECT_NAME}
    Qt::Core
    Qt::Network
    Qt::Test
)

# 主题测试
add_executable(topic_test
    topic_test.cpp
)

target_link_libraries(topic_test
    ${PROJECT_NAME}
    Qt::Core
    Qt::Network
    Qt::Test
)

# Broker测试
add_executable(broker_test
    broker_test.cpp
)

target_link_libraries(broker_test
    ${PROJECT_NAME}
    Qt::Core
    Qt::Network
    Qt::Test
)

# Publisher测试
add_executable(publisher_test
    publisher_test.cpp
)

target_link_libraries(publisher_test
    ${PROJECT_NAME}
    Qt::Core
    Qt::Network
    Qt::Test
)

# Subscriber测试
add_executable(subscriber_test
    subscriber_test.cpp
)

target_link_libraries(subscriber_test
    ${PROJECT_NAME}
    Qt::Core
    Qt::Network
    Qt::Test
)
