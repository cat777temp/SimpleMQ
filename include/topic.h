#ifndef TOPIC_H
#define TOPIC_H

#include <QString>
#include <QHash>
#include <QVariant>

/**
 * @brief 主题类，表示DDS系统中的主题
 */
class Topic
{
public:
    /**
     * @brief 默认构造函数
     */
    Topic();

    /**
     * @brief 构造函数
     * @param name 主题名称
     */
    explicit Topic(const QString& name);

    /**
     * @brief 构造函数
     * @param name 主题名称
     * @param dataType 数据类型
     */
    Topic(const QString& name, const QString& dataType);

    /**
     * @brief 获取主题名称
     * @return 主题名称
     */
    QString name() const;

    /**
     * @brief 设置主题名称
     * @param name 主题名称
     */
    void setName(const QString& name);

    /**
     * @brief 获取数据类型
     * @return 数据类型
     */
    QString dataType() const;

    /**
     * @brief 设置数据类型
     * @param dataType 数据类型
     */
    void setDataType(const QString& dataType);

    /**
     * @brief 获取主题属性
     * @param key 属性键
     * @param defaultValue 默认值
     * @return 属性值
     */
    QVariant property(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /**
     * @brief 设置主题属性
     * @param key 属性键
     * @param value 属性值
     */
    void setProperty(const QString& key, const QVariant& value);

    /**
     * @brief 判断主题是否有效
     * @return 是否有效
     */
    bool isValid() const;

    /**
     * @brief 重载相等运算符
     * @param other 另一个主题
     * @return 是否相等
     */
    bool operator==(const Topic& other) const;

    /**
     * @brief 重载不等运算符
     * @param other 另一个主题
     * @return 是否不等
     */
    bool operator!=(const Topic& other) const;

private:
    QString m_name;                     ///< 主题名称
    QString m_dataType;                 ///< 数据类型
    QHash<QString, QVariant> m_props;   ///< 主题属性
};

// 注册元类型，使其可以在信号槽中使用
Q_DECLARE_METATYPE(Topic)

#endif // TOPIC_H
