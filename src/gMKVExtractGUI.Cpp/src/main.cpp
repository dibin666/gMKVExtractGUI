#include "gmkvextractgui/JobManagerWindow.h"
#include "gmkvextractgui/LogWindow.h"
#include "gmkvextractgui/MainWindow.h"
#include "gmkvextractgui/OptionsDialog.h"
#include "gmkvextractgui/TranslationEditorDialog.h"
#include "gmkvtoolnix/MkvInfo.h"
#include "gmkvtoolnix/Segments.h"
#include "gmkvtoolnix/Settings.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QStringList>
#include <QWidget>

#include <memory>

namespace {

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

    for (QWidget* widget : { static_cast<QWidget*>(&translationEditorDialog),
             static_cast<QWidget*>(&jobManagerWindow),
             static_cast<QWidget*>(&logWindow),
             static_cast<QWidget*>(&optionsDialog),
             static_cast<QWidget*>(&mainWindow) }) {
        widget->close();
        QApplication::processEvents();
    }

    return 0;
}

int runAnalysisSmokeTest(const QString& mkvToolNixDirectory, const QString& inputFile)
{
    if (!QFileInfo::exists(inputFile)) {
        return 2;
    }

    QList<gmkv::SegmentPtr> segments;
    try {
        segments = gmkv::SegmentAnalyzer::analyzeFile(mkvToolNixDirectory, inputFile);
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

    return QApplication::exec();
}
