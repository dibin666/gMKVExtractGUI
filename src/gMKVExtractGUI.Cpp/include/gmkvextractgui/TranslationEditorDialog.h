#pragma once

#include "gmkvtoolnix/Localization.h"

#include <QDialog>
#include <QString>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;

namespace gmkv::gui {

class TranslationEditorDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit TranslationEditorDialog(QWidget* parent = nullptr);

private:
    void loadMasterFile();
    void refreshCultures(const QString& preferredCulture = QString());
    void loadSelectedCulture();
    void loadCulture(const QString& culture);
    void populateTable();
    void updateSummary();
    void setDirty(bool dirty);
    bool confirmDiscardChanges();
    void saveCurrentFile();
    void createLocale();
    void syncCurrentCulture();
    void handleCellChanged(int row, int column);
    QString currentCulture() const;
    QString translationPath(const QString& culture) const;
    bool rowMatchesFilter(const QString& key, const gmkv::TranslationEntry& entry) const;

    QComboBox* m_languageCombo = nullptr;
    QLineEdit* m_translatorEdit = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    QCheckBox* m_untranslatedOnlyCheckBox = nullptr;
    QTableWidget* m_entriesTable = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QLabel* m_stateLabel = nullptr;
    QPushButton* m_syncButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QString m_translationsDirectory;
    gmkv::TranslationFile m_masterFile;
    gmkv::TranslationFile m_currentFile;
    bool m_hasCurrentFile = false;
    bool m_loading = false;
    bool m_dirty = false;
};

}
