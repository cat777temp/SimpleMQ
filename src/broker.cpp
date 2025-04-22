#include "broker.h"
#include "logger.h"

// 初始化静态成员变量
Broker* Broker::m_instance = nullptr;

Broker* Broker::instance()
{
    if (!m_instance) {
        m_instance = new Broker();
    }
    return m_instance;
}

Broker::Broker(QObject* parent)
    : QObject(parent)
    , m_tcpServer(new QTcpServer(this))
    , m_localServer(new QLocalServer(this))
    , m_clientsMutex(new QMutex())
    , m_cacheMutex(new QMutex())
    , m_activityTimer(new QTimer(this))
    , m_cacheSize(100)
    , m_running(false)
{
    // 注册元类型，使其可以在信号槽中使用
    qRegisterMetaType<Message>("Message");
    qRegisterMetaType<Topic>("Topic");

    // 连接信号槽
    connect(m_tcpServer, &QTcpServer::newConnection, this, &Broker::handleNewTcpConnection);
    connect(m_localServer, &QLocalServer::newConnection, this, &Broker::handleNewLocalConnection);

    // 设置活动检查定时器
    connect(m_activityTimer, &QTimer::timeout, this, &Broker::checkClientActivity);
    m_activityTimer->setInterval(30000); // 30秒检查一次
}

Broker::~Broker()
{
    stop();

    delete m_clientsMutex;
    delete m_cacheMutex;
}

bool Broker::start(int tcpPort, const QString& localServerName)
{
    Logger::instance()->info("Starting broker...");

    // 如果已经在运行，先停止
    if (m_running) {
        stop();
    }

    // 启动TCP服务器
    if (!m_tcpServer->listen(QHostAddress::Any, tcpPort)) {
        Logger::instance()->error(QString("Failed to start TCP server: %1").arg(m_tcpServer->errorString()));
        return false;
    }

    // 如果本地服务器已存在，先移除
    QLocalServer::removeServer(localServerName);

    // 启动本地服务器
    if (!m_localServer->listen(localServerName)) {
        Logger::instance()->error(QString("Failed to start local server: %1").arg(m_localServer->errorString()));
        m_tcpServer->close();
        return false;
    }

    // 启动活动检查定时器
    m_activityTimer->start();

    m_running = true;
    Logger::instance()->info(QString("Broker started. TCP port: %1, Local server: %2").arg(tcpPort).arg(localServerName));

    return true;
}

void Broker::stop()
{
    if (!m_running) {
        return;
    }

    Logger::instance()->info("Stopping broker...");

    // 停止活动检查定时器
    m_activityTimer->stop();

    // 关闭所有客户端连接
    QMutexLocker locker(m_clientsMutex);
    QList<QString> clientIds = m_clients.keys();
    locker.unlock();

    for (const QString& clientId : clientIds) {
        unregisterClient(clientId);
    }

    // 关闭服务器
    m_tcpServer->close();
    m_localServer->close();

    // 清除缓存
    clearCache();

    m_running = false;
    Logger::instance()->info("Broker stopped");
}

bool Broker::isRunning() const
{
    return m_running;
}

int Broker::clientCount() const
{
    QMutexLocker locker(m_clientsMutex);
    return m_clients.size();
}

int Broker::topicCount() const
{
    QMutexLocker locker(m_clientsMutex);
    return m_topicSubscribers.size();
}

int Broker::getCacheSize() const
{
    return m_cacheSize;
}

void Broker::setCacheSize(int size)
{
    if (size < 0) {
        return;
    }

    m_cacheSize = size;

    // 调整现有缓存大小
    QMutexLocker locker(m_cacheMutex);
    for (auto it = m_messageCache.begin(); it != m_messageCache.end(); ++it) {
        while (it.value().size() > m_cacheSize) {
            it.value().dequeue();
        }
    }
}

void Broker::clearCache()
{
    QMutexLocker locker(m_cacheMutex);
    m_messageCache.clear();
}

void Broker::forceCleanup()
{
    if (m_instance) {
        // 停止 Broker
        m_instance->stop();

        // 等待一段时间确保资源释放
        QThread::msleep(200);

        // 删除实例
        delete m_instance;
        m_instance = nullptr;
    }
}

