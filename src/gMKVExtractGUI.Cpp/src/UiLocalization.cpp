#include "gmkvextractgui/UiLocalization.h"

#include "gmkvtoolnix/Localization.h"

#include <QAbstractButton>
#include <QAction>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QObject>
#include <QWidget>

namespace gmkv::gui {

namespace {

constexpr const char* TextKeyProperty = "gmkv.localization.textKey";
constexpr const char* PlaceholderKeyProperty = "gmkv.localization.placeholderKey";
constexpr const char* WindowTitleKeyProperty = "gmkv.localization.windowTitleKey";

QString localizedText(const QString& key)
{
    if (key.trimmed().isEmpty()) {
        return {};
    }
    if (!gmkv::LocalizationManager::isInitialized()) {
        return key;
    }
    return gmkv::LocalizationManager::getString(key);
}

void applyLocalizationToObject(QObject* object)
{
    if (object == nullptr) {
        return;
    }

    const QString textKey = object->property(TextKeyProperty).toString();
    if (!textKey.isEmpty()) {
        const QString text = localizedText(textKey);
        if (auto* button = qobject_cast<QAbstractButton*>(object)) {
            button->setText(text);
        } else if (auto* label = qobject_cast<QLabel*>(object)) {
            label->setText(text);
        } else if (auto* groupBox = qobject_cast<QGroupBox*>(object)) {
            groupBox->setTitle(text);
        } else if (auto* menu = qobject_cast<QMenu*>(object)) {
            menu->setTitle(text);
        } else if (auto* action = qobject_cast<QAction*>(object)) {
            action->setText(text);
        }
    }

    const QString placeholderKey = object->property(PlaceholderKeyProperty).toString();
    if (!placeholderKey.isEmpty()) {
        if (auto* lineEdit = qobject_cast<QLineEdit*>(object)) {
            lineEdit->setPlaceholderText(localizedText(placeholderKey));
        }
    }

    const QString windowTitleKey = object->property(WindowTitleKeyProperty).toString();
    if (!windowTitleKey.isEmpty()) {
        if (auto* widget = qobject_cast<QWidget*>(object)) {
            widget->setWindowTitle(localizedText(windowTitleKey));
        }
    }
}

}

QString loc(const QString& key)
{
    return localizedText(key);
}

QString loc(const QString& key, const QStringList& formatArgs)
{
    if (key.trimmed().isEmpty()) {
        return {};
    }
    if (!gmkv::LocalizationManager::isInitialized()) {
        return key;
    }
    return gmkv::LocalizationManager::getString(key, formatArgs);
}

void setTextKey(QObject* object, const QString& key)
{
    if (object != nullptr) {
        object->setProperty(TextKeyProperty, key);
    }
}

void setPlaceholderKey(QObject* object, const QString& key)
{
    if (object != nullptr) {
        object->setProperty(PlaceholderKeyProperty, key);
    }
}

void setWindowTitleKey(QWidget* widget, const QString& key)
{
    if (widget != nullptr) {
        widget->setProperty(WindowTitleKeyProperty, key);
    }
}

void applyLocalization(QObject* root)
{
    if (root == nullptr) {
        return;
    }

    applyLocalizationToObject(root);
    for (QObject* child : root->children()) {
        applyLocalization(child);
    }
}

}
