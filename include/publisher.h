#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <QObject>
#include <QTcpSocket>
#include <QLocalSocket>
#include <QQueue>
#include <QMutex>
#include <QTimer>

#include "message.h"
#include "topic.h"
#include "messageframehandler.h"

/**
 * @brief Publisher类，用于发布消息
 */
class Publisher : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit Publisher(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Publisher();

    /**
     * @brief 连接到Broker
     * @param host 主机地址
     * @param port 端口
     * @return 是否连接成功
     */
    bool connectToBroker(const QString& host, int port);

    /**
     * @brief 连接到本地Broker
     * @param serverName 服务器名称
     * @return 是否连接成功
     */
    bool connectToLocalBroker(const QString& serverName);

    /**
     * @brief 断开与Broker的连接
     */
    void disconnectFromBroker();

    /**
     * @brief 是否已连接到Broker
     * @return 是否已连接
     */
    bool isConnected() const;

    /**
     * @brief 发布消息
     * @param topic 主题
     * @param data 数据
     * @return 是否发布成功
     */
    bool publish(const QString& topic, const QByteArray& data);

    /**
     * @brief 发布消息
     * @param message 消息
     * @return 是否发布成功
     */
    bool publish(const Message& message);

    /**
     * @brief 设置自动重连
     * @param enable 是否启用
     * @param interval 重连间隔（毫秒）
     */
    void setAutoReconnect(bool enable, int interval = 5000);

signals:
    /**
     * @brief 连接成功信号
     */
    void connected();

    /**
     * @brief 断开连接信号
     */
    void disconnected();

    /**
     * @brief 发布成功信号
     * @param messageId 消息ID
     */
    void published(const QString& messageId);

    /**
     * @brief 错误信号
     * @param errorMessage 错误消息
     */
    void error(const QString& errorMessage);

private slots:
    /**
     * @brief 处理连接成功
     */
    void handleConnected();

    /**
     * @brief 处理断开连接
     */
    void handleDisconnected();

    /**
     * @brief 处理错误
     * @param socketError 套接字错误
     */
    void handleError(QAbstractSocket::SocketError socketError);

    /**
     * @brief 处理本地套接字错误
     * @param socketError 套接字错误
     */
    void handleLocalError(QLocalSocket::LocalSocketError socketError);

    /**
     * @brief 尝试重连
     */
    void tryReconnect();

    /**
     * @brief 处理待发送消息
     */
    void processPendingMessages();

private:
    /**
     * @brief 注册为发布者
     */
    void registerAsPublisher();

    /**
     * @brief 发送消息到Broker
     * @param message 消息
     * @return 是否发送成功
     */
    bool sendMessage(const Message& message);

private:
    QTcpSocket* m_tcpSocket;                ///< TCP套接字
    QLocalSocket* m_localSocket;            ///< 本地套接字
    QString m_host;                         ///< 主机地址
    int m_port;                             ///< 端口
    QString m_serverName;                   ///< 服务器名称
    bool m_useLocalSocket;                  ///< 是否使用本地套接字
    bool m_autoReconnect;                   ///< 是否自动重连
    int m_reconnectInterval;                ///< 重连间隔
    QTimer* m_reconnectTimer;               ///< 重连定时器
    QQueue<Message> m_pendingMessages;      ///< 待发送消息队列
    QMutex* m_pendingMessagesMutex;          ///< 待发送消息互斥锁
    bool m_registered;                      ///< 是否已注册为发布者
    MessageFrameHandler* m_frameHandler;     ///< 消息帧处理器
};

#endif // PUBLISHER_H