void Broker::handleNewTcpConnection()
{
    QTcpSocket* socket = m_tcpServer->nextPendingConnection();
    if (!socket) {
        return;
    }

    QString clientId = registerClient(socket, false);

    // 连接信号槽
    connect(socket, &QTcpSocket::readyRead, this, &Broker::handleTcpReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Broker::handleTcpDisconnected);

    Logger::instance()->info(QString("New TCP client connected: %1").arg(clientId));
    emit clientConnected(clientId);
}

void Broker::handleNewLocalConnection()
{
    QLocalSocket* socket = m_localServer->nextPendingConnection();
    if (!socket) {
        return;
    }

    QString clientId = registerClient(socket, true);

    // 连接信号槽
    connect(socket, &QLocalSocket::readyRead, this, &Broker::handleLocalReadyRead);
    connect(socket, &QLocalSocket::disconnected, this, &Broker::handleLocalDisconnected);

    Logger::instance()->info(QString("New local client connected: %1").arg(clientId));
    emit clientConnected(clientId);
}

void Broker::handleTcpReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    // 查找客户端ID和帧处理器
    QString clientId;
    MessageFrameHandler* frameHandler = nullptr;
    {
        QMutexLocker locker(m_clientsMutex);
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
            if (it.value().tcpSocket == socket) {
                clientId = it.key();
                frameHandler = it.value().frameHandler;
                break;
            }
        }
    }

    if (clientId.isEmpty() || !frameHandler) {
        Logger::instance()->warning("Received data from unknown TCP client");
        return;
    }

    // 更新最后活动时间
    {
        QMutexLocker locker(m_clientsMutex);
        m_clients[clientId].lastActiveTime = QDateTime::currentDateTime();
    }

    // 读取数据
    while (socket->bytesAvailable() > 0) {
        QByteArray data = socket->readAll();

        // 使用消息帧处理器处理数据
        // 当收到完整消息时，帧处理器会发出 messageReceived 信号
        // 该信号已在 registerClient 方法中连接到 processMessage 方法
        frameHandler->processIncomingData(data);
    }
}

void Broker::handleLocalReadyRead()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) {
        return;
    }

    // 查找客户端ID和帧处理器
    QString clientId;
    MessageFrameHandler* frameHandler = nullptr;
    {
        QMutexLocker locker(m_clientsMutex);
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
            if (it.value().localSocket == socket) {
                clientId = it.key();
                frameHandler = it.value().frameHandler;
                break;
            }
        }
    }

    if (clientId.isEmpty() || !frameHandler) {
        Logger::instance()->warning("Received data from unknown local client");
        return;
    }

    // 更新最后活动时间
    {
        QMutexLocker locker(m_clientsMutex);
        m_clients[clientId].lastActiveTime = QDateTime::currentDateTime();
    }

    // 读取数据
    while (socket->bytesAvailable() > 0) {
        QByteArray data = socket->readAll();

        // 使用消息帧处理器处理数据
        // 当收到完整消息时，帧处理器会发出 messageReceived 信号
        // 该信号已在 registerClient 方法中连接到 processMessage 方法
        frameHandler->processIncomingData(data);
    }
}

void Broker::handleTcpDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    // 查找客户端ID
    QString clientId;
    {
        QMutexLocker locker(m_clientsMutex);
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
            if (it.value().tcpSocket == socket) {
                clientId = it.key();
                break;
            }
        }
    }

    if (!clientId.isEmpty()) {
        unregisterClient(clientId);
        Logger::instance()->info(QString("TCP client disconnected: %1").arg(clientId));
        emit clientDisconnected(clientId);
    }
}

void Broker::handleLocalDisconnected()
{
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) {
        return;
    }

    // 查找客户端ID
    QString clientId;
    {
        QMutexLocker locker(m_clientsMutex);
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
            if (it.value().localSocket == socket) {
                clientId = it.key();
                break;
            }
        }
    }

    if (!clientId.isEmpty()) {
        unregisterClient(clientId);
        Logger::instance()->info(QString("Local client disconnected: %1").arg(clientId));
        emit clientDisconnected(clientId);
    }
}

