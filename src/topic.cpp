#include "topic.h"

Topic::Topic()
{
}

Topic::Topic(const QString& name)
    : m_name(name)
{
}

Topic::Topic(const QString& name, const QString& dataType)
    : m_name(name)
    , m_dataType(dataType)
{
}

QString Topic::name() const
{
    return m_name;
}

void Topic::setName(const QString& name)
{
    m_name = name;
}

QString Topic::dataType() const
{
    return m_dataType;
}

void Topic::setDataType(const QString& dataType)
{
    m_dataType = dataType;
}

QVariant Topic::property(const QString& key, const QVariant& defaultValue) const
{
    return m_props.value(key, defaultValue);
}

void Topic::setProperty(const QString& key, const QVariant& value)
{
    m_props[key] = value;
}

bool Topic::isValid() const
{
    return !m_name.isEmpty();
}

bool Topic::operator==(const Topic& other) const
{
    return m_name == other.m_name;
}

bool Topic::operator!=(const Topic& other) const
{
    return !(*this == other);
}
