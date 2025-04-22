#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QThread>

#include "broker.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    // 设置应用信息
    QCoreApplication::setApplicationName("MyMQ Broker");
    QCoreApplication::setApplicationVersion("1.0");
    
    // 创建命令行解析器
    QCommandLineParser parser;
    parser.setApplicationDescription("MyMQ Broker Example");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // 添加TCP端口选项
    QCommandLineOption tcpPortOption(QStringList() << "p" << "port",
                                    "TCP port to listen on.",
                                    "port", "5555");
    parser.addOption(tcpPortOption);
    
    // 添加本地服务器名称选项
    QCommandLineOption localServerOption(QStringList() << "s" << "server",
                                        "Local server name.",
                                        "name", "MyMQLocalServer");
    parser.addOption(localServerOption);
    
    // 添加日志文件选项
    QCommandLineOption logFileOption(QStringList() << "l" << "log",
                                    "Log file path.",
                                    "path", "broker.log");
    parser.addOption(logFileOption);
    
    // 解析命令行参数
    parser.process(app);
    
    // 获取参数值
    int tcpPort = parser.value(tcpPortOption).toInt();
    QString localServerName = parser.value(localServerOption);
    QString logFilePath = parser.value(logFileOption);
    
    // 初始化日志系统
    if (!Logger::instance()->init(logFilePath, Logger::DEBUG)) {
        qCritical() << "Failed to initialize logger";
        return 1;
    }
    
    // 启动Broker
    Broker* broker = Broker::instance();
    if (!broker->start(tcpPort, localServerName)) {
        Logger::instance()->fatal("Failed to start broker");
        return 1;
    }
    
    // 连接信号槽
    QObject::connect(broker, &Broker::clientConnected, [](const QString& clientId) {
        Logger::instance()->info(QString("Client connected: %1").arg(clientId));
    });
    
    QObject::connect(broker, &Broker::clientDisconnected, [](const QString& clientId) {
        Logger::instance()->info(QString("Client disconnected: %1").arg(clientId));
    });
    
    QObject::connect(broker, &Broker::messageReceived, [](const Message& message) {
        Logger::instance()->info(QString("Message received: %1").arg(message.topic()));
    });
    
    QObject::connect(broker, &Broker::messagePublished, [](const Message& message) {
        Logger::instance()->info(QString("Message published: %1").arg(message.topic()));
    });
    
    Logger::instance()->info(QString("Broker started. TCP port: %1, Local server: %2").arg(tcpPort).arg(localServerName));
    qDebug() << "Press Ctrl+C to quit";
    
    return app.exec();
}
