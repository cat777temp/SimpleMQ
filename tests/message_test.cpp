#include <QtTest>
#include "message.h"

class MessageTest : public QObject
{
    Q_OBJECT

private slots:
    void testConstructor();
    void testSettersAndGetters();
    void testSerializeDeserialize();
};

void MessageTest::testConstructor()
{
    // 测试默认构造函数
    Message message1;
    QVERIFY(!message1.id().isEmpty());
    QVERIFY(message1.topic().isEmpty());
    QVERIFY(message1.data().isEmpty());
    QVERIFY(message1.timestamp().isValid());
    
    // 测试带参数的构造函数
    QString topic = "test/topic";
    QByteArray data = "Hello, World!";
    Message message2(topic, data);
    QVERIFY(!message2.id().isEmpty());
    QCOMPARE(message2.topic(), topic);
    QCOMPARE(message2.data(), data);
    QVERIFY(message2.timestamp().isValid());
}

void MessageTest::testSettersAndGetters()
{
    Message message;
    
    // 测试topic的setter和getter
    QString topic = "test/topic";
    message.setTopic(topic);
    QCOMPARE(message.topic(), topic);
    
    // 测试data的setter和getter
    QByteArray data = "Hello, World!";
    message.setData(data);
    QCOMPARE(message.data(), data);
}

void MessageTest::testSerializeDeserialize()
{
    // 创建原始消息
    QString topic = "test/topic";
    QByteArray data = "Hello, World!";
    Message originalMessage(topic, data);
    
    // 序列化消息
    QByteArray serializedData = originalMessage.serialize();
    QVERIFY(!serializedData.isEmpty());
    
    // 反序列化消息
    Message deserializedMessage;
    bool success = deserializedMessage.deserialize(serializedData);
    QVERIFY(success);
    
    // 验证反序列化后的消息与原始消息相同
    QCOMPARE(deserializedMessage.id(), originalMessage.id());
    QCOMPARE(deserializedMessage.topic(), originalMessage.topic());
    QCOMPARE(deserializedMessage.data(), originalMessage.data());
    QCOMPARE(deserializedMessage.timestamp(), originalMessage.timestamp());
}

QTEST_MAIN(MessageTest)
#include "message_test.moc"
