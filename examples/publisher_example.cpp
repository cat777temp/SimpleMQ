#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QTimer>
#include <QDateTime>

#include "publisher.h"
#include "logger.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 设置应用信息
    QCoreApplication::setApplicationName("MyMQ Publisher");
    QCoreApplication::setApplicationVersion("1.0");

    // 创建命令行解析器
    QCommandLineParser parser;
    parser.setApplicationDescription("MyMQ Publisher Example");
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
                                "Topic to publish to.",
                                "topic", "test/topic");
    parser.addOption(topicOption);

    // 添加间隔选项
    QCommandLineOption intervalOption(QStringList() << "i" << "interval",
                                    "Publish interval in milliseconds.",
                                    "ms", "1000");
    parser.addOption(intervalOption);

    // 添加日志文件选项
    QCommandLineOption logFileOption(QStringList() << "l" << "log",
                                    "Log file path.",
                                    "path", "publisher.log");
    parser.addOption(logFileOption);

    // 解析命令行参数
    parser.process(app);

    // 获取参数值
    QString host = parser.value(hostOption);
    int tcpPort = parser.value(tcpPortOption).toInt();
    QString localServerName = parser.value(localServerOption);
    QString topic = parser.value(topicOption);
    int interval = parser.value(intervalOption).toInt();
    QString logFilePath = parser.value(logFileOption);
    bool useLocalServer = parser.isSet(localServerOption);

    // 初始化日志系统
    if (!Logger::instance()->init(logFilePath, Logger::DEBUG)) {
        qCritical() << "Failed to initialize logger";
        return 1;
    }

    // 创建发布者
    Publisher publisher;

    // 设置自动重连
    publisher.setAutoReconnect(true, 5000);

    // 连接信号槽
    QObject::connect(&publisher, &Publisher::connected, []() {
        Logger::instance()->info("Connected to broker");
    });

    QObject::connect(&publisher, &Publisher::disconnected, []() {
        Logger::instance()->info("Disconnected from broker");
    });

    QObject::connect(&publisher, &Publisher::published, [](const QString& messageId) {
        Logger::instance()->info(QString("Message published: %1").arg(messageId));
    });

    QObject::connect(&publisher, &Publisher::error, [](const QString& errorMessage) {
        Logger::instance()->error(QString("Error: %1").arg(errorMessage));
    });

    // 连接到Broker
    bool connected = false;
    if (useLocalServer) {
        connected = publisher.connectToLocalBroker(localServerName);
        Logger::instance()->info(QString("Connecting to local broker: %1").arg(localServerName));
    } else {
        connected = publisher.connectToBroker(host, tcpPort);
        Logger::instance()->info(QString("Connecting to broker: %1:%2").arg(host).arg(tcpPort));
    }

    if (!connected) {
        Logger::instance()->warning("Failed to connect to broker, will try to reconnect...");
    }

    // 创建定时器，定期发布消息
    QTimer publishTimer;
    QObject::connect(&publishTimer, &QTimer::timeout, [&publisher, topic]() {
        // 创建消息数据
        QString messageText = QString("Hello from publisher! Time: %1")
                                .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));

        // 发布消息
        if (publisher.publish(topic, messageText.toUtf8())) {
            qDebug() << "Published message:" << messageText;
        } else {
            qDebug() << "Failed to publish message";
        }
    });

    // 启动定时器
    publishTimer.start(interval);

    qDebug() << "Publisher started. Press Ctrl+C to quit";

    return app.exec();
}
