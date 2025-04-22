#ifndef MESSAGE_H
#define MESSAGE_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QUuid>
#include <QIODevice>

/**
 * @brief 消息类，表示DDS系统中的消息
 */
class Message
{
public:
    /**
     * @brief 默认构造函数
     */
    Message();

    /**
     * @brief 构造函数
     * @param topic 消息主题
     * @param data 消息数据
     */
    Message(const QString& topic, const QByteArray& data);

    /**
     * @brief 获取消息ID
     * @return 消息ID
     */
    QString id() const;

    /**
     * @brief 获取消息主题
     * @return 消息主题
     */
    QString topic() const;

    /**
     * @brief 设置消息主题
     * @param topic 消息主题
     */
    void setTopic(const QString& topic);

    /**
     * @brief 获取消息数据
     * @return 消息数据
     */
    QByteArray data() const;

    /**
     * @brief 设置消息数据
     * @param data 消息数据
     */
    void setData(const QByteArray& data);

    /**
     * @brief 获取消息时间戳
     * @return 消息时间戳
     */
    QDateTime timestamp() const;

    /**
     * @brief 将消息序列化为字节数组
     * @return 序列化后的字节数组
     */
    QByteArray serialize() const;

    /**
     * @brief 从字节数组反序列化消息
     * @param data 序列化的字节数组（不包含长度前缀）
     * @return 是否反序列化成功
     */
    bool deserialize(const QByteArray& data);

    /**
     * @brief 从带有长度前缀的完整消息中提取消息内容
     * @param frameData 带有长度前缀的完整消息
     * @param bytesRead 输出参数，表示从输入数据中读取的字节数
     * @return 如果成功，返回消息内容；如果失败，返回空字节数组
     */
    static QByteArray extractMessageContent(const QByteArray& frameData, int& bytesRead);

private:
    QString m_id;           ///< 消息ID
    QString m_topic;        ///< 消息主题
    QByteArray m_data;      ///< 消息数据
    QDateTime m_timestamp;  ///< 消息时间戳
};

// 注册元类型，使其可以在信号槽中使用
Q_DECLARE_METATYPE(Message)

#endif // MESSAGE_H
