#pragma once

#include <QString>
#include <QStringList>

class QObject;
class QWidget;

namespace gmkv::gui {

QString loc(const QString& key);
QString loc(const QString& key, const QStringList& formatArgs);
void setTextKey(QObject* object, const QString& key);
void setPlaceholderKey(QObject* object, const QString& key);
void setWindowTitleKey(QWidget* widget, const QString& key);
void applyLocalization(QObject* root);

}
