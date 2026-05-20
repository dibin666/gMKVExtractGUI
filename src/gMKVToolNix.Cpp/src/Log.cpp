#include "gmkvtoolnix/Log.h"

#include <QDateTime>

namespace gmkv {

Logger::Logger(QObject* parent)
    : QObject(parent)
{
}

Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}

QString Logger::logText()
{
    return instance().m_log;
}

void Logger::clear()
{
    instance().m_log.clear();
}

void Logger::log(const QString& message)
{
    Logger& logger = instance();
    const QDateTime actionDate = QDateTime::currentDateTime();
    const QString logMessage = QStringLiteral("[%1][%2] %3")
        .arg(actionDate.toString(QStringLiteral("yyyy-MM-dd")),
             actionDate.toString(QStringLiteral("HH:mm:ss")),
             message);

    logger.m_log += logMessage + QLatin1Char('\n');
    emit logger.logLineAdded(logMessage, actionDate);
}

}
