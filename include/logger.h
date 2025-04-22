#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDebug>

/**
 * @brief 日志记录类，用于记录系统运行日志
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 日志级别枚举
     */
    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    /**
     * @brief 获取Logger单例实例
     * @return Logger实例
     */
    static Logger* instance();

    /**
     * @brief 初始化日志系统
     * @param logFilePath 日志文件路径
     * @param level 日志级别
     * @return 是否初始化成功
     */
    bool init(const QString& logFilePath, LogLevel level = INFO);

    /**
     * @brief 记录调试级别日志
     * @param message 日志消息
     */
    void debug(const QString& message);

    /**
     * @brief 记录信息级别日志
     * @param message 日志消息
     */
    void info(const QString& message);

    /**
     * @brief 记录警告级别日志
     * @param message 日志消息
     */
    void warning(const QString& message);

    /**
     * @brief 记录错误级别日志
     * @param message 日志消息
     */
    void error(const QString& message);

    /**
     * @brief 记录致命错误级别日志
     * @param message 日志消息
     */
    void fatal(const QString& message);

private:
    /**
     * @brief 构造函数（私有）
     */
    explicit Logger(QObject* parent = nullptr);

    /**
     * @brief 析构函数（私有）
     */
    ~Logger();

    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, const QString& message);

    /**
     * @brief 获取日志级别字符串
     * @param level 日志级别
     * @return 日志级别字符串
     */
    QString levelToString(LogLevel level);

private:
    static Logger* m_instance;      ///< 单例实例
    QFile m_logFile;                ///< 日志文件
    QTextStream m_textStream;       ///< 文本流
    LogLevel m_logLevel;            ///< 日志级别
    QMutex* m_mutex;                 ///< 互斥锁，保证线程安全
    bool m_initialized;             ///< 是否已初始化
};

#endif // LOGGER_H
