#include "gmkvextractgui/MainWindow.h"

#include "gmkvextractgui/ExtractionController.h"
#include "gmkvextractgui/JobManagerWindow.h"
#include "gmkvextractgui/LogWindow.h"
#include "gmkvextractgui/OptionsDialog.h"
#include "gmkvextractgui/UiLocalization.h"
#include "gmkvtoolnix/Jobs.h"
#include "gmkvtoolnix/Localization.h"
#include "gmkvtoolnix/Log.h"
#include "gmkvtoolnix/MkvInfo.h"
#include "gmkvtoolnix/MkvToolNix.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QMimeData>
#include <QMap>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QScopeGuard>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTextEdit>
#include <QThread>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <exception>
#include <memory>
#include <numeric>
#include <stdexcept>

namespace gmkv::gui {

namespace {

constexpr int FileIndexRole = Qt::UserRole + 1;
constexpr int SegmentIndexRole = Qt::UserRole + 2;

std::runtime_error runtimeError(const QString& message)
{
    return std::runtime_error(message.toUtf8().constData());
}

QString toolLocationLabelKey()
{
    return gmkv::Platform::isLinux()
        ? QStringLiteral("UI.MainForm2.Config.LinuxToolsLabel")
        : QStringLiteral("UI.MainForm2.Config.WindowsPathLabel");
}

QString toolLocationPlaceholderKey()
{
    return gmkv::Platform::isLinux()
        ? QStringLiteral("UI.MainForm2.Config.LinuxToolsPlaceholder")
        : QStringLiteral("UI.MainForm2.Config.WindowsPathPlaceholder");
}

QString toolDialogTitle()
{
    return gmkv::Platform::isLinux()
        ? loc(QStringLiteral("UI.MainForm2.Config.LinuxToolsDialogTitle"))
        : loc(QStringLiteral("UI.MainForm2.Config.WindowsPathDialogTitle"));
}

QString toolMessageTitle()
{
    return gmkv::Platform::isLinux()
        ? loc(QStringLiteral("UI.MainForm2.Config.LinuxToolsTitle"))
        : loc(QStringLiteral("UI.MainForm2.Config.WindowsToolsTitle"));
}

QString missingToolsMessage()
{
    return gmkv::Platform::isLinux()
        ? loc(QStringLiteral("UI.MainForm2.Errors.MkvCommandsNotFound"))
        : loc(QStringLiteral("UI.MainForm2.Errors.MkvToolNixDirectoryNotFound"));
}

bool isChecked(const QTreeWidgetItem* item)
{
    return item != nullptr && item->checkState(0) == Qt::Checked;
}

bool isMatroskaFile(const QString& filename)
{
    const QString suffix = QFileInfo(filename).suffix().toLower();
    return suffix == QStringLiteral("mkv")
        || suffix == QStringLiteral("mka")
        || suffix == QStringLiteral("mks")
        || suffix == QStringLiteral("mk3d")
        || suffix == QStringLiteral("webm");
}

QStringList matroskaFilesFromUrls(const QList<QUrl>& urls)
{
    QStringList filenames;
    for (const QUrl& url : urls) {
        if (!url.isLocalFile()) {
            continue;
        }

        const QFileInfo info(url.toLocalFile());
        if (info.isDir()) {
            QDirIterator iterator(
                info.absoluteFilePath(),
                { QStringLiteral("*.mkv"), QStringLiteral("*.mka"), QStringLiteral("*.mks"), QStringLiteral("*.mk3d"), QStringLiteral("*.webm") },
                QDir::Files,
                QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                filenames.append(iterator.next());
            }
        } else if (info.isFile() && isMatroskaFile(info.absoluteFilePath())) {
            filenames.append(info.absoluteFilePath());
        }
    }

    filenames.removeDuplicates();
    return filenames;
}

bool containsTrack(const QList<gmkv::SegmentPtr>& segments)
{
    return std::any_of(segments.cbegin(), segments.cend(), [](const gmkv::SegmentPtr& segment) {
        return std::dynamic_pointer_cast<gmkv::Track>(segment) != nullptr;
    });
}

bool modeRequiresTrackSegments(gmkv::FormMkvExtractionMode mode)
{
    switch (mode) {
    case gmkv::FormMkvExtractionMode::CueSheet:
    case gmkv::FormMkvExtractionMode::Tags:
        return false;
    case gmkv::FormMkvExtractionMode::Tracks:
    case gmkv::FormMkvExtractionMode::Timecodes:
    case gmkv::FormMkvExtractionMode::TracksAndTimecodes:
    case gmkv::FormMkvExtractionMode::Cues:
    case gmkv::FormMkvExtractionMode::TracksAndCues:
    case gmkv::FormMkvExtractionMode::TracksAndCuesAndTimecodes:
        return true;
    }
    return true;
}

bool modeRequiresTrackIds(gmkv::FormMkvExtractionMode mode)
{
    switch (mode) {
    case gmkv::FormMkvExtractionMode::Timecodes:
    case gmkv::FormMkvExtractionMode::TracksAndTimecodes:
    case gmkv::FormMkvExtractionMode::Cues:
    case gmkv::FormMkvExtractionMode::TracksAndCues:
    case gmkv::FormMkvExtractionMode::TracksAndCuesAndTimecodes:
        return true;
    case gmkv::FormMkvExtractionMode::Tracks:
    case gmkv::FormMkvExtractionMode::CueSheet:
    case gmkv::FormMkvExtractionMode::Tags:
        return false;
    }
    return false;
}

bool modeUsesRawTrackExtraction(gmkv::FormMkvExtractionMode mode)
{
    switch (mode) {
    case gmkv::FormMkvExtractionMode::Tracks:
    case gmkv::FormMkvExtractionMode::TracksAndTimecodes:
    case gmkv::FormMkvExtractionMode::TracksAndCues:
    case gmkv::FormMkvExtractionMode::TracksAndCuesAndTimecodes:
        return true;
    case gmkv::FormMkvExtractionMode::CueSheet:
    case gmkv::FormMkvExtractionMode::Tags:
    case gmkv::FormMkvExtractionMode::Timecodes:
    case gmkv::FormMkvExtractionMode::Cues:
        return false;
    }
    return false;
}

void applyApplicationTheme(bool darkMode)
{
    if (!darkMode) {
        qApp->setPalette(QPalette());
        qApp->setStyleSheet(QString());
        return;
    }

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(45, 45, 48));
    palette.setColor(QPalette::WindowText, QColor(241, 241, 241));
    palette.setColor(QPalette::Base, QColor(30, 30, 30));
    palette.setColor(QPalette::AlternateBase, QColor(45, 45, 48));
    palette.setColor(QPalette::ToolTipBase, QColor(37, 37, 38));
    palette.setColor(QPalette::ToolTipText, QColor(241, 241, 241));
    palette.setColor(QPalette::Text, QColor(241, 241, 241));
    palette.setColor(QPalette::Button, QColor(63, 63, 70));
    palette.setColor(QPalette::ButtonText, QColor(241, 241, 241));
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Highlight, QColor(0, 122, 204));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(150, 150, 150));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(150, 150, 150));
    qApp->setPalette(palette);
    qApp->setStyleSheet(QStringLiteral(
        "QToolTip { color: #f1f1f1; background-color: #252526; border: 1px solid #5a5a5a; }"
        "QMenu { background-color: #2d2d30; color: #f1f1f1; border: 1px solid #5a5a5a; }"
        "QMenu::item:selected { background-color: #007acc; color: #ffffff; }"));
}

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_settings(QCoreApplication::applicationDirPath())
{
    m_settings.reload();
    gmkv::LocalizationManager::initialize(QCoreApplication::applicationDirPath(), m_settings.culture);
    m_settings.culture = gmkv::LocalizationManager::currentCulture();
    buildUi();
    applySettingsToUi();
    applyLocalization();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("gMKVExtractGUI"));
    setWindowTitleKey(this, QStringLiteral("UI.MainForm2.Title"));
    resize(900, 680);
    setAcceptDrops(true);

    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    auto* configGroup = new QGroupBox(QStringLiteral("Configuration"), centralWidget);
    setTextKey(configGroup, QStringLiteral("UI.MainForm2.Config.Group"));
    auto* configLayout = new QHBoxLayout(configGroup);
    m_mkvToolNixPathEdit = new QLineEdit(configGroup);
    m_mkvToolNixPathEdit->setPlaceholderText(gmkv::Platform::isLinux()
            ? QStringLiteral("Auto-detect from PATH or choose mkvmerge")
            : QStringLiteral("MKVToolNix path"));
    setPlaceholderKey(m_mkvToolNixPathEdit, toolLocationPlaceholderKey());
    auto* toolLocationLabel = new QLabel(gmkv::Platform::isLinux()
            ? QStringLiteral("MKV tools:")
            : QStringLiteral("MKVToolNix Path:"), configGroup);
    setTextKey(toolLocationLabel, toolLocationLabelKey());
    configLayout->addWidget(toolLocationLabel);
    configLayout->addWidget(m_mkvToolNixPathEdit, 1);
    auto* browseToolsButton = new QPushButton(QStringLiteral("Browse..."), configGroup);
    setTextKey(browseToolsButton, QStringLiteral("UI.MainForm2.Config.Browse"));
    auto* autoDetectButton = new QPushButton(QStringLiteral("Auto Detect"), configGroup);
    setTextKey(autoDetectButton, QStringLiteral("UI.MainForm2.Config.AutoDetect"));
    configLayout->addWidget(browseToolsButton);
    configLayout->addWidget(autoDetectButton);
    mainLayout->addWidget(configGroup);

    auto* inputGroup = new QGroupBox(QStringLiteral("Input Files"), centralWidget);
    setTextKey(inputGroup, QStringLiteral("UI.MainForm2.InputFiles.Group"));
    auto* inputLayout = new QVBoxLayout(inputGroup);
    m_inputTree = new QTreeWidget(inputGroup);
    m_inputTree->setHeaderLabels({ QStringLiteral("Element"), QStringLiteral("Type"), QStringLiteral("Details") });
    m_inputTree->header()->setStretchLastSection(true);
    m_inputTree->setAcceptDrops(true);
    m_inputTree->setContextMenuPolicy(Qt::CustomContextMenu);
    inputLayout->addWidget(m_inputTree, 1);

    m_selectedFileInfoGroup = new QGroupBox(QStringLiteral("Selected File Information"), inputGroup);
    auto* selectedFileInfoLayout = new QVBoxLayout(m_selectedFileInfoGroup);
    m_selectedFileInfoText = new QTextEdit(m_selectedFileInfoGroup);
    m_selectedFileInfoText->setReadOnly(true);
    m_selectedFileInfoText->setMinimumHeight(88);
    m_selectedFileInfoText->setMaximumHeight(132);
    selectedFileInfoLayout->addWidget(m_selectedFileInfoText);
    inputLayout->addWidget(m_selectedFileInfoGroup);

    auto* inputOptionsLayout = new QHBoxLayout();
    m_appendOnDragAndDropCheckBox = new QCheckBox(QStringLiteral("Append on Drag and Drop"), inputGroup);
    setTextKey(m_appendOnDragAndDropCheckBox, QStringLiteral("UI.MainForm2.FileOptions.AppendOnDragAndDrop"));
    m_overwriteExistingFilesCheckBox = new QCheckBox(QStringLiteral("Overwrite Existing Files"), inputGroup);
    setTextKey(m_overwriteExistingFilesCheckBox, QStringLiteral("UI.MainForm2.FileOptions.OverwriteExistingFiles"));
    m_disableTooltipsCheckBox = new QCheckBox(QStringLiteral("Disable Tooltips"), inputGroup);
    setTextKey(m_disableTooltipsCheckBox, QStringLiteral("UI.MainForm2.FileOptions.DisableTooltips"));
    inputOptionsLayout->addWidget(m_appendOnDragAndDropCheckBox);
    inputOptionsLayout->addWidget(m_overwriteExistingFilesCheckBox);
    inputOptionsLayout->addWidget(m_disableTooltipsCheckBox);
    inputOptionsLayout->addStretch(1);
    auto* addFilesButton = new QPushButton(QStringLiteral("Add Files..."), inputGroup);
    setTextKey(addFilesButton, QStringLiteral("UI.MainForm2.ContextMenu.AddInputFiles"));
    inputOptionsLayout->addWidget(addFilesButton);
    inputLayout->addLayout(inputOptionsLayout);
    mainLayout->addWidget(inputGroup, 1);

    auto* outputGroup = new QGroupBox(QStringLiteral("Output Directory"), centralWidget);
    setTextKey(outputGroup, QStringLiteral("UI.MainForm2.OutputDirectory.Group"));
    auto* outputLayout = new QHBoxLayout(outputGroup);
    m_outputDirectoryEdit = new QLineEdit(outputGroup);
    m_outputDirectoryEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    outputLayout->addWidget(m_outputDirectoryEdit, 1);
    auto* browseOutputButton = new QPushButton(QStringLiteral("Browse..."), outputGroup);
    setTextKey(browseOutputButton, QStringLiteral("UI.MainForm2.OutputDirectory.Browse"));
    outputLayout->addWidget(browseOutputButton);
    m_useSourceDirectoryCheckBox = new QCheckBox(QStringLiteral("Use Source"), outputGroup);
    setTextKey(m_useSourceDirectoryCheckBox, QStringLiteral("UI.MainForm2.OutputDirectory.UseSource"));
    outputLayout->addWidget(m_useSourceDirectoryCheckBox);
    mainLayout->addWidget(outputGroup);

    auto* actionsGroup = new QGroupBox(QStringLiteral("Actions"), centralWidget);
    setTextKey(actionsGroup, QStringLiteral("UI.MainForm2.Actions.Group"));
    auto* actionsLayout = new QHBoxLayout(actionsGroup);
    auto* logsButton = new QPushButton(QStringLiteral("Logs"), actionsGroup);
    setTextKey(logsButton, QStringLiteral("UI.MainForm2.Actions.Log"));
    auto* jobsButton = new QPushButton(QStringLiteral("Jobs"), actionsGroup);
    setTextKey(jobsButton, QStringLiteral("UI.MainForm2.Actions.ShowJobs"));
    actionsLayout->addWidget(logsButton);
    actionsLayout->addWidget(jobsButton);
    m_popupCheckBox = new QCheckBox(QStringLiteral("Popup"), actionsGroup);
    setTextKey(m_popupCheckBox, QStringLiteral("UI.MainForm2.Actions.Popup"));
    actionsLayout->addWidget(m_popupCheckBox);

    m_chapterTypeCombo = new QComboBox(actionsGroup);
    m_chapterTypeCombo->addItems({ QStringLiteral("XML"), QStringLiteral("OGM"), QStringLiteral("CUE"), QStringLiteral("PBF") });
    auto* chapterTypeLabel = new QLabel(QStringLiteral("Chapter Type:"), actionsGroup);
    setTextKey(chapterTypeLabel, QStringLiteral("UI.MainForm2.Actions.ChapterType"));
    actionsLayout->addWidget(chapterTypeLabel);
    actionsLayout->addWidget(m_chapterTypeCombo);

    m_extractionModeCombo = new QComboBox(actionsGroup);
    m_extractionModeCombo->addItems({
        QStringLiteral("Tracks"),
        QStringLiteral("Cue_Sheet"),
        QStringLiteral("Tags"),
        QStringLiteral("Timecodes"),
        QStringLiteral("Tracks_And_Timecodes"),
        QStringLiteral("Cues"),
        QStringLiteral("Tracks_And_Cues"),
        QStringLiteral("Tracks_And_Cues_And_Timecodes"),
    });
    auto* extractionModeLabel = new QLabel(QStringLiteral("Extraction Mode:"), actionsGroup);
    setTextKey(extractionModeLabel, QStringLiteral("UI.MainForm2.Actions.ExtractionMode"));
    actionsLayout->addWidget(extractionModeLabel);
    actionsLayout->addWidget(m_extractionModeCombo, 1);
    m_addJobsButton = new QPushButton(QStringLiteral("Add Jobs"), actionsGroup);
    setTextKey(m_addJobsButton, QStringLiteral("UI.MainForm2.Actions.AddJobs"));
    m_extractButton = new QPushButton(QStringLiteral("Extract"), actionsGroup);
    setTextKey(m_extractButton, QStringLiteral("UI.MainForm2.Actions.Extract"));
    actionsLayout->addWidget(m_addJobsButton);
    actionsLayout->addWidget(m_extractButton);
    mainLayout->addWidget(actionsGroup);

    auto* footerLayout = new QHBoxLayout();
    m_currentProgress = new QProgressBar(centralWidget);
    m_currentProgress->setRange(0, 100);
    m_totalProgress = new QProgressBar(centralWidget);
    m_totalProgress->setRange(0, 100);
    m_darkModeCheckBox = new QCheckBox(QStringLiteral("Dark Mode"), centralWidget);
    setTextKey(m_darkModeCheckBox, QStringLiteral("UI.MainForm2.Appearance.Dark"));
    footerLayout->addWidget(m_currentProgress, 1);
    footerLayout->addWidget(m_totalProgress, 1);
    footerLayout->addWidget(m_darkModeCheckBox);
    auto* optionsButton = new QPushButton(QStringLiteral("Options"), centralWidget);
    setTextKey(optionsButton, QStringLiteral("UI.MainForm2.Appearance.Options"));
    footerLayout->addWidget(optionsButton);
    m_abortAllButton = new QPushButton(QStringLiteral("Abort All"), centralWidget);
    setTextKey(m_abortAllButton, QStringLiteral("UI.MainForm2.Actions.AbortAll"));
    m_abortButton = new QPushButton(QStringLiteral("Abort"), centralWidget);
    setTextKey(m_abortButton, QStringLiteral("UI.MainForm2.Actions.Abort"));
    footerLayout->addWidget(m_abortAllButton);
    footerLayout->addWidget(m_abortButton);
    mainLayout->addLayout(footerLayout);

    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("Ready"));

    connect(logsButton, &QPushButton::clicked, this, &MainWindow::showLogWindow);
    connect(jobsButton, &QPushButton::clicked, this, &MainWindow::showJobManagerWindow);
    connect(optionsButton, &QPushButton::clicked, this, &MainWindow::showOptionsDialog);
    connect(browseToolsButton, &QPushButton::clicked, this, &MainWindow::browseMkvToolNixPath);
    connect(autoDetectButton, &QPushButton::clicked, this, &MainWindow::autoDetectMkvToolNixPath);
    connect(addFilesButton, &QPushButton::clicked, this, [this]() {
        addInputFiles();
    });
    connect(browseOutputButton, &QPushButton::clicked, this, &MainWindow::browseOutputDirectory);
    connect(m_inputTree, &QTreeWidget::itemChanged, this, &MainWindow::handleTreeItemChanged);
    connect(m_inputTree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem*, QTreeWidgetItem*) {
        updateSelectedFileInformation();
    });
    connect(m_inputTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::showInputTreeContextMenu);
    connect(m_outputDirectoryEdit, &QLineEdit::customContextMenuRequested, this, &MainWindow::showOutputDirectoryContextMenu);
    connect(m_addJobsButton, &QPushButton::clicked, this, &MainWindow::addSelectedJobs);
    connect(m_extractButton, &QPushButton::clicked, this, &MainWindow::extractSelectedJobs);
    connect(m_abortButton, &QPushButton::clicked, this, &MainWindow::abortExtraction);
    connect(m_abortAllButton, &QPushButton::clicked, this, &MainWindow::abortAllExtractions);
    connect(m_darkModeCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_settings.darkMode = checked;
        applyApplicationTheme(checked);
        saveMainSettings();
    });
    connect(m_useSourceDirectoryCheckBox, &QCheckBox::toggled, this, [this, browseOutputButton](bool checked) {
        m_outputDirectoryEdit->setEnabled(!checked);
        browseOutputButton->setEnabled(!checked);
    });

    m_extractionController = new ExtractionController(this);
    connect(m_extractionController, &ExtractionController::started, this, [this](int) {
        setExtractionControlsEnabled(true);
        m_currentProgress->setValue(0);
        m_totalProgress->setValue(0);
        statusBar()->showMessage(QStringLiteral("Starting extraction"));
    });
    connect(m_extractionController, &ExtractionController::progressUpdated, this, [this](int currentProgress, int totalProgress) {
        m_currentProgress->setValue(currentProgress);
        m_totalProgress->setValue(totalProgress);
    });
    connect(m_extractionController, &ExtractionController::trackUpdated, this, [this](const QString& filename, const QString& trackName) {
        statusBar()->showMessage(QStringLiteral("Extracting %1: %2").arg(QFileInfo(filename).fileName(), trackName));
    });
    connect(m_extractionController, &ExtractionController::finished, this, [this](bool aborted, const QStringList& errors) {
        setExtractionControlsEnabled(false);

        if (aborted) {
            statusBar()->showMessage(QStringLiteral("Extraction aborted"), 5000);
            return;
        }

        if (!errors.isEmpty()) {
            statusBar()->showMessage(QStringLiteral("Extraction failed"), 5000);
            QMessageBox::critical(this, QStringLiteral("Extract"), errors.join(QLatin1Char('\n')));
            return;
        }

        statusBar()->showMessage(QStringLiteral("Extraction completed"), 5000);
        if (m_showCompletionPopupForCurrentRun && m_popupCheckBox->isChecked()) {
            QMessageBox::information(this, QStringLiteral("Extract"), QStringLiteral("Extraction completed."));
        }
    });

    setExtractionControlsEnabled(false);
}

