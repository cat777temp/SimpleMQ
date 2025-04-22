#include "messageframehandler.h"
#include "logger.h"

MessageFrameHandler::MessageFrameHandler(QObject* parent)
    : QObject(parent)
{
}

void MessageFrameHandler::processIncomingData(const QByteArray& data)
{
    // 将接收到的数据添加到缓冲区
    m_buffer.append(data);

    // 循环处理缓冲区中的所有完整消息
    while (!m_buffer.isEmpty()) {
        // 尝试从缓冲区中提取一个完整的消息
        int bytesRead = 0;
        QByteArray messageContent = Message::extractMessageContent(m_buffer, bytesRead);

        // 如果没有足够的数据形成一个完整的消息，退出循环
        if (bytesRead == 0) {
            break;
        }

        // 从缓冲区中移除已处理的数据
        m_buffer.remove(0, bytesRead);

        // 反序列化消息
        Message message;
        if (message.deserialize(messageContent)) {
            // 发出消息接收信号
            emit messageReceived(message);
        } else {
            // 发出错误信号
            emit error("Failed to deserialize message");
            Logger::instance()->warning("Failed to deserialize message");
        }
    }
}

void MessageFrameHandler::clearBuffer()
{
    m_buffer.clear();
}