void Broker::checkClientActivity()
{
    QDateTime now = QDateTime::currentDateTime();
    QList<QString> inactiveClients;

    // 查找不活跃的客户端
    {
        QMutexLocker locker(m_clientsMutex);
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
            if (it.value().lastActiveTime.secsTo(now) > 60) { // 60秒不活跃
                inactiveClients.append(it.key());
            }
        }
    }

    // 注销不活跃的客户端
    for (const QString& clientId : inactiveClients) {
        Logger::instance()->info(QString("Client inactive, disconnecting: %1").arg(clientId));
        unregisterClient(clientId);
        emit clientDisconnected(clientId);
    }
}

void Broker::processMessage(const QString& clientId, const Message& message)
{
    Logger::instance()->debug(QString("Processing message from client %1, topic: %2").arg(clientId).arg(message.topic()));

    // 特殊主题处理
    if (message.topic() == "$SYS/SUBSCRIBE") {
        // 订阅请求
        QString topicToSubscribe = QString::fromUtf8(message.data());
        handleSubscription(clientId, topicToSubscribe);
        return;
    } else if (message.topic() == "$SYS/UNSUBSCRIBE") {
        // 取消订阅请求
        QString topicToUnsubscribe = QString::fromUtf8(message.data());
        handleUnsubscription(clientId, topicToUnsubscribe);
        return;
    } else if (message.topic() == "$SYS/REGISTER") {
        // 注册为发布者或订阅者
        QString role = QString::fromUtf8(message.data());
        QMutexLocker locker(m_clientsMutex);
        if (m_clients.contains(clientId)) {
            if (role == "PUBLISHER") {
                m_clients[clientId].isPublisher = true;
                Logger::instance()->info(QString("Client %1 registered as publisher").arg(clientId));
            } else if (role == "SUBSCRIBER") {
                m_clients[clientId].isSubscriber = true;
                Logger::instance()->info(QString("Client %1 registered as subscriber").arg(clientId));
            }
        }
        return;
    }

    // 检查客户端是否为发布者
    bool isPublisher = false;
    {
        QMutexLocker locker(m_clientsMutex);
        if (m_clients.contains(clientId)) {
            isPublisher = m_clients[clientId].isPublisher;
        }
    }

    if (!isPublisher) {
        Logger::instance()->warning(QString("Client %1 is not registered as publisher").arg(clientId));
        return;
    }

    // 缓存消息
    {
        QMutexLocker locker(m_cacheMutex);
        // 如果缓存大小为0，不缓存消息
        if (m_cacheSize > 0) {
            // 添加消息到缓存
            m_messageCache[message.topic()].enqueue(message);

            // 如果缓存超过大小限制，移除最旧的消息
            while (m_messageCache[message.topic()].size() > m_cacheSize) {
                m_messageCache[message.topic()].dequeue();
            }
        }
    }

    // 获取订阅该主题的客户端
    QSet<QString> subscribers;
    QMap<QString, ClientInfo> clientsCopy;
    {
        QMutexLocker locker(m_clientsMutex);
        subscribers = m_topicSubscribers.value(message.topic());

        // 复制需要的客户端信息
        for (const QString& subId : subscribers) {
            if (m_clients.contains(subId)) {
                clientsCopy[subId] = m_clients[subId];
            }
        }
    }

    // 发送消息给订阅者，不需要再次获取锁
    for (const QString& subscriberId : subscribers) {
        if (clientsCopy.contains(subscriberId)) {
            const ClientInfo& clientInfo = clientsCopy[subscriberId];

            // 检查客户端是否为订阅者
            if (!clientInfo.isSubscriber) {
                continue;
            }

            // 序列化消息
            QByteArray data = message.serialize();

            // 发送消息
            bool sent = false;
            if (clientInfo.tcpSocket) {
                sent = clientInfo.tcpSocket->write(data) == data.size();
            } else if (clientInfo.localSocket) {
                sent = clientInfo.localSocket->write(data) == data.size();
            }

            if (sent) {
                Logger::instance()->debug(QString("Sent message to client %1: %2").arg(subscriberId).arg(message.topic()));
            }
        }
    }

    emit messageReceived(message);
    emit messagePublished(message);
}