void MainWindow::applyLocalization()
{
    gmkv::gui::applyLocalization(this);
    m_inputTree->setHeaderLabels({
        QStringLiteral("Element"),
        QStringLiteral("Type"),
        QStringLiteral("Details"),
    });
    updateSelectedFileInformation();
    if (m_logWindow != nullptr) {
        m_logWindow->applyLocalization();
    }
    if (m_jobManagerWindow != nullptr) {
        m_jobManagerWindow->applyLocalization();
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_analysisThread != nullptr) {
        QMessageBox::information(this, QStringLiteral("Input Files"), QStringLiteral("Input file analysis is still running."));
        event->ignore();
        return;
    }

    if (m_extractionController != nullptr && m_extractionController->isRunning()) {
        const QMessageBox::StandardButton result = QMessageBox::question(
            this,
            QStringLiteral("Extraction"),
            QStringLiteral("Extraction is still running. Abort all extraction jobs and close?"));
        if (result != QMessageBox::Yes) {
            event->ignore();
            return;
        }
        m_extractionController->abortAll();
        event->ignore();
        return;
    }

    saveMainSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData() != nullptr && event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }

    QMainWindow::dragEnterEvent(event);
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if (event->mimeData() == nullptr || !event->mimeData()->hasUrls()) {
        QMainWindow::dropEvent(event);
        return;
    }

    const QStringList filenames = matroskaFilesFromUrls(event->mimeData()->urls());
    if (filenames.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Input Files"), QStringLiteral("No supported Matroska files were dropped."));
        return;
    }

    addInputFiles(filenames, m_appendOnDragAndDropCheckBox->isChecked());
    event->acceptProposedAction();
}

