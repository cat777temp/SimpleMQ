#include "publisher.h"
#include "logger.h"

Publisher::Publisher(QObject* parent)
    : QObject(parent)
    , m_tcpSocket(nullptr)
    , m_localSocket(nullptr)
    , m_port(0)
    , m_useLocalSocket(false)
    , m_autoReconnect(false)
    , m_reconnectInterval(5000)
    , m_reconnectTimer(new QTimer(this))
    , m_pendingMessagesMutex(new QMutex())
    , m_registered(false)
    , m_frameHandler(new MessageFrameHandler(this))
{
    // 连接重连定时器信号
    connect(m_reconnectTimer, &QTimer::timeout, this, &Publisher::tryReconnect);

    // 连接消息帧处理器的信号
    connect(m_frameHandler, &MessageFrameHandler::error,
            [this](const QString& errorMessage) {
                Logger::instance()->warning(QString("Frame handler error: %1").arg(errorMessage));
                emit error(errorMessage);
            });
}

Publisher::~Publisher()
{
    disconnectFromBroker();
    delete m_pendingMessagesMutex;
}

bool Publisher::connectToBroker(const QString& host, int port)
{
    // 如果已连接，先断开
    if (isConnected()) {
        disconnectFromBroker();
    }

    m_host = host;
    m_port = port;
    m_useLocalSocket = false;

    // 创建TCP套接字
    m_tcpSocket = new QTcpSocket(this);

    // 连接信号槽
    connect(m_tcpSocket, &QTcpSocket::connected, this, &Publisher::handleConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &Publisher::handleDisconnected);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred,
            this, &Publisher::handleError);

    // 连接到服务器
    m_tcpSocket->connectToHost(host, port);

    // 等待连接成功 - 使用更短的超时时间
    if (!m_tcpSocket->waitForConnected(1000)) {
        Logger::instance()->error(QString("Failed to connect to broker: %1").arg(m_tcpSocket->errorString()));
        emit error(QString("Failed to connect to broker: %1").arg(m_tcpSocket->errorString()));

        // 如果启用了自动重连，启动重连定时器
        if (m_autoReconnect) {
            m_reconnectTimer->start(m_reconnectInterval);
        }

        return false;
    }

    return true;
}

bool Publisher::connectToLocalBroker(const QString& serverName)
{
    // 如果已连接，先断开
    if (isConnected()) {
        disconnectFromBroker();
    }

    m_serverName = serverName;
    m_useLocalSocket = true;

    // 创建本地套接字
    m_localSocket = new QLocalSocket(this);

    // 连接信号槽
    connect(m_localSocket, &QLocalSocket::connected, this, &Publisher::handleConnected);
    connect(m_localSocket, &QLocalSocket::disconnected, this, &Publisher::handleDisconnected);
    connect(m_localSocket, &QLocalSocket::errorOccurred,
            this, &Publisher::handleLocalError);

    // 连接到服务器
    m_localSocket->connectToServer(serverName);

    // 等待连接成功 - 使用更短的超时时间
    if (!m_localSocket->waitForConnected(1000)) {
        Logger::instance()->error(QString("Failed to connect to local broker: %1").arg(m_localSocket->errorString()));
        emit error(QString("Failed to connect to local broker: %1").arg(m_localSocket->errorString()));

        // 如果启用了自动重连，启动重连定时器
        if (m_autoReconnect) {
            m_reconnectTimer->start(m_reconnectInterval);
        }

        return false;
    }

    return true;
}

void Publisher::disconnectFromBroker()
{
    // 停止重连定时器
    m_reconnectTimer->stop();

    // 断开TCP连接
    if (m_tcpSocket) {
        m_tcpSocket->disconnect();
        m_tcpSocket->close();
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
    }

    // 断开本地连接
    if (m_localSocket) {
        m_localSocket->disconnect();
        m_localSocket->close();
        m_localSocket->deleteLater();
        m_localSocket = nullptr;
    }

    // 清除消息帧处理器的缓冲区
    if (m_frameHandler) {
        m_frameHandler->clearBuffer();
    }

    m_registered = false;
}

