#pragma once

#include "gmkvtoolnix/Settings.h"

#include <QList>
#include <QDialog>
#include <QString>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLineEdit;

namespace gmkv::gui {

struct PlaceholderOption
{
    QString label;
    QString value;
};

class OptionsDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(gmkv::Settings* settings, QWidget* parent = nullptr);

private:
    void addPatternRow(
        QFormLayout* layout,
        const QString& label,
        QLineEdit* edit,
        const QString& defaultPattern,
        const QList<PlaceholderOption>& placeholders);
    void appendPlaceholder(QLineEdit* edit, const QString& placeholder);
    void resetPatternsToDefaults();
    void applyToSettings();
    void openTranslationEditor();

    gmkv::Settings* m_settings = nullptr;
    QComboBox* m_languageCombo = nullptr;
    QCheckBox* m_disableBomCheckBox = nullptr;
    QCheckBox* m_rawExtractionCheckBox = nullptr;
    QCheckBox* m_fullRawExtractionCheckBox = nullptr;
    QLineEdit* m_videoPatternEdit = nullptr;
    QLineEdit* m_audioPatternEdit = nullptr;
    QLineEdit* m_subtitlePatternEdit = nullptr;
    QLineEdit* m_chapterPatternEdit = nullptr;
    QLineEdit* m_attachmentPatternEdit = nullptr;
    QLineEdit* m_tagsPatternEdit = nullptr;
};

}
