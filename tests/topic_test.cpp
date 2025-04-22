#include <QtTest>
#include "topic.h"

class TopicTest : public QObject
{
    Q_OBJECT

private slots:
    void testConstructor();
    void testSettersAndGetters();
    void testProperties();
    void testValidity();
    void testEquality();
};

void TopicTest::testConstructor()
{
    // 测试默认构造函数
    Topic topic1;
    QVERIFY(topic1.name().isEmpty());
    QVERIFY(topic1.dataType().isEmpty());
    QVERIFY(!topic1.isValid());
    
    // 测试带名称的构造函数
    QString name = "test/topic";
    Topic topic2(name);
    QCOMPARE(topic2.name(), name);
    QVERIFY(topic2.dataType().isEmpty());
    QVERIFY(topic2.isValid());
    
    // 测试带名称和数据类型的构造函数
    QString dataType = "string";
    Topic topic3(name, dataType);
    QCOMPARE(topic3.name(), name);
    QCOMPARE(topic3.dataType(), dataType);
    QVERIFY(topic3.isValid());
}

void TopicTest::testSettersAndGetters()
{
    Topic topic;
    
    // 测试name的setter和getter
    QString name = "test/topic";
    topic.setName(name);
    QCOMPARE(topic.name(), name);
    
    // 测试dataType的setter和getter
    QString dataType = "string";
    topic.setDataType(dataType);
    QCOMPARE(topic.dataType(), dataType);
}

void TopicTest::testProperties()
{
    Topic topic("test/topic");
    
    // 测试设置和获取属性
    QString key = "key1";
    QVariant value = "value1";
    topic.setProperty(key, value);
    QCOMPARE(topic.property(key), value);
    
    // 测试获取不存在的属性
    QString nonExistentKey = "nonExistentKey";
    QVariant defaultValue = "default";
    QCOMPARE(topic.property(nonExistentKey, defaultValue), defaultValue);
}

void TopicTest::testValidity()
{
    // 测试空主题的有效性
    Topic emptyTopic;
    QVERIFY(!emptyTopic.isValid());
    
    // 测试有名称的主题的有效性
    Topic validTopic("test/topic");
    QVERIFY(validTopic.isValid());
}

void TopicTest::testEquality()
{
    // 创建两个相同名称的主题
    QString name = "test/topic";
    Topic topic1(name);
    Topic topic2(name);
    
    // 测试相等性
    QVERIFY(topic1 == topic2);
    QVERIFY(!(topic1 != topic2));
    
    // 创建一个不同名称的主题
    Topic topic3("another/topic");
    
    // 测试不等性
    QVERIFY(topic1 != topic3);
    QVERIFY(!(topic1 == topic3));
}

QTEST_MAIN(TopicTest)
#include "topic_test.moc"
