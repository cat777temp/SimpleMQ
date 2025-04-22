# MyMQ - 轻量级消息队列系统

MyMQ是一个基于Qt和C++11实现的轻量级消息队列系统，采用发布/订阅模式，支持TCP和本地套接字通信，适用于进程间通信场景。

## 功能特性

- **发布/订阅模式**：支持基于主题的消息发布和订阅
- **多种通信方式**：支持TCP和本地套接字两种通信方式
- **消息缓存**：支持消息缓存，新订阅者可以接收到订阅前发布的消息
- **自动重连**：客户端支持自动重连功能，提高系统稳定性
- **线程安全**：所有组件都是线程安全的，可以在多线程环境中使用
- **日志系统**：内置日志系统，方便调试和问题排查
- **轻量级**：代码简洁，依赖少，易于集成和使用

## 系统架构

MyMQ系统由以下几个主要组件组成：

- **Broker**：消息代理，负责管理连接和消息路由
- **Publisher**：消息发布者，负责发布消息
- **Subscriber**：消息订阅者，负责订阅和接收消息
- **Message**：消息类，表示消息的数据结构
- **Topic**：主题类，表示消息的主题
- **Logger**：日志系统，负责记录系统运行日志

## 使用方法

### 启动Broker

```cpp
#include "broker.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 初始化日志系统
    Logger::instance()->init("broker.log", Logger::DEBUG);
    
    // 启动Broker
    Broker* broker = Broker::instance();
    if (!broker->start(5555, "MyMQLocalServer")) {
        Logger::instance()->fatal("Failed to start broker");
        return 1;
    }
    
    return app.exec();
}
```

### 发布消息

```cpp
#include "publisher.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 初始化日志系统
    Logger::instance()->init("publisher.log", Logger::DEBUG);
    
    // 创建发布者
    Publisher publisher;
    
    // 设置自动重连
    publisher.setAutoReconnect(true, 5000);
    
    // 连接到Broker
    if (!publisher.connectToBroker("localhost", 5555)) {
        Logger::instance()->warning("Failed to connect to broker, will try to reconnect...");
    }
    
    // 发布消息
    QString topic = "test/topic";
    QByteArray data = "Hello, World!";
    if (publisher.publish(topic, data)) {
        qDebug() << "Published message:" << QString::fromUtf8(data);
    }
    
    return app.exec();
}
```

### 订阅消息

```cpp
#include "subscriber.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 初始化日志系统
    Logger::instance()->init("subscriber.log", Logger::DEBUG);
    
    // 创建订阅者
    Subscriber subscriber;
    
    // 设置自动重连
    subscriber.setAutoReconnect(true, 5000);
    
    // 连接到Broker
    if (!subscriber.connectToBroker("localhost", 5555)) {
        Logger::instance()->warning("Failed to connect to broker, will try to reconnect...");
    }
    
    // 订阅主题
    QString topic = "test/topic";
    if (subscriber.subscribe(topic)) {
        Logger::instance()->info(QString("Subscribed to topic: %1").arg(topic));
    }
    
    // 处理接收到的消息
    QObject::connect(&subscriber, &Subscriber::messageReceived, [](const Message& message) {
        QString messageText = QString::fromUtf8(message.data());
        qDebug() << "Received message on topic" << message.topic() << ":" << messageText;
    });
    
    return app.exec();
}
```

## 编译和运行

### 依赖项

- Qt 5.12或更高版本
- C++11兼容的编译器

### 编译步骤

1. 使用CMake生成项目文件：

```bash
mkdir build
cd build
cmake ..
```

2. 编译项目：

```bash
cmake --build . --config Debug
```

3. 运行示例程序：

```bash
# 先运行Broker
cd build/examples/Debug
./broker_example

# 然后运行Subscriber
./subscriber_example

# 最后运行Publisher
./publisher_example
```

## 许可证

本项目采用MIT许可证。详情请参阅LICENSE文件。

## 贡献

欢迎提交问题和拉取请求。对于重大更改，请先开issue讨论您想要更改的内容。

## 作者

MyMQ团队
