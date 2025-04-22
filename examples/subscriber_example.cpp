#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>

#include "subscriber.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 设置应用信息
    QCoreApplication::setApplicationName("MyMQ Subscriber");
    QCoreApplication::setApplicationVersion("1.0");

    // 创建命令行解析器
    QCommandLineParser parser;
    parser.setApplicationDescription("MyMQ Subscriber Example");
    parser.addHelpOption();
    parser.addVersionOption();

    // 添加主机选项
    QCommandLineOption hostOption(QStringList() << "H" << "host",
                                "Broker host.",
                                "host", "localhost");
    parser.addOption(hostOption);

    // 添加TCP端口选项
    QCommandLineOption tcpPortOption(QStringList() << "p" << "port",
                                    "Broker TCP port.",
                                    "port", "5555");
    parser.addOption(tcpPortOption);

    // 添加本地服务器选项
    QCommandLineOption localServerOption(QStringList() << "s" << "server",
                                        "Use local server instead of TCP.",
                                        "name", "MyMQLocalServer");
    parser.addOption(localServerOption);

    // 添加主题选项
    QCommandLineOption topicOption(QStringList() << "t" << "topic",
                                "Topic to subscribe to.",
                                "topic", "test/topic");
    parser.addOption(topicOption);

    // 添加日志文件选项
    QCommandLineOption logFileOption(QStringList() << "l" << "log",
                                    "Log file path.",
                                    "path", "subscriber.log");
    parser.addOption(logFileOption);

    // 解析命令行参数
    parser.process(app);

    // 获取参数值
    QString host = parser.value(hostOption);
    int tcpPort = parser.value(tcpPortOption).toInt();
    QString localServerName = parser.value(localServerOption);
    QString topic = parser.value(topicOption);
    QString logFilePath = parser.value(logFileOption);
    bool useLocalServer = parser.isSet(localServerOption);

    // 初始化日志系统
    if (!Logger::instance()->init(logFilePath, Logger::DEBUG)) {
        qCritical() << "Failed to initialize logger";
        return 1;
    }

    // 创建订阅者
    Subscriber subscriber;

    // 设置自动重连
    subscriber.setAutoReconnect(true, 5000);

    // 连接信号槽
    QObject::connect(&subscriber, &Subscriber::connected, []() {
        Logger::instance()->info("Connected to broker");
    });

    QObject::connect(&subscriber, &Subscriber::disconnected, []() {
        Logger::instance()->info("Disconnected from broker");
    });

    QObject::connect(&subscriber, &Subscriber::subscribed, [](const QString& topic) {
        Logger::instance()->info(QString("Subscribed to topic: %1").arg(topic));
    });

    QObject::connect(&subscriber, &Subscriber::unsubscribed, [](const QString& topic) {
        Logger::instance()->info(QString("Unsubscribed from topic: %1").arg(topic));
    });

    QObject::connect(&subscriber, &Subscriber::messageReceived, [](const Message& message) {
        QString messageText = QString::fromUtf8(message.data());
        Logger::instance()->info(QString("Received message on topic %1: %2").arg(message.topic()).arg(messageText));
        qDebug() << "Received:" << messageText;
    });

    QObject::connect(&subscriber, &Subscriber::error, [](const QString& errorMessage) {
        Logger::instance()->error(QString("Error: %1").arg(errorMessage));
    });

    // 连接到Broker
    bool connected = false;
    if (useLocalServer) {
        connected = subscriber.connectToLocalBroker(localServerName);
        Logger::instance()->info(QString("Connecting to local broker: %1").arg(localServerName));
    } else {
        connected = subscriber.connectToBroker(host, tcpPort);
        Logger::instance()->info(QString("Connecting to broker: %1:%2").arg(host).arg(tcpPort));
    }

    if (!connected) {
        Logger::instance()->warning("Failed to connect to broker, will try to reconnect...");
    } else {
        // 订阅主题
        if (subscriber.subscribe(topic)) {
            Logger::instance()->info(QString("Subscribed to topic: %1").arg(topic));
        } else {
            Logger::instance()->warning(QString("Failed to subscribe to topic: %1").arg(topic));
        }
    }

    qDebug() << "Subscriber started. Press Ctrl+C to quit";

    return app.exec();
}
