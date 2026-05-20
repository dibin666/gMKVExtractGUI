#pragma once

#include <QObject>
#include <QString>

class QDateTime;

namespace gmkv {

class Logger final : public QObject
{
    Q_OBJECT

public:
    static Logger& instance();

    static QString logText();
    static void clear();
    static void log(const QString& message);

signals:
    void logLineAdded(const QString& lineAdded, const QDateTime& actionDate);

private:
    explicit Logger(QObject* parent = nullptr);

    QString m_log;
};

}