void MainWindow::showLogWindow()
{
    if (m_logWindow == nullptr) {
        m_logWindow = new LogWindow(this);
    }
    m_logWindow->show();
    m_logWindow->raise();
    m_logWindow->activateWindow();
}

void MainWindow::showJobManagerWindow()
{
    if (m_jobManagerWindow == nullptr) {
        m_jobManagerWindow = new JobManagerWindow(this);
    }
    m_jobManagerWindow->show();
    m_jobManagerWindow->raise();
    m_jobManagerWindow->activateWindow();
}

void MainWindow::showOptionsDialog()
{
    OptionsDialog dialog(&m_settings, this);
    if (dialog.exec() == QDialog::Accepted) {
        applySettingsToUi();
        applyLocalization();
    }
}

void MainWindow::browseMkvToolNixPath()
{
    if (gmkv::Platform::isLinux()) {
        const QString filename = QFileDialog::getOpenFileName(
            this,
            toolDialogTitle(),
            m_mkvToolNixPathEdit->text(),
            QStringLiteral("mkvmerge (mkvmerge);;All Files (*)"));
        if (!filename.isEmpty()) {
            m_mkvToolNixPathEdit->setText(QFileInfo(filename).absoluteFilePath());
            saveMainSettings();
        }
        return;
    }

    const QString directory = QFileDialog::getExistingDirectory(this, toolDialogTitle(), m_mkvToolNixPathEdit->text());
    if (!directory.isEmpty()) {
        m_mkvToolNixPathEdit->setText(QDir(directory).absolutePath());
        saveMainSettings();
    }
}

