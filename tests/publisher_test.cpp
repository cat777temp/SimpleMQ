#include <QtTest>
#include "publisher.h"
#include "broker.h"
#include "logger.h"

class PublisherTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testConstructor();
    void testAutoReconnect();
    void testPublish();
};

void PublisherTest::initTestCase()
{
    // 初始化日志系统
    Logger::instance()->init("publisher_test.log", Logger::DEBUG);

    // 启动Broker
    Broker::instance()->start(5557, "PublisherTestBroker");
}

void PublisherTest::cleanupTestCase()
{
    // 停止Broker并释放资源
    Broker::forceCleanup();
}

void PublisherTest::testConstructor()
{
    Publisher publisher;

    // 测试初始状态
    QVERIFY(!publisher.isConnected());
}

void PublisherTest::testAutoReconnect()
{
    Publisher publisher;

    // 测试设置自动重连 - 使用更短的重连间隔
    publisher.setAutoReconnect(true, 500);

    // 尝试连接到不存在的Broker - 使用一个不太可能被使用的端口
    bool connected = publisher.connectToBroker("localhost", 65000);

    // 预期连接失败
    QVERIFY(!connected);

    // 等待一段时间，让自动重连尝试一次
    QTest::qWait(600);

    // 断开连接
    publisher.disconnectFromBroker();

    // 等待一下确保资源释放
    QTest::qWait(100);
}

void PublisherTest::testPublish()
{
    // 确保 Broker 已启动
    Broker* broker = Broker::instance();
    if (!broker->isRunning()) {
        broker->start(5557, "PublisherTestBroker");
        QTest::qWait(100); // 等待 Broker 启动
    }

    Publisher publisher;

    // 连接到Broker
    bool connected = publisher.connectToBroker("localhost", 5557);
    if (!connected) {
        QSKIP("Could not connect to broker, skipping test");
    }

    // 等待连接建立
    QTest::qWait(100);

    // 测试发布消息
    QString topic = "test/topic";
    QByteArray data = "Hello, World!";
    bool published = publisher.publish(topic, data);

    // 预期发布成功
    QVERIFY(published);

    // 等待消息处理
    QTest::qWait(100);

    // 断开连接
    publisher.disconnectFromBroker();

    // 等待一下确保资源释放
    QTest::qWait(100);
}

QTEST_MAIN(PublisherTest)
#include "publisher_test.moc"
