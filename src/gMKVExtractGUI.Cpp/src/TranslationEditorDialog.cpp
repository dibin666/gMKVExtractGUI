#include "gmkvextractgui/TranslationEditorDialog.h"

#include "gmkvextractgui/UiLocalization.h"
#include "gmkvtoolnix/Localization.h"
#include "gmkvtoolnix/Log.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScopeGuard>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <exception>

namespace gmkv::gui {

namespace {

constexpr int KeyColumn = 0;
constexpr int SourceColumn = 1;
constexpr int TranslationColumn = 2;
constexpr int TranslatedColumn = 3;
constexpr int NotesColumn = 4;

QString canonicalCulture(const QString& culture)
{
    return gmkv::TranslationPathService::canonicalCultureCode(culture);
}

QTableWidgetItem* readOnlyItem(const QString& text)
{
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

}

TranslationEditorDialog::TranslationEditorDialog(QWidget* parent)
    : QDialog(parent)
    , m_translationsDirectory(QCoreApplication::applicationDirPath())
{
    setWindowTitle(loc(QStringLiteral("UI.TranslationEditor.Title")));
    resize(980, 620);

    auto* layout = new QVBoxLayout(this);

    auto* settingsLayout = new QFormLayout();
    auto* cultureLayout = new QHBoxLayout();
    m_languageCombo = new QComboBox(this);
    m_languageCombo->setEditable(true);
    auto* loadButton = new QPushButton(loc(QStringLiteral("UI.TranslationEditor.Actions.Load")), this);
    auto* createButton = new QPushButton(loc(QStringLiteral("UI.TranslationEditor.Actions.NewLocale")), this);
    m_syncButton = new QPushButton(loc(QStringLiteral("UI.TranslationEditor.Actions.Sync")), this);
    cultureLayout->addWidget(m_languageCombo, 1);
    cultureLayout->addWidget(loadButton);
    cultureLayout->addWidget(createButton);
    cultureLayout->addWidget(m_syncButton);
    settingsLayout->addRow(loc(QStringLiteral("UI.TranslationEditor.Fields.TargetCulture")), cultureLayout);

    m_translatorEdit = new QLineEdit(this);
    settingsLayout->addRow(loc(QStringLiteral("UI.TranslationEditor.Fields.Translator")), m_translatorEdit);

    auto* filterLayout = new QHBoxLayout();
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(loc(QStringLiteral("UI.TranslationEditor.Fields.Search")));
    m_untranslatedOnlyCheckBox = new QCheckBox(loc(QStringLiteral("UI.TranslationEditor.Fields.ShowOnlyUntranslated")), this);
    filterLayout->addWidget(m_filterEdit, 1);
    filterLayout->addWidget(m_untranslatedOnlyCheckBox);
    settingsLayout->addRow(loc(QStringLiteral("UI.TranslationEditor.Fields.Search")), filterLayout);
    layout->addLayout(settingsLayout);

    m_entriesTable = new QTableWidget(0, 5, this);
    m_entriesTable->setHorizontalHeaderLabels({
        loc(QStringLiteral("UI.TranslationEditor.Columns.Key")),
        loc(QStringLiteral("UI.TranslationEditor.Columns.Source")),
        loc(QStringLiteral("UI.TranslationEditor.Columns.Translation")),
        loc(QStringLiteral("UI.TranslationEditor.Columns.IsTranslated")),
        loc(QStringLiteral("UI.TranslationEditor.Columns.Notes")),
    });
    m_entriesTable->horizontalHeader()->setStretchLastSection(true);
    m_entriesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(m_entriesTable, 1);

    auto* footerLayout = new QHBoxLayout();
    m_summaryLabel = new QLabel(this);
    m_stateLabel = new QLabel(this);
    footerLayout->addWidget(m_summaryLabel, 1);
    footerLayout->addWidget(m_stateLabel, 1);
    m_saveButton = new QPushButton(loc(QStringLiteral("UI.TranslationEditor.Actions.Save")), this);
    auto* closeButton = new QPushButton(loc(QStringLiteral("UI.TranslationEditor.Actions.Close")), this);
    footerLayout->addWidget(m_saveButton);
    footerLayout->addWidget(closeButton);
    layout->addLayout(footerLayout);

    connect(loadButton, &QPushButton::clicked, this, &TranslationEditorDialog::loadSelectedCulture);
    connect(createButton, &QPushButton::clicked, this, &TranslationEditorDialog::createLocale);
    connect(m_syncButton, &QPushButton::clicked, this, &TranslationEditorDialog::syncCurrentCulture);
    connect(m_saveButton, &QPushButton::clicked, this, [this]() {
        try {
            saveCurrentFile();
            QMessageBox::information(this, loc(QStringLiteral("UI.TranslationEditor.Title")), loc(QStringLiteral("UI.TranslationEditor.Success.LocaleSaved"), { currentCulture() }));
        } catch (const std::exception& ex) {
            gmkv::Logger::log(QString::fromUtf8(ex.what()));
            QMessageBox::critical(this, loc(QStringLiteral("UI.TranslationEditor.Title")), QString::fromUtf8(ex.what()));
        }
    });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &TranslationEditorDialog::populateTable);
    connect(m_untranslatedOnlyCheckBox, &QCheckBox::toggled, this, &TranslationEditorDialog::populateTable);
    connect(m_translatorEdit, &QLineEdit::textChanged, this, [this]() {
        if (!m_loading && m_hasCurrentFile) {
            setDirty(true);
        }
    });
    connect(m_entriesTable, &QTableWidget::cellChanged, this, &TranslationEditorDialog::handleCellChanged);