void MainWindow::autoDetectMkvToolNixPath()
{
    gmkv::ToolLocatorInputs inputs;
    inputs.savedPath = m_mkvToolNixPathEdit->text();
    inputs.applicationPath = QCoreApplication::applicationDirPath();
    const QString detected = gmkv::ToolLocator::locate(inputs);
    if (detected.isEmpty()) {
        QMessageBox::warning(this, toolMessageTitle(), missingToolsMessage());
        return;
    }

    m_mkvToolNixPathEdit->setText(detected);
    saveMainSettings();
    statusBar()->showMessage(gmkv::Platform::isLinux()
            ? loc(QStringLiteral("UI.MainForm2.Status.MkvCommandsDetected"))
            : loc(QStringLiteral("UI.MainForm2.Status.MkvToolNixDetected")),
        4000);
}

void MainWindow::browseOutputDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(this, QStringLiteral("Output Directory"), m_outputDirectoryEdit->text());
    if (!directory.isEmpty()) {
        m_outputDirectoryEdit->setText(QDir(directory).absolutePath());
        saveMainSettings();
    }
}

void MainWindow::applySettingsToUi()
{
    m_mkvToolNixPathEdit->setText(m_settings.mkvToolNixPath);
    m_outputDirectoryEdit->setText(m_settings.outputDirectory);
    m_chapterTypeCombo->setCurrentText(gmkv::toSettingsString(m_settings.chapterType));
    m_useSourceDirectoryCheckBox->setChecked(m_settings.lockedOutputDirectory);
    m_popupCheckBox->setChecked(m_settings.showPopup);
    m_appendOnDragAndDropCheckBox->setChecked(m_settings.appendOnDragAndDrop);
    m_overwriteExistingFilesCheckBox->setChecked(m_settings.overwriteExistingFiles);
    m_disableTooltipsCheckBox->setChecked(m_settings.disableTooltips);
    m_darkModeCheckBox->setChecked(m_settings.darkMode);
    applyApplicationTheme(m_settings.darkMode);
    m_outputDirectoryEdit->setEnabled(!m_settings.lockedOutputDirectory);
    if (m_settings.windowSizeWidth > 0 && m_settings.windowSizeHeight > 0) {
        resize(m_settings.windowSizeWidth, m_settings.windowSizeHeight);
    }
    updateSelectedFileInformation();
}

void MainWindow::saveMainSettings()
{
    m_settings.mkvToolNixPath = m_mkvToolNixPathEdit->text();
    m_settings.outputDirectory = m_outputDirectoryEdit->text();
    m_settings.chapterType = gmkv::chapterTypeFromString(m_chapterTypeCombo->currentText(), gmkv::MkvChapterType::Xml);
    m_settings.lockedOutputDirectory = m_useSourceDirectoryCheckBox->isChecked();
    m_settings.showPopup = m_popupCheckBox->isChecked();
    m_settings.appendOnDragAndDrop = m_appendOnDragAndDropCheckBox->isChecked();
    m_settings.overwriteExistingFiles = m_overwriteExistingFilesCheckBox->isChecked();
    m_settings.disableTooltips = m_disableTooltipsCheckBox->isChecked();
    m_settings.darkMode = m_darkModeCheckBox->isChecked();
    m_settings.windowSizeWidth = width();
    m_settings.windowSizeHeight = height();
    m_settings.save();
}

void MainWindow::showOutputDirectoryContextMenu(const QPoint& position)
{
    std::unique_ptr<QMenu> menu(m_outputDirectoryEdit->createStandardContextMenu());
    menu->addSeparator();

    QAction* setDefaultAction = menu->addAction(loc(QStringLiteral("UI.MainForm2.OutputDirectory.SetAsDefault")), this, &MainWindow::setOutputDirectoryAsDefault);
    QAction* useDefaultAction = menu->addAction(
        m_settings.defaultOutputDirectory.trimmed().isEmpty()
            ? loc(QStringLiteral("UI.MainForm2.OutputDirectory.UseDefaultWithValue"), { loc(QStringLiteral("UI.Common.NotSet")) })
            : loc(QStringLiteral("UI.MainForm2.OutputDirectory.UseDefaultWithValue"), { m_settings.defaultOutputDirectory }),
        this,
        &MainWindow::useDefaultOutputDirectory);

    const QString currentDirectory = m_outputDirectoryEdit->text().trimmed();
    const bool currentDirectoryExists = !currentDirectory.isEmpty() && QDir(currentDirectory).exists();
    const bool sameAsDefault = QFileInfo(currentDirectory).absoluteFilePath().compare(
        QFileInfo(m_settings.defaultOutputDirectory).absoluteFilePath(),
        Qt::CaseInsensitive) == 0;
    setDefaultAction->setEnabled(currentDirectoryExists && !sameAsDefault);
    useDefaultAction->setEnabled(!m_useSourceDirectoryCheckBox->isChecked()
        && !m_settings.defaultOutputDirectory.trimmed().isEmpty()
        && QDir(m_settings.defaultOutputDirectory).exists());

    menu->exec(m_outputDirectoryEdit->mapToGlobal(position));
}

void MainWindow::setOutputDirectoryAsDefault()
{
    const QString outputDirectory = m_outputDirectoryEdit->text().trimmed();
    if (outputDirectory.isEmpty() || !QDir(outputDirectory).exists()) {
        QMessageBox::warning(this, QStringLiteral("Output Directory"), QStringLiteral("Select an existing output directory first."));
        return;
    }

    if (!m_settings.defaultOutputDirectory.trimmed().isEmpty()
        && QDir(m_settings.defaultOutputDirectory).exists()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            QStringLiteral("Output Directory"),
            QStringLiteral("Change the default output directory from \"%1\" to \"%2\"?")
                .arg(m_settings.defaultOutputDirectory, outputDirectory));
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    m_settings.defaultOutputDirectory = outputDirectory;
    saveMainSettings();
    statusBar()->showMessage(QStringLiteral("Default output directory updated"), 4000);
}

void MainWindow::useDefaultOutputDirectory()
{
    if (m_useSourceDirectoryCheckBox->isChecked()) {
        return;
    }

    const QString defaultDirectory = m_settings.defaultOutputDirectory.trimmed();
    if (defaultDirectory.isEmpty() || !QDir(defaultDirectory).exists()) {
        QMessageBox::warning(this, QStringLiteral("Output Directory"), QStringLiteral("No valid default output directory is configured."));
        return;
    }

    m_outputDirectoryEdit->setText(defaultDirectory);
    saveMainSettings();
}

void MainWindow::addInputFiles()
{
    const QStringList filenames = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("Add MKV Files"),
        QString(),
        QStringLiteral("Matroska Files (*.mkv *.mka *.mks *.mk3d);;All Files (*)"));

    addInputFiles(filenames, true);
}

