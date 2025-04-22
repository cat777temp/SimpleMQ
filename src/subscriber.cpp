#include "subscriber.h"
#include "logger.h"

Subscriber::Subscriber(QObject* parent)
    : QObject(parent)
    , m_tcpSocket(nullptr)
    , m_localSocket(nullptr)
    , m_port(0)
    , m_useLocalSocket(false)
    , m_autoReconnect(false)
    , m_reconnectInterval(5000)
    , m_reconnectTimer(new QTimer(this))
    , m_registered(false)
{
    // 连接重连定时器信号
    connect(m_reconnectTimer, &QTimer::timeout, this, &Subscriber::tryReconnect);
}

Subscriber::~Subscriber()
{
    disconnectFromBroker();
}

bool Subscriber::connectToBroker(const QString& host, int port)
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
    connect(m_tcpSocket, &QTcpSocket::connected, this, &Subscriber::handleConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &Subscriber::handleDisconnected);
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &Subscriber::handleTcpReadyRead);
    connect(m_tcpSocket, &QTcpSocket::errorOccurred,
            this, &Subscriber::handleError);

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

bool Subscriber::connectToLocalBroker(const QString& serverName)
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
    connect(m_localSocket, &QLocalSocket::connected, this, &Subscriber::handleConnected);
    connect(m_localSocket, &QLocalSocket::disconnected, this, &Subscriber::handleDisconnected);
    connect(m_localSocket, &QLocalSocket::readyRead, this, &Subscriber::handleLocalReadyRead);
    connect(m_localSocket, &QLocalSocket::errorOccurred,
            this, &Subscriber::handleLocalError);

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

void Subscriber::disconnectFromBroker()
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

    m_registered = false;
}

bool Subscriber::isConnected() const
{
    if (m_useLocalSocket) {
        return m_localSocket && m_localSocket->state() == QLocalSocket::ConnectedState;
    } else {
        return m_tcpSocket && m_tcpSocket->state() == QTcpSocket::ConnectedState;
    }
}

bool Subscriber::subscribe(const QString& topic)
{
    // 如果未连接，返回失败
    if (!isConnected()) {
        Logger::instance()->warning(QString("Not connected to broker, cannot subscribe to topic: %1").arg(topic));
        return false;
    }

    // 如果未注册为订阅者，先注册
    if (!m_registered) {
        registerAsSubscriber();
    }

    // 创建订阅消息
    Message subscribeMessage("$SYS/SUBSCRIBE", topic.toUtf8());

    // 发送订阅消息
    if (sendMessage(subscribeMessage)) {
        m_subscribedTopics.insert(topic);
        Logger::instance()->info(QString("Subscribed to topic: %1").arg(topic));
        emit subscribed(topic);
        return true;
    }

    return false;
}

bool Subscriber::unsubscribe(const QString& topic)
{
    // 如果未连接，返回失败
    if (!isConnected()) {
        Logger::instance()->warning(QString("Not connected to broker, cannot unsubscribe from topic: %1").arg(topic));
        return false;
    }

    // 如果未订阅该主题，直接返回成功
    if (!m_subscribedTopics.contains(topic)) {
        return true;
    }

    // 创建取消订阅消息
    Message unsubscribeMessage("$SYS/UNSUBSCRIBE", topic.toUtf8());

    // 发送取消订阅消息
    if (sendMessage(unsubscribeMessage)) {
        m_subscribedTopics.remove(topic);
        Logger::instance()->info(QString("Unsubscribed from topic: %1").arg(topic));
        emit unsubscribed(topic);
        return true;
    }

    return false;
}

QSet<QString> Subscriber::subscribedTopics() const
{
    return m_subscribedTopics;
}

void Subscriber::setAutoReconnect(bool enable, int interval)
{
    m_autoReconnect = enable;
    m_reconnectInterval = interval;

    if (!enable) {
        m_reconnectTimer->stop();
    }
}

void Subscriber::handleConnected()
{
    Logger::instance()->info("Connected to broker");

    // 注册为订阅者
    registerAsSubscriber();

    emit connected();

    // 重新订阅所有主题
    resubscribeAll();
}

void Subscriber::handleDisconnected()
{
    Logger::instance()->info("Disconnected from broker");

    m_registered = false;

    emit disconnected();

    // 如果启用了自动重连，启动重连定时器
    if (m_autoReconnect) {
        m_reconnectTimer->start(m_reconnectInterval);
    }
}

void Subscriber::handleTcpReadyRead()
{
    // 读取数据
    QByteArray data = m_tcpSocket->readAll();

    // 处理收到的数据
    processReceivedData(data);
}

void Subscriber::handleLocalReadyRead()
{
    // 读取数据
    QByteArray data = m_localSocket->readAll();

    // 处理收到的数据
    processReceivedData(data);
}

void Subscriber::handleError(QAbstractSocket::SocketError socketError)
{
    QString errorMessage = QString("Socket error: %1").arg(m_tcpSocket->errorString());
    Logger::instance()->error(errorMessage);

    emit error(errorMessage);
}

void Subscriber::handleLocalError(QLocalSocket::LocalSocketError socketError)
{
    QString errorMessage = QString("Local socket error: %1").arg(m_localSocket->errorString());
    Logger::instance()->error(errorMessage);

    emit error(errorMessage);
}

void Subscriber::tryReconnect()
{
    Logger::instance()->info("Trying to reconnect to broker...");

    if (m_useLocalSocket) {
        connectToLocalBroker(m_serverName);
    } else {
        connectToBroker(m_host, m_port);
    }
}

void Subscriber::registerAsSubscriber()
{
    // 创建注册消息
    Message registerMessage("$SYS/REGISTER", "SUBSCRIBER");

    // 发送注册消息
    if (sendMessage(registerMessage)) {
        m_registered = true;
        Logger::instance()->info("Registered as subscriber");
    } else {
        Logger::instance()->error("Failed to register as subscriber");
    }
}

void Subscriber::resubscribeAll()
{
    // 获取已订阅的主题
    QSet<QString> topics = m_subscribedTopics;

    // 清空已订阅的主题
    m_subscribedTopics.clear();

    // 重新订阅所有主题
    for (const QString& topic : topics) {
        subscribe(topic);
    }
}

bool Subscriber::sendMessage(const Message& message)
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

void Subscriber::processReceivedData(const QByteArray& data)
{
    // 反序列化消息
    Message message;
    if (!message.deserialize(data)) {
        Logger::instance()->warning("Failed to deserialize received message");
        return;
    }

    // 如果是系统消息，不发送给用户
    if (message.topic().startsWith("$SYS/")) {
        return;
    }

    // 检查是否订阅了该主题
    if (m_subscribedTopics.contains(message.topic())) {
        Logger::instance()->debug(QString("Received message on topic: %1").arg(message.topic()));
        emit messageReceived(message);
    }
}