void Broker::publishMessage(const Message& message)
{
    // 注意：这个方法已经被弃用，所有的消息发布都应该通过 processMessage 方法处理
    Logger::instance()->debug(QString("publishMessage is deprecated, use processMessage instead: %1").arg(message.topic()));

    // 为了兼容性，我们仍然尝试发布消息，但使用更安全的方式

    // 获取订阅该主题的客户端和客户端信息
    QSet<QString> subscribers;
    QMap<QString, ClientInfo> clientsCopy;
    {
        QMutexLocker locker(m_clientsMutex);
        subscribers = m_topicSubscribers.value(message.topic());

        // 复制需要的客户端信息
        for (const QString& subId : subscribers) {
            if (m_clients.contains(subId)) {
                clientsCopy[subId] = m_clients[subId];
            }
        }
    }

    // 发送消息给订阅者，不需要再次获取锁
    for (const QString& subscriberId : subscribers) {
        if (clientsCopy.contains(subscriberId)) {
            const ClientInfo& clientInfo = clientsCopy[subscriberId];

            // 检查客户端是否为订阅者
            if (!clientInfo.isSubscriber) {
                continue;
            }

            // 序列化消息
            QByteArray data = message.serialize();

            // 发送消息
            bool sent = false;
            if (clientInfo.tcpSocket) {
                sent = clientInfo.tcpSocket->write(data) == data.size();
            } else if (clientInfo.localSocket) {
                sent = clientInfo.localSocket->write(data) == data.size();
            }

            if (sent) {
                Logger::instance()->debug(QString("Sent message to client %1: %2").arg(subscriberId).arg(message.topic()));
            }
        }
    }

    emit messagePublished(message);
}

void Broker::cacheMessage(const Message& message)
{
    // 注意：这个方法已经被弃用，所有的消息缓存都应该通过 processMessage 方法处理
    Logger::instance()->debug(QString("cacheMessage is deprecated, use processMessage instead: %1").arg(message.topic()));

    // 为了兼容性，我们仍然尝试缓存消息
    QMutexLocker locker(m_cacheMutex);

    // 如果缓存大小为0，不缓存消息
    if (m_cacheSize <= 0) {
        return;
    }

    // 添加消息到缓存
    m_messageCache[message.topic()].enqueue(message);

    // 如果缓存超过大小限制，移除最旧的消息
    while (m_messageCache[message.topic()].size() > m_cacheSize) {
        m_messageCache[message.topic()].dequeue();
    }
}

bool Broker::sendMessageToClient(const QString& clientId, const Message& message)
{
    // 注意：这个方法已经被弃用，所有的消息发送都应该通过 processMessage 或 publishMessage 方法处理
    Logger::instance()->debug(QString("sendMessageToClient is deprecated, use processMessage instead: %1").arg(clientId));

    // 为了兼容性，我们仍然尝试发送消息，但使用更安全的方式

    // 获取客户端信息
    ClientInfo clientInfo;
    bool clientExists = false;
    bool isSubscriber = false;

    {
        QMutexLocker locker(m_clientsMutex);
        if (m_clients.contains(clientId)) {
            clientExists = true;
            clientInfo = m_clients[clientId];
            isSubscriber = clientInfo.isSubscriber;
        }
    }

    if (!clientExists) {
        return false;
    }

    // 检查客户端是否为订阅者
    if (!isSubscriber) {
        return false;
    }

    // 序列化消息
    QByteArray data = message.serialize();

    // 发送消息
    if (clientInfo.tcpSocket) {
        return clientInfo.tcpSocket->write(data) == data.size();
    } else if (clientInfo.localSocket) {
        return clientInfo.localSocket->write(data) == data.size();
    }

    return false;
}

