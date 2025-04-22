#ifndef BROKER_H
#define BROKER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMap>
#include <QSet>
#include <QQueue>
#include <QMutex>
#include <QTimer>
#include <QThread>

#include "message.h"
#include "topic.h"
#include "messageframehandler.h"

/**
 * @brief 客户端连接信息
 */
struct ClientInfo {
    QString id;                 ///< 客户端ID
    QTcpSocket* tcpSocket;      ///< TCP套接字
    QLocalSocket* localSocket;  ///< 本地套接字
    QSet<QString> subscriptions; ///< 订阅的主题
    bool isPublisher;           ///< 是否为发布者
    bool isSubscriber;          ///< 是否为订阅者
    QDateTime lastActiveTime;   ///< 最后活动时间
    MessageFrameHandler* frameHandler; ///< 消息帧处理器
};

/**
 * @brief Broker类，负责管理连接和消息路由
 */
class Broker : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取Broker单例实例
     * @return Broker实例
     */
    static Broker* instance();

    /**
     * @brief 启动Broker
     * @param tcpPort TCP端口
     * @param localServerName 本地服务器名称
     * @return 是否启动成功
     */
    bool start(int tcpPort = 5555, const QString& localServerName = "MyMQLocalServer");

    /**
     * @brief 停止Broker
     */
    void stop();

    /**
     * @brief 是否正在运行
     * @return 是否正在运行
     */
    bool isRunning() const;

    /**
     * @brief 获取连接的客户端数量
     * @return 客户端数量
     */
    int clientCount() const;

    /**
     * @brief 获取主题数量
     * @return 主题数量
     */
    int topicCount() const;

    /**
     * @brief 获取消息缓存大小
     * @return 缓存大小
     */
    int getCacheSize() const;

    /**
     * @brief 设置消息缓存大小
     * @param size 缓存大小
     */
    void setCacheSize(int size);

    /**
     * @brief 清除消息缓存
     */
    void clearCache();

    /**
     * @brief 强制释放所有资源，用于测试
     */
    static void forceCleanup();

signals:
    /**
     * @brief 新客户端连接信号
     * @param clientId 客户端ID
     */
    void clientConnected(const QString& clientId);

    /**
     * @brief 客户端断开连接信号
     * @param clientId 客户端ID
     */
    void clientDisconnected(const QString& clientId);

    /**
     * @brief 收到新消息信号
     * @param message 消息
     */
    void messageReceived(const Message& message);

    /**
     * @brief 消息发布信号
     * @param message 消息
     */
    void messagePublished(const Message& message);

private slots:
    /**
     * @brief 处理新的TCP连接
     */
    void handleNewTcpConnection();

    /**
     * @brief 处理新的本地连接
     */
    void handleNewLocalConnection();

    /**
     * @brief 处理TCP套接字读取就绪
     */
    void handleTcpReadyRead();

    /**
     * @brief 处理本地套接字读取就绪
     */
    void handleLocalReadyRead();

    /**
     * @brief 处理TCP套接字断开连接
     */
    void handleTcpDisconnected();

    /**
     * @brief 处理本地套接字断开连接
     */
    void handleLocalDisconnected();

    /**
     * @brief 检查客户端活动状态
     */
    void checkClientActivity();

private:
    /**
     * @brief 构造函数（私有）
     */
    explicit Broker(QObject* parent = nullptr);

    /**
     * @brief 析构函数（私有）
     */
    ~Broker();

    /**
     * @brief 处理收到的消息
     * @param clientId 客户端ID
     * @param message 消息
     */
    void processMessage(const QString& clientId, const Message& message);

    /**
     * @brief 发布消息到订阅者
     * @param message 消息
     */
    void publishMessage(const Message& message);

    /**
     * @brief 缓存消息
     * @param message 消息
     */
    void cacheMessage(const Message& message);

    /**
     * @brief 发送消息到客户端
     * @param clientId 客户端ID
     * @param message 消息
     * @return 是否发送成功
     */
    bool sendMessageToClient(const QString& clientId, const Message& message);

    /**
     * @brief 注册客户端
     * @param socket 套接字
     * @param isLocal 是否为本地套接字
     * @return 客户端ID
     */
    QString registerClient(QObject* socket, bool isLocal);

    /**
     * @brief 注销客户端
     * @param clientId 客户端ID
     */
    void unregisterClient(const QString& clientId);

    /**
     * @brief 处理订阅请求
     * @param clientId 客户端ID
     * @param topic 主题
     */
    void handleSubscription(const QString& clientId, const QString& topic);

    /**
     * @brief 处理取消订阅请求
     * @param clientId 客户端ID
     * @param topic 主题
     */
    void handleUnsubscription(const QString& clientId, const QString& topic);

private:
    static Broker* m_instance;                      ///< 单例实例
    QTcpServer* m_tcpServer;                        ///< TCP服务器
    QLocalServer* m_localServer;                    ///< 本地服务器
    QMap<QString, ClientInfo> m_clients;            ///< 客户端信息映射
    QMap<QString, QSet<QString>> m_topicSubscribers; ///< 主题订阅者映射
    QMap<QString, QQueue<Message>> m_messageCache;  ///< 消息缓存
    QMutex* m_clientsMutex;                          ///< 客户端互斥锁
    QMutex* m_cacheMutex;                            ///< 缓存互斥锁
    QTimer* m_activityTimer;                        ///< 活动检查定时器
    int m_cacheSize;                                ///< 缓存大小
    bool m_running;                                 ///< 是否正在运行
};

#endif // BROKER_H
