#ifndef MESSAGEFRAMEHANDLER_H
#define MESSAGEFRAMEHANDLER_H

#include <QObject>
#include <QByteArray>
#include "message.h"

/**
 * @brief 消息帧处理器类，用于处理消息的分包和粘包问题
 */
class MessageFrameHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit MessageFrameHandler(QObject* parent = nullptr);

    /**
     * @brief 处理接收到的数据
     * @param data 接收到的数据
     */
    void processIncomingData(const QByteArray& data);

    /**
     * @brief 清除接收缓冲区
     */
    void clearBuffer();

signals:
    /**
     * @brief 收到完整消息的信号
     * @param message 收到的消息
     */
    void messageReceived(const Message& message);

    /**
     * @brief 处理数据时发生错误的信号
     * @param errorMessage 错误信息
     */
    void error(const QString& errorMessage);

private:
    QByteArray m_buffer;  ///< 接收缓冲区
};

#endif // MESSAGEFRAMEHANDLER_H
