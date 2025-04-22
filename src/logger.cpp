#include "logger.h"

// 初始化静态成员变量
Logger* Logger::m_instance = nullptr;

Logger* Logger::instance()
{
    if (!m_instance) {
        m_instance = new Logger();
    }
    return m_instance;
}

Logger::Logger(QObject* parent)
    : QObject(parent)
    , m_logLevel(INFO)
    , m_mutex(new QMutex())
    , m_initialized(false)
{
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        m_textStream.flush();
        m_logFile.close();
    }

    delete m_mutex;
}

bool Logger::init(const QString& logFilePath, LogLevel level)
{
    {
        QMutexLocker locker(m_mutex);

        if (m_initialized) {
            return true;
        }

        m_logLevel = level;
        m_logFile.setFileName(logFilePath);

        if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            qDebug() << "Failed to open log file:" << logFilePath;
            return false;
        }

        m_textStream.setDevice(&m_logFile);
        m_initialized = true;
    }

    // 在锁的作用域外调用 info，避免死锁
    info("Logger initialized");
    return true;
}

void Logger::debug(const QString& message)
{
    log(DEBUG, message);
}

void Logger::info(const QString& message)
{
    log(INFO, message);
}

void Logger::warning(const QString& message)
{
    log(WARNING, message);
}

void Logger::error(const QString& message)
{
    log(ERROR, message);
}

void Logger::fatal(const QString& message)
{
    log(FATAL, message);
}

void Logger::log(LogLevel level, const QString& message)
{
    if (!m_initialized) {
        qDebug() << "Logger not initialized";
        return;
    }

    if (level < m_logLevel) {
        return;
    }

    QMutexLocker locker(m_mutex);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logEntry = QString("[%1] [%2] %3")
                           .arg(timestamp)
                           .arg(levelToString(level))
                           .arg(message);

    m_textStream << logEntry << Qt::endl;
    m_textStream.flush();

    // 同时输出到控制台
    qDebug().noquote() << logEntry;
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        case FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}