void MainWindow::addInputFiles(const QStringList& filenames, bool append)
{
    if (filenames.isEmpty()) {
        return;
    }

    if (m_analysisThread != nullptr) {
        QMessageBox::warning(this, QStringLiteral("Input Files"), QStringLiteral("Input file analysis is already running."));
        return;
    }

    const QString toolLocation = m_mkvToolNixPathEdit->text().trimmed();
    const gmkv::MkvToolPaths toolPaths = gmkv::MkvToolNix::toolPathsFromLocation(toolLocation);
    if (!toolPaths.isValid()) {
        QMessageBox::warning(this, toolMessageTitle(), missingToolsMessage());
        return;
    }

    QStringList filesToAnalyze;
    for (const QString& filename : filenames) {
        const QString absoluteFilename = QFileInfo(filename).absoluteFilePath();
        if (absoluteFilename.isEmpty()) {
            continue;
        }
        if (append) {
            const bool alreadyLoaded = std::any_of(m_loadedFiles.cbegin(), m_loadedFiles.cend(), [&](const LoadedFile& loadedFile) {
                return QFileInfo(loadedFile.filename).absoluteFilePath().compare(absoluteFilename, Qt::CaseInsensitive) == 0;
            });
            if (alreadyLoaded) {
                continue;
            }
        }
        filesToAnalyze.append(absoluteFilename);
    }
    filesToAnalyze.removeDuplicates();

    if (filesToAnalyze.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Input Files"), QStringLiteral("No new Matroska files to add."));
        return;
    }

    if (!append) {
        clearInputFiles();
    }

    m_currentProgress->setValue(0);
    statusBar()->showMessage(QStringLiteral("Analyzing input files"));
    m_analysisThread = QThread::create([this, toolPaths, filesToAnalyze]() {
        QList<AnalyzedFile> results;
        for (qsizetype index = 0; index < filesToAnalyze.size(); ++index) {
            AnalyzedFile result;
            result.filename = filesToAnalyze[index];
            try {
                result.segments = gmkv::SegmentAnalyzer::analyzeFile(toolPaths, result.filename);
            } catch (const std::exception& ex) {
                result.error = QString::fromUtf8(ex.what());
            }
            results.append(result);

            QMetaObject::invokeMethod(this, [this, index, total = filesToAnalyze.size(), filename = result.filename]() {
                m_currentProgress->setValue(static_cast<int>(((index + 1) * 100) / total));
                statusBar()->showMessage(QStringLiteral("Analyzed %1").arg(QFileInfo(filename).fileName()));
            }, Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(this, [this, results]() {
            finishAnalyzedFiles(results);
        }, Qt::QueuedConnection);
    });
    connect(m_analysisThread, &QThread::finished, m_analysisThread, &QObject::deleteLater);
    m_analysisThread->start();
}

void MainWindow::addInputFile(const QString& filename)
{
    addInputFiles({ filename }, true);
}

void MainWindow::appendAnalyzedFile(const QString& filename, const QList<gmkv::SegmentPtr>& segments)
{
    const int fileIndex = m_loadedFiles.size();
    m_loadedFiles.append({ filename, segments });

    auto* fileItem = new QTreeWidgetItem(m_inputTree, {
        QFileInfo(filename).fileName(),
        QStringLiteral("File"),
        filename,
    });
    fileItem->setData(0, FileIndexRole, fileIndex);
    fileItem->setData(0, SegmentIndexRole, -1);
    fileItem->setFlags(fileItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsAutoTristate);
    fileItem->setCheckState(0, Qt::Checked);

    for (qsizetype segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex) {
        const gmkv::SegmentPtr& segment = segments[segmentIndex];
        if (std::dynamic_pointer_cast<gmkv::SegmentInfo>(segment)) {
            continue;
        }
        appendSegmentItem(fileItem, fileIndex, static_cast<int>(segmentIndex), segment);
    }

    fileItem->setExpanded(true);
    if (m_inputTree->currentItem() == nullptr) {
        m_inputTree->setCurrentItem(fileItem);
    }
}

void MainWindow::finishAnalyzedFiles(const QList<AnalyzedFile>& results)
{
    QStringList errors;
    for (const AnalyzedFile& result : results) {
        if (result.error.isEmpty()) {
            appendAnalyzedFile(result.filename, result.segments);
        } else {
            errors.append(QStringLiteral("%1: %2").arg(QFileInfo(result.filename).fileName(), result.error));
        }
    }

    m_analysisThread = nullptr;
    statusBar()->showMessage(QStringLiteral("Ready"), 4000);
    if (!errors.isEmpty()) {
        gmkv::Logger::log(errors.join(QLatin1Char('\n')));
        QMessageBox::warning(this, QStringLiteral("Analyze Files"), errors.join(QLatin1Char('\n')));
    }
}

void MainWindow::clearInputFiles()
{
    m_inputTree->clear();
    m_loadedFiles.clear();
    updateSelectedFileInformation();
}

void MainWindow::appendSegmentItem(QTreeWidgetItem* parent, int fileIndex, int segmentIndex, const gmkv::SegmentPtr& segment)
{
    QString element;
    QString type;
    QString details;

    if (const auto info = std::dynamic_pointer_cast<gmkv::SegmentInfo>(segment)) {
        element = QStringLiteral("Segment information");
        type = QStringLiteral("Info");
        details = QStringLiteral("%1 %2").arg(info->duration, info->writingApplication).trimmed();
    } else if (const auto track = std::dynamic_pointer_cast<gmkv::Track>(segment)) {
        element = QStringLiteral("Track %1").arg(track->trackNumber);
        type = gmkv::toDisplayString(track->trackType);
        details = track->toString();
    } else if (const auto attachment = std::dynamic_pointer_cast<gmkv::Attachment>(segment)) {
        element = QStringLiteral("Attachment %1").arg(attachment->id);
        type = QStringLiteral("Attachment");
        details = attachment->toString();
    } else if (const auto chapter = std::dynamic_pointer_cast<gmkv::Chapter>(segment)) {
        element = QStringLiteral("Chapters");
        type = QStringLiteral("Chapter");
        details = chapter->toString();
    }

    auto* item = new QTreeWidgetItem(parent, { element, type, details });
    item->setData(0, FileIndexRole, fileIndex);
    item->setData(0, SegmentIndexRole, segmentIndex);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, Qt::Checked);
}

void MainWindow::reindexInputTree()
{
    for (int fileIndex = 0; fileIndex < m_inputTree->topLevelItemCount(); ++fileIndex) {
        QTreeWidgetItem* fileItem = m_inputTree->topLevelItem(fileIndex);
        fileItem->setData(0, FileIndexRole, fileIndex);
        for (int childIndex = 0; childIndex < fileItem->childCount(); ++childIndex) {
            fileItem->child(childIndex)->setData(0, FileIndexRole, fileIndex);
        }
    }
}

void MainWindow::handleTreeItemChanged(QTreeWidgetItem* item, int column)
{
    if (m_updatingTreeChecks || item == nullptr || column != 0) {
        return;
    }

    const QSignalBlocker blocker(m_inputTree);
    m_updatingTreeChecks = true;
    const auto resetGuard = qScopeGuard([this]() {
        m_updatingTreeChecks = false;
    });

    if (item->parent() == nullptr) {
        const Qt::CheckState state = item->checkState(0) == Qt::Unchecked ? Qt::Unchecked : Qt::Checked;
        for (int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
            item->child(childIndex)->setCheckState(0, state);
        }
        return;
    }

    QTreeWidgetItem* parent = item->parent();
    int checkedChildren = 0;
    int partiallyCheckedChildren = 0;
    for (int childIndex = 0; childIndex < parent->childCount(); ++childIndex) {
        const Qt::CheckState state = parent->child(childIndex)->checkState(0);
        if (state == Qt::Checked) {
            ++checkedChildren;
        } else if (state == Qt::PartiallyChecked) {
            ++partiallyCheckedChildren;
        }
    }

    if (checkedChildren == parent->childCount()) {
        parent->setCheckState(0, Qt::Checked);
    } else if (checkedChildren == 0 && partiallyCheckedChildren == 0) {
        parent->setCheckState(0, Qt::Unchecked);
    } else {
        parent->setCheckState(0, Qt::PartiallyChecked);
    }
}

void MainWindow::showInputTreeContextMenu(const QPoint& position)
{
    QMenu menu(this);
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.AddInputFiles")), this, [this]() {
        addInputFiles();
    });
    menu.addSeparator();
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.CheckAllTracks"), { QString::number(m_loadedFiles.size()), QString::number(m_loadedFiles.size()) }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::All, true);
    });
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.UncheckAllTracks"), { QString::number(m_loadedFiles.size()), QString::number(m_loadedFiles.size()) }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::All, false);
    });
    menu.addSeparator();
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Video")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Video, true);
    });
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Audio")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Audio, true);
    });
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Subtitle")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Subtitles, true);
    });
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Chapter")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Chapters, true);
    });
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Attachment")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Attachments, true);
    });
    menu.addSeparator();
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.UncheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Video")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Video, false);
    });
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.UncheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Audio")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Audio, false);
    });
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.UncheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Subtitle")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Subtitles, false);
    });
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.UncheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Chapter")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Chapters, false);
    });
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.UncheckTrackGroup"), { loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Attachment")), QStringLiteral("0"), QStringLiteral("0") }), this, [this]() {
        setSegmentChecks(SegmentSelectionType::Attachments, false);
    });
    menu.addSeparator();
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::Language, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::LanguageIetf, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::CodecId, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::ExtraInfo, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::Name, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::Language, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::LanguageIetf, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::CodecId, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::ExtraInfo, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::Name, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::Forced, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::Language, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::LanguageIetf, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::CodecId, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::Name, true);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::Forced, true);
    menu.addSeparator();
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::Language, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::LanguageIetf, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::CodecId, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::ExtraInfo, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Video, TrackFilter::Name, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::Language, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::LanguageIetf, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::CodecId, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::ExtraInfo, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::Name, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Audio, TrackFilter::Forced, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::Language, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::LanguageIetf, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::CodecId, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::Name, false);
    addTrackFilterMenu(&menu, SegmentSelectionType::Subtitles, TrackFilter::Forced, false);
    menu.addSeparator();
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.ExpandAll")), m_inputTree, &QTreeWidget::expandAll);
    menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.CollapseAll")), m_inputTree, &QTreeWidget::collapseAll);
    menu.addSeparator();

    QAction* removeAction = menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.RemoveSelectedInputFile")), this, &MainWindow::removeSelectedFile);
    QAction* removeAllAction = menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.RemoveAllInputFiles"), { QString::number(m_loadedFiles.size()) }), this, [this]() {
        clearInputFiles();
    });
    QAction* openFileAction = menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.OpenSelectedFile")), this, &MainWindow::openSelectedFile);
    QAction* openFolderAction = menu.addAction(loc(QStringLiteral("UI.MainForm2.ContextMenu.OpenSelectedFileFolder")), this, &MainWindow::openSelectedFileFolder);
    const bool hasSelectedFile = selectedFileItem() != nullptr;
    removeAction->setEnabled(hasSelectedFile);
    removeAllAction->setEnabled(!m_loadedFiles.isEmpty());
    openFileAction->setEnabled(hasSelectedFile);
    openFolderAction->setEnabled(hasSelectedFile);

    menu.exec(m_inputTree->viewport()->mapToGlobal(position));
}