QString Broker::registerClient(QObject* socket, bool isLocal)
{
    // 生成客户端ID
    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // 创建客户端信息
    ClientInfo clientInfo;
    clientInfo.id = clientId;
    clientInfo.tcpSocket = isLocal ? nullptr : qobject_cast<QTcpSocket*>(socket);
    clientInfo.localSocket = isLocal ? qobject_cast<QLocalSocket*>(socket) : nullptr;
    clientInfo.isPublisher = false;
    clientInfo.isSubscriber = false;
    clientInfo.lastActiveTime = QDateTime::currentDateTime();

    // 创建消息帧处理器
    clientInfo.frameHandler = new MessageFrameHandler(this);

    // 连接消息帧处理器的信号
    connect(clientInfo.frameHandler, &MessageFrameHandler::messageReceived,
            [this, clientId](const Message& message) {
                // 处理收到的消息
                processMessage(clientId, message);
            });

    connect(clientInfo.frameHandler, &MessageFrameHandler::error,
            [this, clientId](const QString& errorMessage) {
                // 记录错误
                Logger::instance()->warning(QString("Client %1: %2").arg(clientId).arg(errorMessage));
            });

    // 添加客户端信息
    QMutexLocker locker(m_clientsMutex);
    m_clients[clientId] = clientInfo;

    return clientId;
}

void Broker::unregisterClient(const QString& clientId)
{
    QMutexLocker locker(m_clientsMutex);

    if (!m_clients.contains(clientId)) {
        return;
    }

    // 获取客户端信息
    ClientInfo clientInfo = m_clients[clientId];

    // 从所有主题的订阅者列表中移除
    for (auto it = m_topicSubscribers.begin(); it != m_topicSubscribers.end(); ++it) {
        it.value().remove(clientId);
    }

    // 断开连接
    if (clientInfo.tcpSocket) {
        clientInfo.tcpSocket->disconnect();
        clientInfo.tcpSocket->deleteLater();
    }

    if (clientInfo.localSocket) {
        clientInfo.localSocket->disconnect();
        clientInfo.localSocket->deleteLater();
    }

    // 释放消息帧处理器
    if (clientInfo.frameHandler) {
        clientInfo.frameHandler->disconnect();
        clientInfo.frameHandler->deleteLater();
    }

    // 移除客户端信息
    m_clients.remove(clientId);
}

void Broker::handleSubscription(const QString& clientId, const QString& topic)
{
    Logger::instance()->info(QString("Client %1 subscribing to topic: %2").arg(clientId).arg(topic));

    // 首先检查客户端是否存在并获取客户端信息
    bool clientExists = false;
    ClientInfo clientInfo;
    {
        QMutexLocker locker(m_clientsMutex);
        if (m_clients.contains(clientId)) {
            clientExists = true;
            clientInfo = m_clients[clientId];

            // 添加到客户端的订阅列表
            m_clients[clientId].subscriptions.insert(topic);

            // 添加到主题的订阅者列表
            m_topicSubscribers[topic].insert(clientId);

            // 标记为订阅者
            m_clients[clientId].isSubscriber = true;
        }
    }

    if (!clientExists) {
        return;
    }

    // 获取缓存的消息
    QQueue<Message> cachedMessages;
    {
        QMutexLocker cacheLocker(m_cacheMutex);
        if (m_messageCache.contains(topic)) {
            cachedMessages = m_messageCache[topic];
        }
    }

    // 发送缓存的消息
    for (const Message& message : cachedMessages) {
        // 使用客户端信息直接发送消息，避免再次获取锁
        QByteArray data = message.serialize();
        bool sent = false;

        if (clientInfo.tcpSocket) {
            sent = clientInfo.tcpSocket->write(data) == data.size();
        } else if (clientInfo.localSocket) {
            sent = clientInfo.localSocket->write(data) == data.size();
        }

        if (sent) {
            Logger::instance()->debug(QString("Sent cached message to client %1: %2").arg(clientId).arg(message.topic()));
        }
    }
}

void Broker::handleUnsubscription(const QString& clientId, const QString& topic)
{
    Logger::instance()->info(QString("Client %1 unsubscribing from topic: %2").arg(clientId).arg(topic));

    QMutexLocker locker(m_clientsMutex);

    if (!m_clients.contains(clientId)) {
        return;
    }

    // 从客户端的订阅列表中移除
    m_clients[clientId].subscriptions.remove(topic);

    // 从主题的订阅者列表中移除
    if (m_topicSubscribers.contains(topic)) {
        m_topicSubscribers[topic].remove(clientId);

        // 如果没有订阅者，移除主题
        if (m_topicSubscribers[topic].isEmpty()) {
            m_topicSubscribers.remove(topic);
        }
    }
}