    try {
        loadMasterFile();
        refreshCultures(QStringLiteral("en"));
        loadSelectedCulture();
    } catch (const std::exception& ex) {
        gmkv::Logger::log(QString::fromUtf8(ex.what()));
        QMessageBox::critical(this, loc(QStringLiteral("UI.TranslationEditor.Title")), QString::fromUtf8(ex.what()));
    }
}

void TranslationEditorDialog::loadMasterFile()
{
    const QString masterPath = gmkv::TranslationPathService::masterFilePath(m_translationsDirectory);
    if (QFileInfo::exists(masterPath)) {
        m_masterFile = gmkv::TranslationFileService::loadFile(masterPath);
        m_masterFile.metadata.culture = QStringLiteral("en");
        return;
    }

    m_masterFile = gmkv::JsonLocalizationService::createBuiltInEnglishTranslationFile();
}

void TranslationEditorDialog::refreshCultures(const QString& preferredCulture)
{
    const QString previousCulture = preferredCulture.trimmed().isEmpty()
        ? currentCulture()
        : canonicalCulture(preferredCulture);

    QStringList cultures;
    for (const QString& path : gmkv::TranslationPathService::enumerateTranslationFiles(m_translationsDirectory)) {
        QString culture;
        if (gmkv::TranslationPathService::tryGetCultureFromPath(path, &culture)) {
            cultures.append(canonicalCulture(culture));
        }
    }
    if (!cultures.contains(QStringLiteral("en"), Qt::CaseInsensitive)) {
        cultures.prepend(QStringLiteral("en"));
    }
    cultures.removeDuplicates();
    cultures.sort(Qt::CaseInsensitive);

    const QSignalBlocker blocker(m_languageCombo);
    m_languageCombo->clear();
    m_languageCombo->addItems(cultures);
    const int selectedIndex = cultures.indexOf(previousCulture);
    if (selectedIndex >= 0) {
        m_languageCombo->setCurrentIndex(selectedIndex);
    } else if (!previousCulture.isEmpty()) {
        m_languageCombo->setCurrentText(previousCulture);
    }
}

void TranslationEditorDialog::loadSelectedCulture()
{
    const QString culture = canonicalCulture(m_languageCombo->currentText());
    if (culture.isEmpty()) {
        QMessageBox::warning(this, loc(QStringLiteral("UI.TranslationEditor.Title")), loc(QStringLiteral("UI.TranslationEditor.Errors.CultureCodeRequired")));
        return;
    }
    if (!confirmDiscardChanges()) {
        return;
    }

    try {
        loadCulture(culture);
    } catch (const std::exception& ex) {
        gmkv::Logger::log(QString::fromUtf8(ex.what()));
        QMessageBox::critical(this, loc(QStringLiteral("UI.TranslationEditor.Title")), QString::fromUtf8(ex.what()));
    }
}

void TranslationEditorDialog::loadCulture(const QString& culture)
{
    m_loading = true;
    const auto resetLoading = qScopeGuard([this]() {
        m_loading = false;
    });

    const QString path = gmkv::TranslationPathService::existingTranslationFilePath(m_translationsDirectory, culture);
    if (QFileInfo::exists(path)) {
        m_currentFile = gmkv::TranslationFileService::loadFile(path);
    } else if (culture.compare(QStringLiteral("en"), Qt::CaseInsensitive) == 0) {
        m_currentFile = m_masterFile;
    } else {
        throw std::runtime_error(QStringLiteral("Translation file not found: %1").arg(path).toStdString());
    }

    m_currentFile.metadata.culture = canonicalCulture(m_currentFile.metadata.culture);
    if (m_currentFile.metadata.culture.isEmpty()) {
        m_currentFile.metadata.culture = culture;
    }

    m_hasCurrentFile = true;
    m_translatorEdit->setText(m_currentFile.metadata.translator);
    refreshCultures(m_currentFile.metadata.culture);
    populateTable();
    setDirty(false);
}

void TranslationEditorDialog::populateTable()
{
    if (!m_hasCurrentFile) {
        return;
    }

    const QSignalBlocker blocker(m_entriesTable);
    m_entriesTable->setRowCount(0);

    for (auto it = m_currentFile.entries.begin(); it != m_currentFile.entries.end(); ++it) {
        if (!rowMatchesFilter(it.key(), it.value())) {
            continue;
        }

        const int row = m_entriesTable->rowCount();
        m_entriesTable->insertRow(row);

        auto* keyItem = readOnlyItem(it.key());
        keyItem->setData(Qt::UserRole, it.key());
        m_entriesTable->setItem(row, KeyColumn, keyItem);
        m_entriesTable->setItem(row, SourceColumn, readOnlyItem(it.value().source));
        m_entriesTable->setItem(row, TranslationColumn, new QTableWidgetItem(it.value().translation));

        auto* translatedItem = new QTableWidgetItem();
        translatedItem->setFlags((translatedItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        translatedItem->setCheckState(it.value().isTranslated ? Qt::Checked : Qt::Unchecked);
        m_entriesTable->setItem(row, TranslatedColumn, translatedItem);

        m_entriesTable->setItem(row, NotesColumn, readOnlyItem(it.value().notes));
    }

    m_entriesTable->resizeColumnsToContents();
    updateSummary();
}

void TranslationEditorDialog::updateSummary()
{
    int translatedCount = 0;
    for (const gmkv::TranslationEntry& entry : m_currentFile.entries) {
        if (entry.isTranslated) {
            ++translatedCount;
        }
    }

    m_summaryLabel->setText(loc(QStringLiteral("UI.TranslationEditor.Fields.Summary"), { QString::number(translatedCount), QString::number(m_currentFile.entries.size()) }));
}

void TranslationEditorDialog::setDirty(bool dirty)
{
    m_dirty = dirty;
    m_stateLabel->setText(dirty ? loc(QStringLiteral("UI.TranslationEditor.Status.Dirty")) : loc(QStringLiteral("UI.TranslationEditor.Status.Clean")));
    m_saveButton->setEnabled(m_hasCurrentFile && dirty);
    m_syncButton->setEnabled(m_hasCurrentFile && currentCulture().compare(QStringLiteral("en"), Qt::CaseInsensitive) != 0);
}

bool TranslationEditorDialog::confirmDiscardChanges()
{
    if (!m_dirty) {
        return true;
    }

    return QMessageBox::question(
        this,
        loc(QStringLiteral("UI.TranslationEditor.Title")),
        loc(QStringLiteral("UI.TranslationEditor.Dialogs.DiscardChanges"), { currentCulture() })) == QMessageBox::Yes;
}

void TranslationEditorDialog::saveCurrentFile()
{
    if (!m_hasCurrentFile) {
        throw std::runtime_error(loc(QStringLiteral("UI.TranslationEditor.Errors.SelectCulture")).toStdString());
    }

    m_currentFile.metadata.culture = currentCulture();
    m_currentFile.metadata.translator = m_translatorEdit->text().trimmed();
    m_currentFile.metadata.lastEditDate = QDateTime::currentDateTimeUtc();
    if (!m_currentFile.metadata.creationDate.isValid()) {
        m_currentFile.metadata.creationDate = m_currentFile.metadata.lastEditDate;
    }

    gmkv::TranslationFileService::saveFile(m_currentFile, translationPath(m_currentFile.metadata.culture));
    refreshCultures(m_currentFile.metadata.culture);
    setDirty(false);
}

void TranslationEditorDialog::createLocale()
{
    if (!confirmDiscardChanges()) {
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(loc(QStringLiteral("UI.TranslationEditor.Dialogs.NewLocaleTitle")));
    auto* layout = new QFormLayout(&dialog);
    auto* cultureEdit = new QLineEdit(&dialog);
    auto* translatorEdit = new QLineEdit(m_translatorEdit->text().trimmed(), &dialog);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(loc(QStringLiteral("UI.TranslationEditor.Fields.NewCulture")), cultureEdit);
    layout->addRow(loc(QStringLiteral("UI.TranslationEditor.Fields.Translator")), translatorEdit);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString culture = canonicalCulture(cultureEdit->text());
    if (culture.isEmpty()) {
        QMessageBox::warning(this, loc(QStringLiteral("UI.TranslationEditor.Title")), loc(QStringLiteral("UI.TranslationEditor.Errors.CultureCodeRequired")));
        return;
    }

    const QString path = translationPath(culture);
    if (QFileInfo::exists(path)) {
        QMessageBox::warning(this, loc(QStringLiteral("UI.TranslationEditor.Title")), loc(QStringLiteral("UI.TranslationEditor.Errors.LocaleExists"), { culture }));
        return;
    }

    loadMasterFile();
    gmkv::TranslationFile newFile = gmkv::TranslationMaintenanceService::createTemplate(m_masterFile, culture, translatorEdit->text().trimmed());
    gmkv::TranslationFileService::saveFile(newFile, path);
    refreshCultures(culture);
    loadCulture(culture);
    QMessageBox::information(this, loc(QStringLiteral("UI.TranslationEditor.Title")), loc(QStringLiteral("UI.TranslationEditor.Success.LocaleCreated"), { culture }));
}

void TranslationEditorDialog::syncCurrentCulture()
{
    if (!m_hasCurrentFile) {
        QMessageBox::warning(this, loc(QStringLiteral("UI.TranslationEditor.Title")), loc(QStringLiteral("UI.TranslationEditor.Errors.SelectCulture")));
        return;
    }
    if (currentCulture().compare(QStringLiteral("en"), Qt::CaseInsensitive) == 0) {
        QMessageBox::warning(this, loc(QStringLiteral("UI.TranslationEditor.Title")), loc(QStringLiteral("UI.TranslationEditor.Errors.CannotSyncEnglish")));
        return;
    }

    try {
        saveCurrentFile();
        loadMasterFile();
        const gmkv::TranslationFile target = gmkv::TranslationFileService::loadFile(
            gmkv::TranslationPathService::existingTranslationFilePath(m_translationsDirectory, currentCulture()));
        const gmkv::TranslationSyncResult result = gmkv::TranslationMaintenanceService::synchronize(m_masterFile, target);
        gmkv::TranslationFileService::saveFile(result.translationFile, translationPath(result.translationFile.metadata.culture));
        loadCulture(result.translationFile.metadata.culture);
        QMessageBox::information(
            this,
            loc(QStringLiteral("UI.TranslationEditor.Title")),
            loc(QStringLiteral("UI.TranslationEditor.Success.LocaleSynced"), {
                result.translationFile.metadata.culture,
                QString::number(result.addedCount),
                QString::number(result.updatedCount),
                QString::number(result.removedCount),
            }));
    } catch (const std::exception& ex) {
        gmkv::Logger::log(QString::fromUtf8(ex.what()));
        QMessageBox::critical(this, loc(QStringLiteral("UI.TranslationEditor.Title")), QString::fromUtf8(ex.what()));
    }
}

void TranslationEditorDialog::handleCellChanged(int row, int column)
{
    if (m_loading || row < 0 || !m_hasCurrentFile) {
        return;
    }

    QTableWidgetItem* keyItem = m_entriesTable->item(row, KeyColumn);
    if (keyItem == nullptr) {
        return;
    }

    const QString key = keyItem->data(Qt::UserRole).toString();
    if (!m_currentFile.entries.contains(key)) {
        return;
    }

    gmkv::TranslationEntry& entry = m_currentFile.entries[key];
    entry.translation = m_entriesTable->item(row, TranslationColumn) == nullptr ? QString() : m_entriesTable->item(row, TranslationColumn)->text();
    entry.isTranslated = m_entriesTable->item(row, TranslatedColumn) != nullptr
        && m_entriesTable->item(row, TranslatedColumn)->checkState() == Qt::Checked;

    if (column == TranslationColumn) {
        const bool translated = !entry.translation.trimmed().isEmpty();
        if (entry.isTranslated != translated) {
            const QSignalBlocker blocker(m_entriesTable);
            entry.isTranslated = translated;
            if (m_entriesTable->item(row, TranslatedColumn) != nullptr) {
                m_entriesTable->item(row, TranslatedColumn)->setCheckState(translated ? Qt::Checked : Qt::Unchecked);
            }
        }
    }

    setDirty(true);
    updateSummary();
}

QString TranslationEditorDialog::currentCulture() const
{
    return canonicalCulture(m_languageCombo->currentText());
}

QString TranslationEditorDialog::translationPath(const QString& culture) const
{
    return gmkv::TranslationPathService::translationFilePath(m_translationsDirectory, culture);
}

bool TranslationEditorDialog::rowMatchesFilter(const QString& key, const gmkv::TranslationEntry& entry) const
{
    if (m_untranslatedOnlyCheckBox->isChecked() && entry.isTranslated) {
        return false;
    }

    const QString filter = m_filterEdit->text().trimmed();
    if (filter.isEmpty()) {
        return true;
    }

    return key.contains(filter, Qt::CaseInsensitive)
        || entry.source.contains(filter, Qt::CaseInsensitive)
        || entry.translation.contains(filter, Qt::CaseInsensitive)
        || entry.notes.contains(filter, Qt::CaseInsensitive);
}

}
