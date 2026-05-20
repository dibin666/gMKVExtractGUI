#include "gmkvextractgui/JobManagerWindow.h"
#include "gmkvextractgui/LogWindow.h"
#include "gmkvextractgui/MainWindow.h"
#include "gmkvextractgui/OptionsDialog.h"
#include "gmkvextractgui/TranslationEditorDialog.h"
#include "gmkvtoolnix/MkvInfo.h"
#include "gmkvtoolnix/Segments.h"
#include "gmkvtoolnix/Settings.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QWidget>

#include <memory>

namespace {

template <typename T>
bool hasAtLeast(QObject* root, qsizetype minimumCount)
{
    return root->findChildren<T*>().size() >= minimumCount;
}

bool smokeWidgetStructureLooksComplete(
    gmkv::gui::MainWindow& mainWindow,
    gmkv::gui::OptionsDialog& optionsDialog,
    gmkv::gui::LogWindow& logWindow,
    gmkv::gui::JobManagerWindow& jobManagerWindow,
    gmkv::gui::TranslationEditorDialog& translationEditorDialog)
{
    return hasAtLeast<QTreeWidget>(&mainWindow, 1)
        && hasAtLeast<QLineEdit>(&mainWindow, 2)
        && hasAtLeast<QComboBox>(&mainWindow, 2)
        && hasAtLeast<QCheckBox>(&mainWindow, 5)
        && hasAtLeast<QPushButton>(&mainWindow, 8)
        && hasAtLeast<QProgressBar>(&mainWindow, 2)
        && hasAtLeast<QTextEdit>(&mainWindow, 1)
        && hasAtLeast<QLineEdit>(&optionsDialog, 6)
        && hasAtLeast<QComboBox>(&optionsDialog, 1)
        && hasAtLeast<QCheckBox>(&optionsDialog, 3)
        && hasAtLeast<QPushButton>(&optionsDialog, 10)
        && hasAtLeast<QTextEdit>(&optionsDialog, 1)
        && hasAtLeast<QTextEdit>(&logWindow, 1)
        && hasAtLeast<QPushButton>(&logWindow, 5)
        && hasAtLeast<QTableWidget>(&jobManagerWindow, 1)
        && hasAtLeast<QProgressBar>(&jobManagerWindow, 2)
        && hasAtLeast<QCheckBox>(&jobManagerWindow, 1)
        && hasAtLeast<QPushButton>(&jobManagerWindow, 6)
        && hasAtLeast<QLabel>(&jobManagerWindow, 3)
        && hasAtLeast<QTableWidget>(&translationEditorDialog, 1)
        && hasAtLeast<QComboBox>(&translationEditorDialog, 1)
        && hasAtLeast<QLineEdit>(&translationEditorDialog, 2)
        && hasAtLeast<QCheckBox>(&translationEditorDialog, 1)
        && hasAtLeast<QPushButton>(&translationEditorDialog, 4)
        && hasAtLeast<QLabel>(&translationEditorDialog, 2);
}

int runSmokeTest(gmkv::gui::MainWindow& mainWindow)
{
    gmkv::Settings smokeSettings(QCoreApplication::applicationDirPath());
    smokeSettings.reload();

    gmkv::gui::OptionsDialog optionsDialog(&smokeSettings, &mainWindow);
    gmkv::gui::LogWindow logWindow(&mainWindow);
    gmkv::gui::JobManagerWindow jobManagerWindow(&mainWindow);
    gmkv::gui::TranslationEditorDialog translationEditorDialog(&mainWindow);

    for (QWidget* widget : { static_cast<QWidget*>(&mainWindow),
             static_cast<QWidget*>(&optionsDialog),
             static_cast<QWidget*>(&logWindow),
             static_cast<QWidget*>(&jobManagerWindow),
             static_cast<QWidget*>(&translationEditorDialog) }) {
        widget->show();
        QApplication::processEvents();
    }

    const bool structureOk = smokeWidgetStructureLooksComplete(
        mainWindow,
        optionsDialog,
        logWindow,
        jobManagerWindow,
        translationEditorDialog);

    for (QWidget* widget : { static_cast<QWidget*>(&translationEditorDialog),
             static_cast<QWidget*>(&jobManagerWindow),
             static_cast<QWidget*>(&logWindow),
             static_cast<QWidget*>(&optionsDialog),
             static_cast<QWidget*>(&mainWindow) }) {
        widget->close();
        QApplication::processEvents();
    }

    return structureOk ? 0 : 5;
}

int runAnalysisSmokeTest(const QString& toolLocation, const QString& inputFile)
{
    if (!QFileInfo::exists(inputFile)) {
        return 2;
    }

    QList<gmkv::SegmentPtr> segments;
    try {
        segments = gmkv::SegmentAnalyzer::analyzeFile(toolLocation, inputFile);
    } catch (...) {
        return 4;
    }
    bool hasVideo = false;
    bool hasAudio = false;
    bool hasSubtitles = false;
    bool hasChapter = false;
    bool hasAttachment = false;

    for (const gmkv::SegmentPtr& segment : segments) {
        if (const auto track = std::dynamic_pointer_cast<gmkv::Track>(segment)) {
            hasVideo = hasVideo || track->trackType == gmkv::MkvTrackType::Video;
            hasAudio = hasAudio || track->trackType == gmkv::MkvTrackType::Audio;
            hasSubtitles = hasSubtitles || track->trackType == gmkv::MkvTrackType::Subtitles;
            continue;
        }

        hasChapter = hasChapter || std::dynamic_pointer_cast<gmkv::Chapter>(segment) != nullptr;
        hasAttachment = hasAttachment || std::dynamic_pointer_cast<gmkv::Attachment>(segment) != nullptr;
    }

    return hasVideo && hasAudio && hasSubtitles && hasChapter && hasAttachment ? 0 : 3;
}

}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("gMKVExtractGUI"));
    QApplication::setOrganizationName(QStringLiteral("gMKVToolNix"));

    const QStringList arguments = QCoreApplication::arguments();
    const qsizetype analysisSmokeIndex = arguments.indexOf(QStringLiteral("--analyze-smoke-test"));
    if (analysisSmokeIndex >= 0) {
        if (analysisSmokeIndex + 2 >= arguments.size()) {
            return 1;
        }
        return runAnalysisSmokeTest(arguments.at(analysisSmokeIndex + 1), arguments.at(analysisSmokeIndex + 2));
    }

    gmkv::gui::MainWindow mainWindow;
    mainWindow.show();

    if (arguments.contains(QStringLiteral("--smoke-test"))) {
        return runSmokeTest(mainWindow);
    }
    if (arguments.contains(QStringLiteral("--selection-smoke-test"))) {
        return mainWindow.runSelectionSmokeTest();
    }

    return QApplication::exec();
}