void MainWindow::setSegmentChecks(SegmentSelectionType selectionType, bool checked)
{
    const QSignalBlocker blocker(m_inputTree);
    m_updatingTreeChecks = true;
    const auto resetGuard = qScopeGuard([this]() {
        m_updatingTreeChecks = false;
    });

    for (int fileIndex = 0; fileIndex < m_inputTree->topLevelItemCount(); ++fileIndex) {
        QTreeWidgetItem* fileItem = m_inputTree->topLevelItem(fileIndex);
        for (int childIndex = 0; childIndex < fileItem->childCount(); ++childIndex) {
            QTreeWidgetItem* child = fileItem->child(childIndex);
            const int loadedFileIndex = child->data(0, FileIndexRole).toInt();
            const int segmentIndex = child->data(0, SegmentIndexRole).toInt();
            if (loadedFileIndex < 0 || loadedFileIndex >= m_loadedFiles.size()
                || segmentIndex < 0 || segmentIndex >= m_loadedFiles[loadedFileIndex].segments.size()) {
                continue;
            }

            const gmkv::SegmentPtr segment = m_loadedFiles[loadedFileIndex].segments[segmentIndex];
            bool matches = selectionType == SegmentSelectionType::All;
            if (const auto track = std::dynamic_pointer_cast<gmkv::Track>(segment)) {
                matches = matches
                    || (selectionType == SegmentSelectionType::Video && track->trackType == gmkv::MkvTrackType::Video)
                    || (selectionType == SegmentSelectionType::Audio && track->trackType == gmkv::MkvTrackType::Audio)
                    || (selectionType == SegmentSelectionType::Subtitles && track->trackType == gmkv::MkvTrackType::Subtitles);
            } else if (std::dynamic_pointer_cast<gmkv::Chapter>(segment)) {
                matches = matches || selectionType == SegmentSelectionType::Chapters;
            } else if (std::dynamic_pointer_cast<gmkv::Attachment>(segment)) {
                matches = matches || selectionType == SegmentSelectionType::Attachments;
            }

            if (matches) {
                child->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
            }
        }
        updateParentCheckState(fileItem);
    }
}

void MainWindow::addTrackFilterMenu(QMenu* menu, SegmentSelectionType selectionType, TrackFilter filter, bool checked)
{
    struct Counts
    {
        QString display;
        int total = 0;
        int checked = 0;
    };

    QMap<QString, Counts> entries;
    for (int fileIndex = 0; fileIndex < m_inputTree->topLevelItemCount(); ++fileIndex) {
        QTreeWidgetItem* fileItem = m_inputTree->topLevelItem(fileIndex);
        for (int childIndex = 0; childIndex < fileItem->childCount(); ++childIndex) {
            QTreeWidgetItem* child = fileItem->child(childIndex);
            const int loadedFileIndex = child->data(0, FileIndexRole).toInt();
            const int segmentIndex = child->data(0, SegmentIndexRole).toInt();
            if (loadedFileIndex < 0 || loadedFileIndex >= m_loadedFiles.size()
                || segmentIndex < 0 || segmentIndex >= m_loadedFiles[loadedFileIndex].segments.size()) {
                continue;
            }

            const auto track = std::dynamic_pointer_cast<gmkv::Track>(m_loadedFiles[loadedFileIndex].segments[segmentIndex]);
            if (!track) {
                continue;
            }
            const bool typeMatches = (selectionType == SegmentSelectionType::Video && track->trackType == gmkv::MkvTrackType::Video)
                || (selectionType == SegmentSelectionType::Audio && track->trackType == gmkv::MkvTrackType::Audio)
                || (selectionType == SegmentSelectionType::Subtitles && track->trackType == gmkv::MkvTrackType::Subtitles);
            if (!typeMatches) {
                continue;
            }

            const QString value = trackFilterValue(*track, filter);
            Counts& counts = entries[value];
            counts.display = value.trimmed().isEmpty() ? loc(QStringLiteral("UI.Common.NotSet")) : value;
            if (filter == TrackFilter::Forced) {
                counts.display = value == QStringLiteral("true")
                    ? loc(QStringLiteral("UI.Common.True"))
                    : loc(QStringLiteral("UI.Common.False"));
            }
            counts.total++;
            if (isChecked(child)) {
                counts.checked++;
            }
        }
    }

    if (entries.isEmpty() || entries.size() > 50) {
        return;
    }

    QString groupLabel;
    switch (selectionType) {
    case SegmentSelectionType::Video:
        groupLabel = loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Video"));
        break;
    case SegmentSelectionType::Audio:
        groupLabel = loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Audio"));
        break;
    case SegmentSelectionType::Subtitles:
        groupLabel = loc(QStringLiteral("UI.MainForm2.ContextMenu.TrackGroup.Subtitle"));
        break;
    case SegmentSelectionType::All:
    case SegmentSelectionType::Chapters:
    case SegmentSelectionType::Attachments:
        return;
    }

    QString filterLabel;
    switch (filter) {
    case TrackFilter::Language:
        filterLabel = loc(QStringLiteral("UI.MainForm2.ContextMenu.Filter.Language"));
        break;
    case TrackFilter::LanguageIetf:
        filterLabel = loc(QStringLiteral("UI.MainForm2.ContextMenu.Filter.LanguageIetf"));
        break;
    case TrackFilter::CodecId:
        filterLabel = loc(QStringLiteral("UI.MainForm2.ContextMenu.Filter.Codec"));
        break;
    case TrackFilter::ExtraInfo:
        filterLabel = selectionType == SegmentSelectionType::Video
            ? loc(QStringLiteral("UI.MainForm2.ContextMenu.Filter.Resolution"))
            : loc(QStringLiteral("UI.MainForm2.ContextMenu.Filter.Channels"));
        break;
    case TrackFilter::Name:
        filterLabel = loc(QStringLiteral("UI.MainForm2.ContextMenu.Filter.TrackName"));
        break;
    case TrackFilter::Forced:
        filterLabel = loc(QStringLiteral("UI.MainForm2.ContextMenu.Filter.Forced"));
        break;
    }

    const int total = std::accumulate(entries.cbegin(), entries.cend(), 0, [](int sum, const Counts& counts) {
        return sum + counts.total;
    });
    const int checkedCount = std::accumulate(entries.cbegin(), entries.cend(), 0, [](int sum, const Counts& counts) {
        return sum + counts.checked;
    });
    const QString title = loc(
        checked
            ? QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroupByFilter")
            : QStringLiteral("UI.MainForm2.ContextMenu.UncheckTrackGroupByFilter"),
        { groupLabel, filterLabel, QString::number(entries.size()) });

    QMenu* submenu = menu->addMenu(title);
    for (auto it = entries.cbegin(); it != entries.cend(); ++it) {
        const QString value = it.key();
        const Counts counts = it.value();
        submenu->addAction(
            loc(QStringLiteral("UI.MainForm2.ContextMenu.FilterValueCount"), {
                filterLabel,
                counts.display,
                QString::number(checked ? counts.checked : counts.total - counts.checked),
                QString::number(counts.total),
            }),
            this,
            [this, selectionType, filter, value, checked]() {
                setTrackChecksByFilter(selectionType, filter, value, checked);
            });
    }

    submenu->setEnabled(total > 0 && (checked ? checkedCount < total : checkedCount > 0));
}

