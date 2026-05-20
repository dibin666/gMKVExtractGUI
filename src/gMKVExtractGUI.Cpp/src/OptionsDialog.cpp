#include "gmkvextractgui/OptionsDialog.h"

#include "gmkvextractgui/TranslationEditorDialog.h"
#include "gmkvextractgui/UiLocalization.h"
#include "gmkvtoolnix/FilenamePatterns.h"
#include "gmkvtoolnix/Localization.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace gmkv::gui {

namespace {

QString patternToken(const char* token)
{
    return QString::fromLatin1(token);
}

QList<PlaceholderOption> commonPlaceholders()
{
    return {
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.InputFilenameWithoutExtension")), patternToken(gmkv::FilenamePatterns::FilenameNoExt) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.InputFilenameWithExtension")), patternToken(gmkv::FilenamePatterns::Filename) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.DirectorySeparator")), patternToken(gmkv::FilenamePatterns::DirectorySeparator) },
    };
}

QList<PlaceholderOption> trackPlaceholders()
{
    QList<PlaceholderOption> placeholders = commonPlaceholders();
    placeholders.append({
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackNumber.NoFormat")), patternToken(gmkv::FilenamePatterns::TrackNumber) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackNumber.OneDigit")), patternToken(gmkv::FilenamePatterns::TrackNumber_0) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackNumber.TwoDigits")), patternToken(gmkv::FilenamePatterns::TrackNumber_00) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackNumber.ThreeDigits")), patternToken(gmkv::FilenamePatterns::TrackNumber_000) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackId.NoFormat")), patternToken(gmkv::FilenamePatterns::TrackID) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackId.OneDigit")), patternToken(gmkv::FilenamePatterns::TrackID_0) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackId.TwoDigits")), patternToken(gmkv::FilenamePatterns::TrackID_00) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackId.ThreeDigits")), patternToken(gmkv::FilenamePatterns::TrackID_000) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackName")), patternToken(gmkv::FilenamePatterns::TrackName) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackLanguage")), patternToken(gmkv::FilenamePatterns::TrackLanguage) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackLanguageIetf")), patternToken(gmkv::FilenamePatterns::TrackLanguageIetf) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackCodecId")), patternToken(gmkv::FilenamePatterns::TrackCodecID) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackCodecPrivate")), patternToken(gmkv::FilenamePatterns::TrackCodecPrivate) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackDelay")), patternToken(gmkv::FilenamePatterns::TrackDelay) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackEffectiveDelay")), patternToken(gmkv::FilenamePatterns::TrackEffectiveDelay) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.TrackForced")), patternToken(gmkv::FilenamePatterns::TrackForced) },
    });
    return placeholders;
}

QList<PlaceholderOption> videoPlaceholders()
{
    QList<PlaceholderOption> placeholders = trackPlaceholders();
    placeholders.append({
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.VideoPixelWidth")), patternToken(gmkv::FilenamePatterns::VideoPixelWidth) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.VideoPixelHeight")), patternToken(gmkv::FilenamePatterns::VideoPixelHeight) },
    });
    return placeholders;
}

QList<PlaceholderOption> audioPlaceholders()
{
    QList<PlaceholderOption> placeholders = trackPlaceholders();
    placeholders.append({
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.AudioSamplingFrequency")), patternToken(gmkv::FilenamePatterns::AudioSamplingFrequency) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.AudioChannels")), patternToken(gmkv::FilenamePatterns::AudioChannels) },
    });
    return placeholders;
}

QList<PlaceholderOption> attachmentPlaceholders()
{
    QList<PlaceholderOption> placeholders = commonPlaceholders();
    placeholders.append({
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.AttachmentId.NoFormat")), patternToken(gmkv::FilenamePatterns::AttachmentID) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.AttachmentId.OneDigit")), patternToken(gmkv::FilenamePatterns::AttachmentID_0) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.AttachmentId.TwoDigits")), patternToken(gmkv::FilenamePatterns::AttachmentID_00) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.AttachmentId.ThreeDigits")), patternToken(gmkv::FilenamePatterns::AttachmentID_000) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.AttachmentFilename")), patternToken(gmkv::FilenamePatterns::AttachmentFilename) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.AttachmentMimeType")), patternToken(gmkv::FilenamePatterns::AttachmentMimeType) },
        { loc(QStringLiteral("UI.OptionsForm.Placeholders.AttachmentFileSizeBytes")), patternToken(gmkv::FilenamePatterns::AttachmentFileSize) },
    });
    return placeholders;
}

}

