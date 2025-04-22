#include "message.h"

Message::Message()
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_timestamp(QDateTime::currentDateTime())
{
}

Message::Message(const QString& topic, const QByteArray& data)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_topic(topic)
    , m_data(data)
    , m_timestamp(QDateTime::currentDateTime())
{
}

QString Message::id() const
{
    return m_id;
}

QString Message::topic() const
{
    return m_topic;
}

void Message::setTopic(const QString& topic)
{
    m_topic = topic;
}

QByteArray Message::data() const
{
    return m_data;
}

void Message::setData(const QByteArray& data)
{
    m_data = data;
}

QDateTime Message::timestamp() const
{
    return m_timestamp;
}

QByteArray Message::serialize() const
{
    QByteArray serializedData;
    QDataStream stream(&serializedData, QIODevice::WriteOnly);
    
    // 设置数据流版本，确保跨平台兼容性
    stream.setVersion(QDataStream::Qt_5_15);
    
    // 序列化消息属性
    stream << m_id;
    stream << m_topic;
    stream << m_data;
    stream << m_timestamp;
    
    return serializedData;
}

bool Message::deserialize(const QByteArray& data)
{
    QDataStream stream(data);
    
    // 设置数据流版本，确保跨平台兼容性
    stream.setVersion(QDataStream::Qt_5_15);
    
    // 反序列化消息属性
    stream >> m_id;
    stream >> m_topic;
    stream >> m_data;
    stream >> m_timestamp;
    
    // 检查是否有错误发生
    return stream.status() == QDataStream::Ok;
}