void MainWindow::setTrackChecksByFilter(SegmentSelectionType selectionType, TrackFilter filter, const QString& value, bool checked)
{
    const QSignalBlocker blocker(m_inputTree);
    m_updatingTreeChecks = true;
    const auto resetGuard = qScopeGuard([this]() {
        m_updatingTreeChecks = false;
    });

    for (int fileIndex = 0; fileIndex < m_inputTree->topLevelItemCount(); ++fileIndex) {
        QTreeWidgetItem* fileItem = m_inputTree->topLevelItem(fileIndex);
        for (int childIndex = 0; childIndex < fileItem->childCount(); ++childIndex) {
            QTreeWidgetItem* child = fileItem->child(childIndex);
            const int loadedFileIndex = child->data(0, FileIndexRole).toInt();
            const int segmentIndex = child->data(0, SegmentIndexRole).toInt();
            if (loadedFileIndex < 0 || loadedFileIndex >= m_loadedFiles.size()
                || segmentIndex < 0 || segmentIndex >= m_loadedFiles[loadedFileIndex].segments.size()) {
                continue;
            }

            const auto track = std::dynamic_pointer_cast<gmkv::Track>(m_loadedFiles[loadedFileIndex].segments[segmentIndex]);
            if (!track) {
                continue;
            }
            const bool typeMatches = (selectionType == SegmentSelectionType::Video && track->trackType == gmkv::MkvTrackType::Video)
                || (selectionType == SegmentSelectionType::Audio && track->trackType == gmkv::MkvTrackType::Audio)
                || (selectionType == SegmentSelectionType::Subtitles && track->trackType == gmkv::MkvTrackType::Subtitles);
            if (typeMatches && trackFilterValue(*track, filter) == value) {
                child->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
            }
        }
        updateParentCheckState(fileItem);
    }
}

QString MainWindow::trackFilterValue(const gmkv::Track& track, TrackFilter filter) const
{
    switch (filter) {
    case TrackFilter::Language:
        return track.language;
    case TrackFilter::LanguageIetf:
        return track.languageIetf;
    case TrackFilter::CodecId:
        return track.codecID;
    case TrackFilter::ExtraInfo:
        return track.extraInfo;
    case TrackFilter::Name:
        return track.trackName;
    case TrackFilter::Forced:
        return track.forced ? QStringLiteral("true") : QStringLiteral("false");
    }

    return {};
}

void MainWindow::updateParentCheckState(QTreeWidgetItem* parent)
{
    if (parent == nullptr) {
        return;
    }

    if (parent->childCount() == 0) {
        parent->setCheckState(0, Qt::Unchecked);
        return;
    }

    int checkedChildren = 0;
    for (int childIndex = 0; childIndex < parent->childCount(); ++childIndex) {
        if (parent->child(childIndex)->checkState(0) == Qt::Checked) {
            ++checkedChildren;
        }
    }

    if (checkedChildren == parent->childCount()) {
        parent->setCheckState(0, Qt::Checked);
    } else if (checkedChildren == 0) {
        parent->setCheckState(0, Qt::Unchecked);
    } else {
        parent->setCheckState(0, Qt::PartiallyChecked);
    }
}

QTreeWidgetItem* MainWindow::selectedFileItem() const
{
    QTreeWidgetItem* item = m_inputTree->currentItem();
    if (item == nullptr) {
        return nullptr;
    }
    return item->parent() == nullptr ? item : item->parent();
}

QString MainWindow::selectedFilePath() const
{
    QTreeWidgetItem* fileItem = selectedFileItem();
    if (fileItem == nullptr) {
        return {};
    }

    const int fileIndex = fileItem->data(0, FileIndexRole).toInt();
    if (fileIndex < 0 || fileIndex >= m_loadedFiles.size()) {
        return {};
    }
    return m_loadedFiles[fileIndex].filename;
}

void MainWindow::removeSelectedFile()
{
    QTreeWidgetItem* fileItem = selectedFileItem();
    if (fileItem == nullptr) {
        return;
    }

    const int fileIndex = fileItem->data(0, FileIndexRole).toInt();
    const int topIndex = m_inputTree->indexOfTopLevelItem(fileItem);
    if (fileIndex >= 0 && fileIndex < m_loadedFiles.size()) {
        m_loadedFiles.removeAt(fileIndex);
    }
    delete m_inputTree->takeTopLevelItem(topIndex);
    reindexInputTree();
    updateSelectedFileInformation();
}

void MainWindow::updateSelectedFileInformation()
{
    if (m_selectedFileInfoGroup == nullptr || m_selectedFileInfoText == nullptr) {
        return;
    }

    const QString filename = selectedFilePath();
    if (filename.isEmpty()) {
        m_selectedFileInfoGroup->setTitle(loc(QStringLiteral("UI.MainForm2.SelectedFileInfo.Group")));
        m_selectedFileInfoText->clear();
        return;
    }

    const int fileIndex = selectedFileItem() == nullptr ? -1 : selectedFileItem()->data(0, FileIndexRole).toInt();
    if (fileIndex < 0 || fileIndex >= m_loadedFiles.size()) {
        m_selectedFileInfoGroup->setTitle(loc(QStringLiteral("UI.MainForm2.SelectedFileInfo.Group")));
        m_selectedFileInfoText->clear();
        return;
    }

    m_selectedFileInfoGroup->setTitle(loc(QStringLiteral("UI.MainForm2.SelectedFileInfo.GroupWithFile"), { QFileInfo(filename).fileName() }));
    if (m_useSourceDirectoryCheckBox != nullptr && m_useSourceDirectoryCheckBox->isChecked()) {
        m_outputDirectoryEdit->setText(QFileInfo(filename).absolutePath());
    }

    QString infoText;
    for (const gmkv::SegmentPtr& segment : m_loadedFiles[fileIndex].segments) {
        const auto info = std::dynamic_pointer_cast<gmkv::SegmentInfo>(segment);
        if (!info) {
            continue;
        }

        infoText = loc(QStringLiteral("UI.MainForm2.SelectedFileInfo.Details"), {
            info->writingApplication,
            info->muxingApplication,
            info->duration,
            info->date,
            QStringLiteral("\n"),
        });
        break;
    }

    m_selectedFileInfoText->setPlainText(infoText);
}

void MainWindow::openSelectedFile()
{
    const QString filename = selectedFilePath();
    if (!filename.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
    }
}

void MainWindow::openSelectedFileFolder()
{
    const QString filename = selectedFilePath();
    if (!filename.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filename).absolutePath()));
    }
}