bool Publisher::isConnected() const
{
    if (m_useLocalSocket) {
        return m_localSocket && m_localSocket->state() == QLocalSocket::ConnectedState;
    } else {
        return m_tcpSocket && m_tcpSocket->state() == QTcpSocket::ConnectedState;
    }
}

bool Publisher::publish(const QString& topic, const QByteArray& data)
{
    // 创建消息
    Message message(topic, data);

    return publish(message);
}

bool Publisher::publish(const Message& message)
{
    // 如果未连接，将消息添加到待发送队列
    if (!isConnected()) {
        QMutexLocker locker(m_pendingMessagesMutex);
        m_pendingMessages.enqueue(message);

        Logger::instance()->warning(QString("Not connected to broker, message queued: %1").arg(message.topic()));

        // 如果启用了自动重连，启动重连定时器
        if (m_autoReconnect && !m_reconnectTimer->isActive()) {
            m_reconnectTimer->start(m_reconnectInterval);
        }

        return false;
    }

    // 如果未注册为发布者，先注册
    if (!m_registered) {
        registerAsPublisher();
    }

    // 发送消息
    if (sendMessage(message)) {
        emit published(message.id());
        return true;
    }

    return false;
}

void Publisher::setAutoReconnect(bool enable, int interval)
{
    m_autoReconnect = enable;
    m_reconnectInterval = interval;

    if (!enable) {
        m_reconnectTimer->stop();
    }
}

void Publisher::handleConnected()
{
    Logger::instance()->info("Connected to broker");

    // 注册为发布者
    registerAsPublisher();

    emit connected();

    // 处理待发送消息
    processPendingMessages();
}

void Publisher::handleDisconnected()
{
    Logger::instance()->info("Disconnected from broker");

    m_registered = false;

    emit disconnected();

    // 如果启用了自动重连，启动重连定时器
    if (m_autoReconnect) {
        m_reconnectTimer->start(m_reconnectInterval);
    }
}

void Publisher::handleError(QAbstractSocket::SocketError socketError)
{
    QString errorMessage = QString("Socket error: %1").arg(m_tcpSocket->errorString());
    Logger::instance()->error(errorMessage);

    emit error(errorMessage);
}

void Publisher::handleLocalError(QLocalSocket::LocalSocketError socketError)
{
    QString errorMessage = QString("Local socket error: %1").arg(m_localSocket->errorString());
    Logger::instance()->error(errorMessage);

    emit error(errorMessage);
}

void Publisher::tryReconnect()
{
    Logger::instance()->info("Trying to reconnect to broker...");

    if (m_useLocalSocket) {
        connectToLocalBroker(m_serverName);
    } else {
        connectToBroker(m_host, m_port);
    }
}

void Publisher::processPendingMessages()
{
    QMutexLocker locker(m_pendingMessagesMutex);

    while (!m_pendingMessages.isEmpty()) {
        Message message = m_pendingMessages.dequeue();
        locker.unlock();

        publish(message);

        locker.relock();
    }
}

void Publisher::registerAsPublisher()
{
    // 创建注册消息
    Message registerMessage("$SYS/REGISTER", "PUBLISHER");

    // 发送注册消息
    if (sendMessage(registerMessage)) {
        m_registered = true;
        Logger::instance()->info("Registered as publisher");
    } else {
        Logger::instance()->error("Failed to register as publisher");
    }
}

bool Publisher::sendMessage(const Message& message)
{
    // 序列化消息
    QByteArray data = message.serialize();

    // 发送消息
    qint64 bytesSent = 0;

    if (m_useLocalSocket && m_localSocket) {
        bytesSent = m_localSocket->write(data);
        m_localSocket->flush();
    } else if (m_tcpSocket) {
        bytesSent = m_tcpSocket->write(data);
        m_tcpSocket->flush();
    }

    if (bytesSent != data.size()) {
        Logger::instance()->error(QString("Failed to send message: %1").arg(message.topic()));
        return false;
    }

    Logger::instance()->debug(QString("Message sent: %1").arg(message.topic()));
    return true;
}

