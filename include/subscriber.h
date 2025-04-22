#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include <QObject>
#include <QTcpSocket>
#include <QLocalSocket>
#include <QSet>
#include <QMap>
#include <QTimer>

#include "message.h"
#include "topic.h"

/**
 * @brief Subscriber类，用于订阅和接收消息
 */
class Subscriber : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit Subscriber(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~Subscriber();

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
     * @brief 订阅主题
     * @param topic 主题
     * @return 是否订阅成功
     */
    bool subscribe(const QString& topic);

    /**
     * @brief 取消订阅主题
     * @param topic 主题
     * @return 是否取消订阅成功
     */
    bool unsubscribe(const QString& topic);

    /**
     * @brief 获取已订阅的主题
     * @return 已订阅的主题集合
     */
    QSet<QString> subscribedTopics() const;

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
     * @brief 收到消息信号
     * @param message 消息
     */
    void messageReceived(const Message& message);

    /**
     * @brief 订阅成功信号
     * @param topic 主题
     */
    void subscribed(const QString& topic);

    /**
     * @brief 取消订阅成功信号
     * @param topic 主题
     */
    void unsubscribed(const QString& topic);

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
     * @brief 处理TCP套接字读取就绪
     */
    void handleTcpReadyRead();

    /**
     * @brief 处理本地套接字读取就绪
     */
    void handleLocalReadyRead();

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

private:
    /**
     * @brief 注册为订阅者
     */
    void registerAsSubscriber();

    /**
     * @brief 重新订阅所有主题
     */
    void resubscribeAll();

    /**
     * @brief 发送消息到Broker
     * @param message 消息
     * @return 是否发送成功
     */
    bool sendMessage(const Message& message);

    /**
     * @brief 处理收到的消息
     * @param data 消息数据
     */
    void processReceivedData(const QByteArray& data);

private:
    QTcpSocket* m_tcpSocket;                ///< TCP套接字
    QLocalSocket* m_localSocket;            ///< 本地套接字
    QString m_host;                         ///< 主机地址
    int m_port;                             ///< 端口
    QString m_serverName;                   ///< 服务器名称
    bool m_useLocalSocket;                  ///< 是否使用本地套接字
    QSet<QString> m_subscribedTopics;       ///< 已订阅的主题
    bool m_autoReconnect;                   ///< 是否自动重连
    int m_reconnectInterval;                ///< 重连间隔
    QTimer* m_reconnectTimer;               ///< 重连定时器
    bool m_registered;                      ///< 是否已注册为订阅者
};

#endif // SUBSCRIBER_H
