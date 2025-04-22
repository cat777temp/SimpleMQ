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
    // 先序列化消息内容
    QByteArray messageContent;
    QDataStream contentStream(&messageContent, QIODevice::WriteOnly);

    // 设置数据流版本，确保跨平台兼容性
    contentStream.setVersion(QDataStream::Qt_5_15);

    // 序列化消息属性
    contentStream << m_id;
    contentStream << m_topic;
    contentStream << m_data;
    contentStream << m_timestamp;

    // 创建包含消息长度前缀的完整消息
    QByteArray completeMessage;
    QDataStream frameStream(&completeMessage, QIODevice::WriteOnly);
    frameStream.setVersion(QDataStream::Qt_5_15);

    // 写入消息长度和消息内容
    frameStream << (qint32)messageContent.size();
    completeMessage.append(messageContent);

    return completeMessage;
}

bool Message::deserialize(const QByteArray& data)
{
    // 注意：这个方法现在期望收到的是消息内容部分，不包含长度前缀
    // 长度前缀的处理已经移到 MessageFrameHandler 类中

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

// 从带有长度前缀的完整消息中提取消息内容
// 返回值：如果成功，返回消息内容；如果失败，返回空字节数组
// 参数 bytesRead：输出参数，表示从输入数据中读取的字节数
// 如果数据不完整，返回空字节数组，并设置 bytesRead = 0
QByteArray Message::extractMessageContent(const QByteArray& frameData, int& bytesRead)
{
    bytesRead = 0;

    // 检查数据长度是否足够包含消息长度前缀（4字节）
    if (frameData.size() < (int)sizeof(qint32)) {
        return QByteArray();
    }

    // 读取消息长度
    QDataStream stream(frameData);
    stream.setVersion(QDataStream::Qt_5_15);

    qint32 messageSize;
    stream >> messageSize;

    // 检查数据长度是否足够包含完整消息
    if (frameData.size() < (int)sizeof(qint32) + messageSize) {
        return QByteArray();
    }

    // 提取消息内容
    QByteArray messageContent = frameData.mid(sizeof(qint32), messageSize);
    bytesRead = sizeof(qint32) + messageSize;

    return messageContent;
}