OptionsDialog::OptionsDialog(gmkv::Settings* settings, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
{
    setWindowTitle(loc(QStringLiteral("UI.OptionsForm.Title")));
    resize(680, 460);

    const gmkv::FilenamePatterns patterns = m_settings == nullptr ? gmkv::FilenamePatterns() : m_settings->filenamePatterns;
    auto* layout = new QVBoxLayout(this);

    auto* infoGroup = new QGroupBox(loc(QStringLiteral("UI.OptionsForm.Info.Group")), this);
    auto* infoLayout = new QVBoxLayout(infoGroup);
    auto* infoText = new QTextEdit(infoGroup);
    infoText->setReadOnly(true);
    infoText->setMaximumHeight(94);
    infoText->setPlainText(loc(QStringLiteral("UI.OptionsForm.Info.Text")));
    infoLayout->addWidget(infoText);
    layout->addWidget(infoGroup);

    auto* languageGroup = new QGroupBox(loc(QStringLiteral("UI.OptionsForm.Advanced.Group")), this);
    auto* languageLayout = new QFormLayout(languageGroup);
    m_languageCombo = new QComboBox(languageGroup);
    m_languageCombo->setEditable(true);
    m_languageCombo->addItems(gmkv::LocalizationManager::isInitialized()
            ? gmkv::LocalizationManager::availableCultures()
            : QStringList{ QStringLiteral("en") });
    if (m_settings != nullptr) {
        m_languageCombo->setCurrentText(m_settings->culture);
    }
    auto* translationButton = new QPushButton(loc(QStringLiteral("UI.OptionsForm.Advanced.Translations")), languageGroup);
    languageLayout->addRow(loc(QStringLiteral("UI.OptionsForm.Advanced.Culture")), m_languageCombo);
    languageLayout->addRow(translationButton);
    layout->addWidget(languageGroup);

    auto* patternsGroup = new QGroupBox(loc(QStringLiteral("UI.OptionsForm.Patterns.Group")), this);
    auto* patternsLayout = new QFormLayout(patternsGroup);
    m_videoPatternEdit = new QLineEdit(patterns.videoTrackFilenamePattern, patternsGroup);
    m_audioPatternEdit = new QLineEdit(patterns.audioTrackFilenamePattern, patternsGroup);
    m_subtitlePatternEdit = new QLineEdit(patterns.subtitleTrackFilenamePattern, patternsGroup);
    m_chapterPatternEdit = new QLineEdit(patterns.chapterFilenamePattern, patternsGroup);
    m_attachmentPatternEdit = new QLineEdit(patterns.attachmentFilenamePattern, patternsGroup);
    m_tagsPatternEdit = new QLineEdit(patterns.tagsFilenamePattern, patternsGroup);
    addPatternRow(patternsLayout, loc(QStringLiteral("UI.OptionsForm.VideoTracks.Group")), m_videoPatternEdit, gmkv::FilenamePatterns::defaultVideoTrackFilenamePattern(), videoPlaceholders());
    addPatternRow(patternsLayout, loc(QStringLiteral("UI.OptionsForm.AudioTracks.Group")), m_audioPatternEdit, gmkv::FilenamePatterns::defaultAudioTrackFilenamePattern(), audioPlaceholders());
    addPatternRow(patternsLayout, loc(QStringLiteral("UI.OptionsForm.SubtitleTracks.Group")), m_subtitlePatternEdit, gmkv::FilenamePatterns::defaultSubtitleTrackFilenamePattern(), trackPlaceholders());
    addPatternRow(patternsLayout, loc(QStringLiteral("UI.OptionsForm.Chapters.Group")), m_chapterPatternEdit, gmkv::FilenamePatterns::defaultChapterFilenamePattern(), commonPlaceholders());
    addPatternRow(patternsLayout, loc(QStringLiteral("UI.OptionsForm.Attachments.Group")), m_attachmentPatternEdit, gmkv::FilenamePatterns::defaultAttachmentFilenamePattern(), attachmentPlaceholders());
    addPatternRow(patternsLayout, loc(QStringLiteral("UI.OptionsForm.Tags.Group")), m_tagsPatternEdit, gmkv::FilenamePatterns::defaultTagsFilenamePattern(), commonPlaceholders());
    layout->addWidget(patternsGroup, 1);

    auto* extractionGroup = new QGroupBox(loc(QStringLiteral("UI.OptionsForm.Advanced.Group")), this);
    auto* extractionLayout = new QVBoxLayout(extractionGroup);
    m_disableBomCheckBox = new QCheckBox(loc(QStringLiteral("UI.OptionsForm.TextFilesWithoutBom")), extractionGroup);
    m_rawExtractionCheckBox = new QCheckBox(loc(QStringLiteral("UI.OptionsForm.RawMode")), extractionGroup);
    m_fullRawExtractionCheckBox = new QCheckBox(loc(QStringLiteral("UI.OptionsForm.FullRawMode")), extractionGroup);
    if (m_settings != nullptr) {
        m_disableBomCheckBox->setChecked(m_settings->disableBomForTextFiles);
        m_rawExtractionCheckBox->setChecked(m_settings->useRawExtractionMode);
        m_fullRawExtractionCheckBox->setChecked(m_settings->useFullRawExtractionMode);
    }
    extractionLayout->addWidget(m_disableBomCheckBox);
    extractionLayout->addWidget(m_rawExtractionCheckBox);
    extractionLayout->addWidget(m_fullRawExtractionCheckBox);
    layout->addWidget(extractionGroup);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* defaultsButton = buttons->addButton(loc(QStringLiteral("UI.OptionsForm.Defaults")), QDialogButtonBox::ResetRole);
    layout->addWidget(buttons);

    connect(translationButton, &QPushButton::clicked, this, &OptionsDialog::openTranslationEditor);
    connect(defaultsButton, &QPushButton::clicked, this, &OptionsDialog::resetPatternsToDefaults);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        applyToSettings();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void OptionsDialog::addPatternRow(
    QFormLayout* layout,
    const QString& label,
    QLineEdit* edit,
    const QString& defaultPattern,
    const QList<PlaceholderOption>& placeholders)
{
    auto* rowWidget = new QWidget(this);
    auto* rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->addWidget(edit, 1);

    auto* addButton = new QPushButton(loc(QStringLiteral("UI.OptionsForm.VideoTracks.Add")), rowWidget);
    auto* menu = new QMenu(addButton);
    for (const PlaceholderOption& placeholder : placeholders) {
        menu->addAction(placeholder.label, this, [this, edit, placeholder]() {
            appendPlaceholder(edit, placeholder.value);
        });
    }
    addButton->setMenu(menu);
    rowLayout->addWidget(addButton);

    auto* defaultButton = new QPushButton(loc(QStringLiteral("UI.OptionsForm.VideoTracks.Default")), rowWidget);
    connect(defaultButton, &QPushButton::clicked, this, [edit, defaultPattern]() {
        edit->setText(defaultPattern);
        edit->setFocus();
    });
    rowLayout->addWidget(defaultButton);

    layout->addRow(label, rowWidget);
}

void OptionsDialog::appendPlaceholder(QLineEdit* edit, const QString& placeholder)
{
    if (edit == nullptr) {
        return;
    }

    edit->insert(placeholder);
    edit->setFocus();
}

void OptionsDialog::resetPatternsToDefaults()
{
    m_videoPatternEdit->setText(gmkv::FilenamePatterns::defaultVideoTrackFilenamePattern());
    m_audioPatternEdit->setText(gmkv::FilenamePatterns::defaultAudioTrackFilenamePattern());
    m_subtitlePatternEdit->setText(gmkv::FilenamePatterns::defaultSubtitleTrackFilenamePattern());
    m_chapterPatternEdit->setText(gmkv::FilenamePatterns::defaultChapterFilenamePattern());
    m_attachmentPatternEdit->setText(gmkv::FilenamePatterns::defaultAttachmentFilenamePattern());
    m_tagsPatternEdit->setText(gmkv::FilenamePatterns::defaultTagsFilenamePattern());
}

void OptionsDialog::applyToSettings()
{
    if (m_settings == nullptr) {
        return;
    }

    m_settings->culture = m_languageCombo->currentText().trimmed().isEmpty()
        ? QStringLiteral("en")
        : m_languageCombo->currentText().trimmed();
    if (gmkv::LocalizationManager::isInitialized()) {
        gmkv::LocalizationManager::reload(m_settings->culture);
    } else {
        gmkv::LocalizationManager::initialize(QCoreApplication::applicationDirPath(), m_settings->culture);
    }
    m_settings->culture = gmkv::LocalizationManager::currentCulture();
    m_settings->filenamePatterns.videoTrackFilenamePattern = m_videoPatternEdit->text();
    m_settings->filenamePatterns.audioTrackFilenamePattern = m_audioPatternEdit->text();
    m_settings->filenamePatterns.subtitleTrackFilenamePattern = m_subtitlePatternEdit->text();
    m_settings->filenamePatterns.chapterFilenamePattern = m_chapterPatternEdit->text();
    m_settings->filenamePatterns.attachmentFilenamePattern = m_attachmentPatternEdit->text();
    m_settings->filenamePatterns.tagsFilenamePattern = m_tagsPatternEdit->text();
    m_settings->disableBomForTextFiles = m_disableBomCheckBox->isChecked();
    m_settings->useRawExtractionMode = m_rawExtractionCheckBox->isChecked();
    m_settings->useFullRawExtractionMode = m_fullRawExtractionCheckBox->isChecked();
    m_settings->save();
}

void OptionsDialog::openTranslationEditor()
{
    TranslationEditorDialog dialog(this);
    dialog.exec();
}

}