QList<gmkv::Job> MainWindow::createJobsFromSelection()
{
    saveMainSettings();

    const QString toolLocation = m_mkvToolNixPathEdit->text().trimmed();
    if (!gmkv::MkvToolNix::toolPathsFromLocation(toolLocation).isValid()) {
        throw runtimeError(missingToolsMessage());
    }

    if (m_loadedFiles.isEmpty()) {
        throw runtimeError(QStringLiteral("Add at least one Matroska file first."));
    }

    if (!m_useSourceDirectoryCheckBox->isChecked()) {
        const QString outputDirectory = m_outputDirectoryEdit->text().trimmed();
        if (outputDirectory.isEmpty()) {
            throw runtimeError(QStringLiteral("Select an output directory first."));
        }

        QDir directory(outputDirectory);
        if (!directory.exists()) {
            const QMessageBox::StandardButton answer = QMessageBox::question(
                this,
                QStringLiteral("Output Directory"),
                QStringLiteral("The output directory does not exist. Create it now?"));
            if (answer != QMessageBox::Yes) {
                throw runtimeError(QStringLiteral("Output directory creation was cancelled."));
            }

            if (!QDir().mkpath(outputDirectory)) {
                throw runtimeError(QStringLiteral("Unable to create output directory: %1").arg(outputDirectory));
            }
        }
    }

    QList<gmkv::Job> jobs;
    const gmkv::FormMkvExtractionMode mode = gmkv::extractionModeFromString(m_extractionModeCombo->currentText());
    for (int topIndex = 0; topIndex < m_inputTree->topLevelItemCount(); ++topIndex) {
        QTreeWidgetItem* fileItem = m_inputTree->topLevelItem(topIndex);
        const int fileIndex = fileItem->data(0, FileIndexRole).toInt();
        if (fileIndex < 0 || fileIndex >= m_loadedFiles.size()) {
            continue;
        }

        QList<gmkv::SegmentPtr> selectedSegments;
        for (int childIndex = 0; childIndex < fileItem->childCount(); ++childIndex) {
            QTreeWidgetItem* child = fileItem->child(childIndex);
            if (!isChecked(child)) {
                continue;
            }

            const int segmentIndex = child->data(0, SegmentIndexRole).toInt();
            if (segmentIndex < 0 || segmentIndex >= m_loadedFiles[fileIndex].segments.size()) {
                continue;
            }

            selectedSegments.append(m_loadedFiles[fileIndex].segments[segmentIndex]);
        }

        if (selectedSegments.isEmpty()) {
            continue;
        }

        if (modeRequiresTrackSegments(mode) && selectedSegments.isEmpty()) {
            throw runtimeError(QStringLiteral("Select at least one extractable element for the selected mode."));
        }
        if (modeRequiresTrackIds(mode) && !containsTrack(selectedSegments)) {
            throw runtimeError(QStringLiteral("Select at least one track for timecode or cue extraction."));
        }

        jobs.append(createJob(fileIndex, selectedSegments));
    }

    if (jobs.isEmpty()) {
        throw runtimeError(QStringLiteral("Select at least one extractable element."));
    }

    return jobs;
}

gmkv::Job MainWindow::createJob(int fileIndex, const QList<gmkv::SegmentPtr>& selectedSegments) const
{
    const gmkv::FormMkvExtractionMode mode = gmkv::extractionModeFromString(m_extractionModeCombo->currentText());
    gmkv::Job job;
    job.extractionMode = mode;
    job.mkvToolNixPath = m_mkvToolNixPathEdit->text().trimmed();
    job.parameters.mkvFile = m_loadedFiles[fileIndex].filename;
    job.parameters.outputDirectory = m_useSourceDirectoryCheckBox->isChecked()
        ? QFileInfo(m_loadedFiles[fileIndex].filename).absolutePath()
        : m_outputDirectoryEdit->text().trimmed();
    job.parameters.chapterType = gmkv::chapterTypeFromString(m_chapterTypeCombo->currentText(), gmkv::MkvChapterType::Xml);
    job.parameters.filenamePatterns = m_settings.filenamePatterns;
    job.parameters.overwriteExistingFile = m_overwriteExistingFilesCheckBox->isChecked();
    job.parameters.disableBomForTextFiles = m_settings.disableBomForTextFiles;
    job.parameters.useRawExtractionMode = modeUsesRawTrackExtraction(mode) && m_settings.useRawExtractionMode;
    job.parameters.useFullRawExtractionMode = modeUsesRawTrackExtraction(mode) && m_settings.useFullRawExtractionMode;

    switch (mode) {
    case gmkv::FormMkvExtractionMode::Tracks:
        job.parameters.segmentsToExtract = selectedSegments;
        job.parameters.timecodesExtractionMode = gmkv::TimecodesExtractionMode::NoTimecodes;
        job.parameters.cueExtractionMode = gmkv::CuesExtractionMode::NoCues;
        break;
    case gmkv::FormMkvExtractionMode::CueSheet:
    case gmkv::FormMkvExtractionMode::Tags:
        job.parameters.timecodesExtractionMode = gmkv::TimecodesExtractionMode::NoTimecodes;
        job.parameters.cueExtractionMode = gmkv::CuesExtractionMode::NoCues;
        break;
    case gmkv::FormMkvExtractionMode::Timecodes:
        job.parameters.segmentsToExtract = selectedSegments;
        job.parameters.timecodesExtractionMode = gmkv::TimecodesExtractionMode::OnlyTimecodes;
        job.parameters.cueExtractionMode = gmkv::CuesExtractionMode::NoCues;
        break;
    case gmkv::FormMkvExtractionMode::TracksAndTimecodes:
        job.parameters.segmentsToExtract = selectedSegments;
        job.parameters.timecodesExtractionMode = gmkv::TimecodesExtractionMode::WithTimecodes;
        job.parameters.cueExtractionMode = gmkv::CuesExtractionMode::NoCues;
        break;
    case gmkv::FormMkvExtractionMode::Cues:
        job.parameters.segmentsToExtract = selectedSegments;
        job.parameters.timecodesExtractionMode = gmkv::TimecodesExtractionMode::NoTimecodes;
        job.parameters.cueExtractionMode = gmkv::CuesExtractionMode::OnlyCues;
        break;
    case gmkv::FormMkvExtractionMode::TracksAndCues:
        job.parameters.segmentsToExtract = selectedSegments;
        job.parameters.timecodesExtractionMode = gmkv::TimecodesExtractionMode::NoTimecodes;
        job.parameters.cueExtractionMode = gmkv::CuesExtractionMode::WithCues;
        break;
    case gmkv::FormMkvExtractionMode::TracksAndCuesAndTimecodes:
        job.parameters.segmentsToExtract = selectedSegments;
        job.parameters.timecodesExtractionMode = gmkv::TimecodesExtractionMode::WithTimecodes;
        job.parameters.cueExtractionMode = gmkv::CuesExtractionMode::WithCues;
        break;
    }

    return job;
}

void MainWindow::addSelectedJobs()
{
    try {
        const QList<gmkv::Job> jobs = createJobsFromSelection();
        showJobManagerWindow();

        int addedCount = 0;
        for (const gmkv::Job& job : jobs) {
            gmkv::JobInfo jobInfo;
            jobInfo.job = job;
            if (m_jobManagerWindow->addJob(jobInfo)) {
                ++addedCount;
            }
        }

        statusBar()->showMessage(QStringLiteral("%1 job(s) added").arg(addedCount), 4000);
    } catch (const std::exception& ex) {
        gmkv::Logger::log(QString::fromUtf8(ex.what()));
        QMessageBox::warning(this, QStringLiteral("Add Jobs"), QString::fromUtf8(ex.what()));
    }
}

void MainWindow::extractSelectedJobs()
{
    try {
        runJobs(createJobsFromSelection(), true);
    } catch (const std::exception& ex) {
        gmkv::Logger::log(QString::fromUtf8(ex.what()));
        QMessageBox::warning(this, QStringLiteral("Extract"), QString::fromUtf8(ex.what()));
    }
}

void MainWindow::runJobs(const QList<gmkv::Job>& jobs, bool showCompletionPopup)
{
    if (jobs.isEmpty()) {
        return;
    }

    if (m_analysisThread != nullptr) {
        QMessageBox::warning(this, QStringLiteral("Extract"), QStringLiteral("Wait for input file analysis to finish first."));
        return;
    }

    if (m_extractionController->isRunning()) {
        QMessageBox::warning(this, QStringLiteral("Extract"), QStringLiteral("Extraction is already running."));
        return;
    }

    m_showCompletionPopupForCurrentRun = showCompletionPopup;
    m_extractionController->start(jobs);
}

void MainWindow::setExtractionControlsEnabled(bool extracting)
{
    if (m_addJobsButton != nullptr) {
        m_addJobsButton->setEnabled(!extracting);
    }
    if (m_extractButton != nullptr) {
        m_extractButton->setEnabled(!extracting);
    }
    if (m_abortButton != nullptr) {
        m_abortButton->setEnabled(extracting);
    }
    if (m_abortAllButton != nullptr) {
        m_abortAllButton->setEnabled(extracting);
    }
}

void MainWindow::abortExtraction()
{
    if (m_extractionController != nullptr) {
        statusBar()->showMessage(QStringLiteral("Aborting current extraction"));
        m_extractionController->abortCurrent();
    }
}

void MainWindow::abortAllExtractions()
{
    if (m_extractionController != nullptr) {
        statusBar()->showMessage(QStringLiteral("Aborting all extraction jobs"));
        m_extractionController->abortAll();
    }
}

}
