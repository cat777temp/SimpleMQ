#include <QtTest>
#include "broker.h"
#include "logger.h"

class BrokerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingleton();
    void testStartStop();
    void testCacheSize();
};

void BrokerTest::initTestCase()
{
    // 初始化日志系统
    Logger::instance()->init("broker_test.log", Logger::DEBUG);
}

void BrokerTest::cleanupTestCase()
{
    // 确保Broker已停止并释放资源
    Broker::forceCleanup();
}

void BrokerTest::testSingleton()
{
    // 测试单例模式
    Broker* broker1 = Broker::instance();
    Broker* broker2 = Broker::instance();

    QVERIFY(broker1 != nullptr);
    QVERIFY(broker2 != nullptr);
    QCOMPARE(broker1, broker2);
}

void BrokerTest::testStartStop()
{
    Broker* broker = Broker::instance();

    // 测试启动 - 使用不同的端口避免冲突
    bool started = broker->start(5556, "TestBroker");
    QVERIFY(started);
    QVERIFY(broker->isRunning());

    // 测试停止
    broker->stop();
    QVERIFY(!broker->isRunning());

    // 等待一下确保资源释放
    QTest::qWait(100);
}

void BrokerTest::testCacheSize()
{
    Broker* broker = Broker::instance();

    // 测试设置缓存大小
    int cacheSize = 200;
    broker->setCacheSize(cacheSize);
    QCOMPARE(broker->getCacheSize(), cacheSize);

    // 测试设置无效的缓存大小
    broker->setCacheSize(-1);
    QCOMPARE(broker->getCacheSize(), cacheSize); // 应该保持不变

    // 测试清除缓存
    broker->clearCache();
}

QTEST_MAIN(BrokerTest)
#include "broker_test.moc"
