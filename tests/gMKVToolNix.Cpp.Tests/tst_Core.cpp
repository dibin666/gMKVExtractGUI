#include "gmkvtoolnix/ExtractionNaming.h"
#include "gmkvtoolnix/FileName.h"
#include "gmkvtoolnix/FilenamePatterns.h"
#include "gmkvtoolnix/Jobs.h"
#include "gmkvtoolnix/Localization.h"
#include "gmkvtoolnix/Log.h"
#include "gmkvtoolnix/MkvExtract.h"
#include "gmkvtoolnix/MkvInfo.h"
#include "gmkvtoolnix/MkvMerge.h"
#include "gmkvtoolnix/MkvToolNix.h"
#include "gmkvtoolnix/OptionValue.h"
#include "gmkvtoolnix/Settings.h"
#include "gmkvtoolnix/Version.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtTest/QtTest>

#include <exception>
#include <memory>

class CoreTests final : public QObject
{
    Q_OBJECT

private slots:
    void versionOutputShouldBeParsedSuccessfully();
    void emptyVersionOutputShouldReturnDefaultVersion();
    void outputFilenameShouldAppendCounterWhenOverwriteIsDisabled();
    void outputFilenameShouldPreserveMultipleExtensionBase();
    void outputFilenameShouldNotAppendCounterWhenOverwriteIsEnabled();
    void filenamePatternsShouldMatchCurrentDefaults();
    void extractionNamingShouldReplaceTrackPlaceholders();
    void extractionNamingShouldCreateTrackOutputFilenames();
    void extractionNamingShouldCreateSpecialOutputFilenames();
    void extractionNamingShouldAppendCounterWhenTargetExists();
    void mkvExtractPlannerShouldPlanGroupedTracksCuesAndTimestamps();
    void mkvExtractPlannerShouldPlanLegacyStdoutModes();
    void mkvExtractRunnerShouldParseProgressAndErrors();
    void mkvExtractRunnerShouldWriteLegacyStdoutToFile();
    void jobsShouldSerializeAndLoadXml();
    void jobQueueShouldRejectDuplicateJobs();
    void optionValueListShouldMatchCurrentFormatting();
    void loggerShouldAppendTimestampedLines();
    void settingsShouldSaveAndReloadLegacyFormat();
    void settingsShouldFallbackForMalformedValues();
    void translationPathServiceShouldNormalizeAliasesAndEnumerateFiles();
    void localizationServiceShouldUseBuiltInEnglishFallback();
    void localizationServiceShouldResolveNeutralAndChineseAliases();
    void localizationServiceShouldFormatPlaceholders();
    void translationMaintenanceShouldCreateTemplateAndSynchronize();
    void mkvToolNixShouldExposePlatformExecutableNamesAndLanguage();
    void mkvToolNixShouldValidateToolDirectory();
    void toolLocatorShouldPreferExplicitAndSavedPaths();
    void toolLocatorShouldFindLinuxToolsOnConfiguredPath();
    void toolLocatorShouldUseLinuxMkvmergeFileAsDisplayLocation();
    void processRunnerShouldCaptureStdoutAndExitCode();
    void mkvInfoParserShouldParseTextOutput();
    void mkvInfoParserShouldCalculateDelaysFromCheckOutput();
    void segmentMergerShouldFillMissingInfoFromMkvInfo();
    void segmentMergerShouldTranslateCodecPrivateData();
    void mkvMergeParserShouldParseJsonIdentification();
    void mkvMergeParserShouldCalculateDelaysFromMinimumTimestamps();

private:
    void writeTranslationFile(const QString& directory, const QString& culture, const QMap<QString, QString>& entries);
};

void writeFakeTool(const QString& directory, gmkv::MkvTool tool)
{
    QFile file(gmkv::MkvToolNix::executablePath(directory, tool));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("echo tool\n");
    file.close();
    const bool permissionsSet = QFile::setPermissions(
        file.fileName(),
        QFileDevice::ReadOwner
            | QFileDevice::WriteOwner
            | QFileDevice::ExeOwner
            | QFileDevice::ReadGroup
            | QFileDevice::ExeGroup
            | QFileDevice::ReadOther
            | QFileDevice::ExeOther);
    if (gmkv::Platform::isLinux()) {
        QVERIFY(permissionsSet);
    }
}

void writeFakeToolSet(const QString& directory)
{
    for (const gmkv::MkvTool tool : { gmkv::MkvTool::Merge, gmkv::MkvTool::Info, gmkv::MkvTool::Extract }) {
        writeFakeTool(directory, tool);
    }
}

void CoreTests::versionOutputShouldBeParsedSuccessfully()
{
    const QList<QPair<QString, gmkv::Version>> cases = {
        { QStringLiteral("mkvmerge v64.0.0 ('The Last Goodbye') 64-bit"), { 64, 0, 0 } },
        { QStringLiteral("mkvmerge v12.5.7 ('Some Name') 32-bit"), { 12, 5, 7 } },
        { QStringLiteral("mkvmerge v1.2.3 ('Test') 64-bit"), { 1, 2, 3 } },
        { QStringLiteral("mkvmerge v86.0 ('Winter') 64-bit"), { 86, 0, 0 } },
        { QStringLiteral("mkvinfo v64.0.0 ('The Last Goodbye') 64-bit"), { 64, 0, 0 } },
        { QStringLiteral("mkvinfo v12.5.7 ('Some Name') 32-bit"), { 12, 5, 7 } },
        { QStringLiteral("mkvextract v64.0.0 ('The Last Goodbye') 64-bit"), { 64, 0, 0 } },
        { QStringLiteral("mkvextract v12.5.7 ('Some Name') 32-bit"), { 12, 5, 7 } },
    };

    for (const auto& testCase : cases) {
        QCOMPARE(gmkv::parseVersionOutput({ testCase.first }), testCase.second);
    }
}

void CoreTests::emptyVersionOutputShouldReturnDefaultVersion()
{
    const gmkv::Version emptyVersion;

    QCOMPARE(gmkv::parseVersionOutput({}), emptyVersion);
    QCOMPARE(gmkv::parseVersionOutput({ QString() }), emptyVersion);
    QCOMPARE(gmkv::parseVersionOutput({ QStringLiteral("") }), emptyVersion);
    QCOMPARE(gmkv::parseVersionOutput({ QStringLiteral(" ") }), emptyVersion);
}

void CoreTests::outputFilenameShouldAppendCounterWhenOverwriteIsDisabled()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filename = tempDir.filePath(QStringLiteral("test.txt"));
    QCOMPARE(gmkv::getOutputFilename(filename, false), filename);

    QFile file(filename);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    const QString firstCounter = tempDir.filePath(QStringLiteral("test.1.txt"));
    QCOMPARE(gmkv::getOutputFilename(filename, false), firstCounter);

    QFile counterFile(firstCounter);
    QVERIFY(counterFile.open(QIODevice::WriteOnly));
    counterFile.close();

    QCOMPARE(gmkv::getOutputFilename(filename, false), tempDir.filePath(QStringLiteral("test.2.txt")));
}

void CoreTests::outputFilenameShouldPreserveMultipleExtensionBase()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filename = tempDir.filePath(QStringLiteral("test.tc.txt"));
    QCOMPARE(gmkv::getOutputFilename(filename, false), filename);

    QFile file(filename);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    const QString firstCounter = tempDir.filePath(QStringLiteral("test.tc.1.txt"));
    QCOMPARE(gmkv::getOutputFilename(filename, false), firstCounter);

    QFile counterFile(firstCounter);
    QVERIFY(counterFile.open(QIODevice::WriteOnly));
    counterFile.close();

    QCOMPARE(gmkv::getOutputFilename(filename, false), tempDir.filePath(QStringLiteral("test.tc.2.txt")));
}

void CoreTests::outputFilenameShouldNotAppendCounterWhenOverwriteIsEnabled()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filename = tempDir.filePath(QStringLiteral("test.txt"));
    QFile file(filename);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();

    QCOMPARE(gmkv::getOutputFilename(filename, true), filename);
}

void CoreTests::filenamePatternsShouldMatchCurrentDefaults()
{
    const gmkv::FilenamePatterns patterns;

    QCOMPARE(patterns.videoTrackFilenamePattern, QStringLiteral("{FilenameNoExt}_track{TrackNumber}_[{Language}]"));
    QCOMPARE(patterns.audioTrackFilenamePattern, QStringLiteral("{FilenameNoExt}_track{TrackNumber}_[{Language}]_DELAY {EffectiveDelay}ms"));
    QCOMPARE(patterns.subtitleTrackFilenamePattern, QStringLiteral("{FilenameNoExt}_track{TrackNumber}_[{Language}]"));
    QCOMPARE(patterns.chapterFilenamePattern, QStringLiteral("{FilenameNoExt}_chapters"));
    QCOMPARE(patterns.attachmentFilenamePattern, QStringLiteral("{AttachmentFilename}"));
    QCOMPARE(patterns.tagsFilenamePattern, QStringLiteral("{FilenameNoExt}_tags"));
}

void CoreTests::extractionNamingShouldReplaceTrackPlaceholders()
{
    gmkv::Track track;
    track.trackNumber = 2;
    track.trackID = 1;
    track.trackType = gmkv::MkvTrackType::Audio;
    track.codecID = QStringLiteral("A_AC3");
    track.codecPrivate = QStringLiteral("AC-3");
    track.forced = true;
    track.language = QStringLiteral("eng");
    track.languageIetf = QStringLiteral("en");
    track.trackName = QStringLiteral("Main:Audio");
    track.delay = 80;
    track.effectiveDelay = -20;
    track.audioSamplingFrequency = 48000;
    track.audioChannels = 6;

    const QString pattern = QStringLiteral(
        "{FilenameNoExt}_{Filename}_{TrackNumber:00}_{TrackID:000}_{TrackName}_{Language}_{LanguageIETF}_"
        "{CodecID}_{CodecPrivate}_{Delay}_{EffectiveDelay}_{TrackForced}_{SamplingFrequency}_{Channels}_{DirSeparator}tail");

    QCOMPARE(
        gmkv::ExtractionNaming::replaceFilenamePlaceholders(track, QStringLiteral("/tmp/Movie.test.mkv"), pattern),
        QStringLiteral("Movie.test_Movie.test.mkv_02_001_Main_Audio_eng_en_A_AC3_AC-3_80_-20_FORCED_48000_6_")
            + QString(QDir::separator()) + QStringLiteral("tail"));
}

void CoreTests::extractionNamingShouldCreateTrackOutputFilenames()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    gmkv::FilenamePatterns patterns;

    gmkv::Track audio;
    audio.trackNumber = 2;
    audio.trackType = gmkv::MkvTrackType::Audio;
    audio.codecID = QStringLiteral("A_AC3");
    audio.language = QStringLiteral("eng");
    audio.effectiveDelay = -20;

    QCOMPARE(
        gmkv::ExtractionNaming::outputFilename(
            audio,
            tempDir.path(),
            QStringLiteral("/input/Movie.mkv"),
            patterns,
            false,
            gmkv::MkvExtractMode::Tracks),
        tempDir.filePath(QStringLiteral("Movie_track2_[eng]_DELAY -20ms.ac3")));

    gmkv::Track subtitles;
    subtitles.trackNumber = 3;
    subtitles.trackType = gmkv::MkvTrackType::Subtitles;
    subtitles.codecID = QStringLiteral("S_TEXT/UTF8");
    subtitles.language = QStringLiteral("spa");

    QCOMPARE(
        gmkv::ExtractionNaming::outputFilename(
            subtitles,
            QString(),
            QStringLiteral("/input/Movie.mkv"),
            patterns,
            false,
            gmkv::MkvExtractMode::Tracks),
        QStringLiteral("Movie_track3_[spa].srt"));
}

void CoreTests::extractionNamingShouldCreateSpecialOutputFilenames()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const gmkv::FilenamePatterns patterns;

    gmkv::Chapter chapter;
    QCOMPARE(
        gmkv::ExtractionNaming::outputFilename(
            chapter,
            tempDir.path(),
            QStringLiteral("/input/Movie.mkv"),
            patterns,
            false,
            gmkv::MkvExtractMode::Chapters,
            gmkv::MkvChapterType::Pbf),
        tempDir.filePath(QStringLiteral("Movie_chapters.pbf")));

    gmkv::Attachment attachment;
    attachment.id = 3;
    attachment.filename = QStringLiteral("cover.jpg");
    attachment.mimeType = QStringLiteral("image/jpeg");
    attachment.fileSize = QStringLiteral("12345");

    QCOMPARE(
        gmkv::ExtractionNaming::outputFilename(
            attachment,
            tempDir.path(),
            QStringLiteral("/input/Movie.mkv"),
            patterns,
            false,
            gmkv::MkvExtractMode::Attachments),
        tempDir.filePath(QStringLiteral("cover.jpg")));

    gmkv::Track track;
    track.trackNumber = 2;
    track.language = QStringLiteral("eng");

    QCOMPARE(
        gmkv::ExtractionNaming::outputFilename(
            track,
            tempDir.path(),
            QStringLiteral("/input/Movie.mkv"),
            patterns,
            false,
            gmkv::MkvExtractMode::TimecodesV2),
        tempDir.filePath(QStringLiteral("Movie_track2_[eng].tc.txt")));
    QCOMPARE(
        gmkv::ExtractionNaming::outputFilename(
            track,
            tempDir.path(),
            QStringLiteral("/input/Movie.mkv"),
            patterns,
            false,
            gmkv::MkvExtractMode::Cues),
        tempDir.filePath(QStringLiteral("Movie_track2_[eng].cue")));
}

void CoreTests::extractionNamingShouldAppendCounterWhenTargetExists()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString existingFilename = tempDir.filePath(QStringLiteral("Movie_track2_[eng]_DELAY -20ms.ac3"));
    QFile existingFile(existingFilename);
    QVERIFY(existingFile.open(QIODevice::WriteOnly));
    existingFile.close();

    gmkv::Track audio;
    audio.trackNumber = 2;
    audio.trackType = gmkv::MkvTrackType::Audio;
    audio.codecID = QStringLiteral("A_AC3");
    audio.language = QStringLiteral("eng");
    audio.effectiveDelay = -20;

    QCOMPARE(
        gmkv::ExtractionNaming::outputFilename(
            audio,
            tempDir.path(),
            QStringLiteral("/input/Movie.mkv"),
            gmkv::FilenamePatterns(),
            false,
            gmkv::MkvExtractMode::Tracks),
        tempDir.filePath(QStringLiteral("Movie_track2_[eng]_DELAY -20ms.1.ac3")));
}

void CoreTests::mkvExtractPlannerShouldPlanGroupedTracksCuesAndTimestamps()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto audio = std::make_shared<gmkv::Track>();
    audio->trackID = 1;
    audio->trackNumber = 2;
    audio->trackType = gmkv::MkvTrackType::Audio;
    audio->codecID = QStringLiteral("A_AC3");
    audio->language = QStringLiteral("eng");
    audio->effectiveDelay = -20;

    auto subtitles = std::make_shared<gmkv::Track>();
    subtitles->trackID = 3;
    subtitles->trackNumber = 4;
    subtitles->trackType = gmkv::MkvTrackType::Subtitles;
    subtitles->codecID = QStringLiteral("S_TEXT/UTF8");
    subtitles->language = QStringLiteral("spa");

    gmkv::ExtractSegmentsParameters parameters;
    parameters.mkvFile = tempDir.filePath(QStringLiteral("Movie.mkv"));
    parameters.outputDirectory = tempDir.path();
    parameters.segmentsToExtract = { audio, subtitles };
    parameters.timecodesExtractionMode = gmkv::TimecodesExtractionMode::WithTimecodes;
    parameters.cueExtractionMode = gmkv::CuesExtractionMode::WithCues;
    parameters.disableBomForTextFiles = true;
    parameters.useRawExtractionMode = true;

    const QList<gmkv::PlannedExtractCommand> commands = gmkv::MkvExtractPlanner::planSegments(parameters, { 96, 0, 0 });
    QCOMPARE(commands.size(), 3);

    const QString language = gmkv::MkvToolNix::uiLanguageCode();
    const QString audioTimecodes = QStringLiteral("1:%1").arg(tempDir.filePath(QStringLiteral("Movie_track2_[eng].tc.txt")));
    const QString subtitleTimecodes = QStringLiteral("3:%1").arg(tempDir.filePath(QStringLiteral("Movie_track4_[spa].tc.txt")));
    const QString audioCues = QStringLiteral("1:%1").arg(tempDir.filePath(QStringLiteral("Movie_track2_[eng].cue")));
    const QString subtitleCues = QStringLiteral("3:%1").arg(tempDir.filePath(QStringLiteral("Movie_track4_[spa].cue")));
    const QString audioTrack = QStringLiteral("1:%1").arg(tempDir.filePath(QStringLiteral("Movie_track2_[eng]_DELAY -20ms.ac3")));
    const QString subtitleTrack = QStringLiteral("3:%1").arg(tempDir.filePath(QStringLiteral("Movie_track4_[spa].srt")));

    QVERIFY(commands[0].parameter.extractMode == gmkv::MkvExtractMode::TimestampsV2);
    QCOMPARE(commands[0].arguments, QStringList({
        parameters.mkvFile,
        QStringLiteral("timestamps_v2"),
        QStringLiteral("--gui-mode"),
        QStringLiteral("--no-bom"),
        QStringLiteral("--ui-language"),
        language,
        audioTimecodes,
        subtitleTimecodes,
    }));

    QVERIFY(commands[1].parameter.extractMode == gmkv::MkvExtractMode::Cues);
    QCOMPARE(commands[1].arguments, QStringList({
        parameters.mkvFile,
        QStringLiteral("cues"),
        QStringLiteral("--gui-mode"),
        QStringLiteral("--no-bom"),
        QStringLiteral("--ui-language"),
        language,
        audioCues,
        subtitleCues,
    }));

    QVERIFY(commands[2].parameter.extractMode == gmkv::MkvExtractMode::Tracks);
    QCOMPARE(commands[2].arguments, QStringList({
        parameters.mkvFile,
        QStringLiteral("tracks"),
        QStringLiteral("--gui-mode"),
        QStringLiteral("--no-bom"),
        QStringLiteral("--ui-language"),
        language,
        QStringLiteral("--raw"),
        audioTrack,
        subtitleTrack,
    }));
}

void CoreTests::mkvExtractPlannerShouldPlanLegacyStdoutModes()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto chapter = std::make_shared<gmkv::Chapter>();

    gmkv::ExtractSegmentsParameters parameters;
    parameters.mkvFile = tempDir.filePath(QStringLiteral("Movie.mkv"));
    parameters.outputDirectory = tempDir.path();
    parameters.segmentsToExtract = { chapter };
    parameters.chapterType = gmkv::MkvChapterType::Ogm;
    parameters.disableBomForTextFiles = true;

    const QString language = gmkv::MkvToolNix::uiLanguageCode();
    const QList<gmkv::PlannedExtractCommand> chapterCommands = gmkv::MkvExtractPlanner::planSegments(parameters, { 16, 0, 0 });
    QCOMPARE(chapterCommands.size(), 1);
    QVERIFY(chapterCommands[0].parameter.extractMode == gmkv::MkvExtractMode::Chapters);
    QVERIFY(chapterCommands[0].parameter.writeOutputToFile);
    QCOMPARE(chapterCommands[0].parameter.outputFilename, tempDir.filePath(QStringLiteral("Movie_chapters.txt")));
    QCOMPARE(chapterCommands[0].arguments, QStringList({
        QStringLiteral("chapters"),
        parameters.mkvFile,
        QStringLiteral("--gui-mode"),
        QStringLiteral("--ui-language"),
        language,
        QStringLiteral("--simple"),
    }));

    const gmkv::PlannedExtractCommand tagsCommand = gmkv::MkvExtractPlanner::planTags(parameters, { 16, 0, 0 });
    QVERIFY(tagsCommand.parameter.writeOutputToFile);
    QCOMPARE(tagsCommand.parameter.outputFilename, tempDir.filePath(QStringLiteral("Movie_tags.xml")));
    QCOMPARE(tagsCommand.arguments, QStringList({
        QStringLiteral("tags"),
        parameters.mkvFile,
        QStringLiteral("--gui-mode"),
        QStringLiteral("--ui-language"),
        language,
    }));

    const gmkv::PlannedExtractCommand cueSheetCommand = gmkv::MkvExtractPlanner::planCueSheet(parameters, { 17, 0, 0 });
    QVERIFY(!cueSheetCommand.parameter.writeOutputToFile);
    QCOMPARE(cueSheetCommand.arguments, QStringList({
        parameters.mkvFile,
        QStringLiteral("cuesheet"),
        QStringLiteral("--gui-mode"),
        QStringLiteral("--ui-language"),
        language,
        tempDir.filePath(QStringLiteral("Movie_cuesheet.cue")),
    }));
}

void CoreTests::mkvExtractRunnerShouldParseProgressAndErrors()
{
    bool ok = false;
    QCOMPARE(gmkv::MkvExtractRunner::progressFromLine(QStringLiteral("#GUI#progress 42%"), &ok), 42);
    QVERIFY(ok);

    QCOMPARE(gmkv::MkvExtractRunner::progressFromLine(QStringLiteral("Progress: 7%"), &ok), 7);
    QVERIFY(ok);

    QCOMPARE(gmkv::MkvExtractRunner::progressFromLine(QStringLiteral("plain output"), &ok), 0);
    QVERIFY(!ok);

    QCOMPARE(gmkv::MkvExtractRunner::errorFromLine(QStringLiteral("#GUI#error failed badly"), &ok), QStringLiteral("failed badly"));
    QVERIFY(ok);

    QCOMPARE(gmkv::MkvExtractRunner::errorFromLine(QStringLiteral("Error: failed too"), &ok), QStringLiteral("failed too"));
    QVERIFY(ok);
}

void CoreTests::mkvExtractRunnerShouldWriteLegacyStdoutToFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString program = gmkv::Platform::isLinux() ? QStringLiteral("/bin/sh") : QStringLiteral("cmd");
    const QStringList arguments = gmkv::Platform::isLinux()
        ? QStringList{ QStringLiteral("-c"), QStringLiteral("printf 'legacy xml\\n'") }
        : QStringList{ QStringLiteral("/C"), QStringLiteral("echo legacy xml") };

    gmkv::PlannedExtractCommand command;
    command.sourceFile = QStringLiteral("Movie.mkv");
    command.arguments = arguments;
    command.parameter.extractMode = gmkv::MkvExtractMode::Chapters;
    command.parameter.writeOutputToFile = true;
    command.parameter.outputFilename = tempDir.filePath(QStringLiteral("chapters.xml"));

    gmkv::MkvExtractRunner runner(program);
    QSignalSpy progressSpy(&runner, &gmkv::MkvExtractRunner::progressUpdated);
    QSignalSpy trackSpy(&runner, &gmkv::MkvExtractRunner::trackUpdated);

    const gmkv::MkvExtractRunResult result = runner.runCommand(command);
    QVERIFY(result.succeeded());
    QCOMPARE(progressSpy.size(), 1);
    QCOMPARE(trackSpy.size(), 1);
    const QList<QVariant> trackSignal = trackSpy.takeFirst();
    QCOMPARE(trackSignal.at(0).toString(), QStringLiteral("Movie.mkv"));
    QCOMPARE(trackSignal.at(1).toString(), QStringLiteral("chapters"));

    QFile outputFile(command.parameter.outputFilename);
    QVERIFY(outputFile.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(outputFile.readAll()), QStringLiteral("legacy xml\n"));
}

void CoreTests::jobsShouldSerializeAndLoadXml()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    auto audio = std::make_shared<gmkv::Track>();
    audio->trackID = 1;
    audio->trackNumber = 2;
    audio->trackType = gmkv::MkvTrackType::Audio;
    audio->codecID = QStringLiteral("A_AC3");
    audio->language = QStringLiteral("eng");
    audio->effectiveDelay = -20;

    auto chapter = std::make_shared<gmkv::Chapter>();
    chapter->chapterCount = 4;

    gmkv::JobInfo jobInfo;
    jobInfo.job.extractionMode = gmkv::FormMkvExtractionMode::TracksAndCues;
    jobInfo.job.mkvToolNixPath = QStringLiteral("/opt/mkvtoolnix");
    jobInfo.job.parameters.mkvFile = tempDir.filePath(QStringLiteral("Movie.mkv"));
    jobInfo.job.parameters.outputDirectory = tempDir.path();
    jobInfo.job.parameters.chapterType = gmkv::MkvChapterType::Cue;
    jobInfo.job.parameters.cueExtractionMode = gmkv::CuesExtractionMode::WithCues;
    jobInfo.job.parameters.segmentsToExtract = { audio, chapter };
    jobInfo.job.parameters.filenamePatterns.audioTrackFilenamePattern = QStringLiteral("audio-{TrackID}");
    jobInfo.state = gmkv::JobState::Pending;
    jobInfo.startTime = QDateTime(QDate(2026, 5, 20), QTime(12, 0), Qt::UTC);

    const QString filename = tempDir.filePath(QStringLiteral("jobs.xml"));
    gmkv::JobXmlService::save({ jobInfo }, filename);

    QFile savedFile(filename);
    QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString savedXml = QString::fromUtf8(savedFile.readAll());
    QVERIFY(savedXml.contains(QStringLiteral("ArrayOfGMKVJobInfo")));
    QVERIFY(savedXml.contains(QStringLiteral("<ExtractionMode>Tracks_And_Cues</ExtractionMode>")));
    savedFile.close();

    QList<gmkv::JobInfo> loaded;
    try {
        loaded = gmkv::JobXmlService::load(filename);
    } catch (const std::exception& ex) {
        QFAIL(qPrintable(QStringLiteral("%1\n%2").arg(QString::fromUtf8(ex.what()), savedXml)));
    }
    QCOMPARE(loaded.size(), 1);
    QVERIFY(loaded[0].job.extractionMode == gmkv::FormMkvExtractionMode::TracksAndCues);
    QVERIFY(loaded[0].state == gmkv::JobState::Pending);
    QCOMPARE(loaded[0].job.mkvToolNixPath, QStringLiteral("/opt/mkvtoolnix"));
    QCOMPARE(loaded[0].job.parameters.outputDirectory, tempDir.path());
    QCOMPARE(loaded[0].job.parameters.filenamePatterns.audioTrackFilenamePattern, QStringLiteral("audio-{TrackID}"));
    QCOMPARE(loaded[0].job.parameters.segmentsToExtract.size(), 2);

    const auto loadedTrack = std::dynamic_pointer_cast<gmkv::Track>(loaded[0].job.parameters.segmentsToExtract[0]);
    QVERIFY(loadedTrack != nullptr);
    QCOMPARE(loadedTrack->trackID, 1);
    QCOMPARE(loadedTrack->effectiveDelay, -20);

    QVERIFY(loaded[0].job.toString().contains(QStringLiteral("Tracks/Cues")));
}

void CoreTests::jobQueueShouldRejectDuplicateJobs()
{
    auto audio = std::make_shared<gmkv::Track>();
    audio->trackID = 1;
    audio->trackNumber = 2;
    audio->trackType = gmkv::MkvTrackType::Audio;
    audio->codecID = QStringLiteral("A_AC3");
    audio->language = QStringLiteral("eng");

    auto attachment = std::make_shared<gmkv::Attachment>();
    attachment->id = 3;
    attachment->filename = QStringLiteral("cover.jpg");

    gmkv::JobInfo first;
    first.job.extractionMode = gmkv::FormMkvExtractionMode::Tracks;
    first.job.mkvToolNixPath = QStringLiteral("/opt/mkvtoolnix");
    first.job.parameters.mkvFile = QStringLiteral("/input/Movie.mkv");
    first.job.parameters.outputDirectory = QStringLiteral("/out");
    first.job.parameters.segmentsToExtract = { audio, attachment };

    gmkv::JobInfo duplicate = first;
    duplicate.job.parameters.segmentsToExtract = { attachment, audio };

    gmkv::JobQueue queue;
    QVERIFY(queue.addJob(first));
    QVERIFY(!queue.addJob(duplicate));
    QCOMPARE(queue.jobs().size(), 1);

    duplicate.job.parameters.outputDirectory = QStringLiteral("/other");
    QVERIFY(queue.addJob(duplicate));
    QCOMPARE(queue.jobs().size(), 2);
}

void CoreTests::optionValueListShouldMatchCurrentFormatting()
{
    const QList<gmkv::OptionValue<gmkv::MkvExtractGlobalOption>> options = {
        { gmkv::MkvExtractGlobalOption::Version, QString() },
        { gmkv::MkvExtractGlobalOption::UiLanguage, QStringLiteral("en_US") },
    };

    QCOMPARE(gmkv::convertOptionValueListToString(options), QStringLiteral(" --version --ui-language en_US"));
}

void CoreTests::loggerShouldAppendTimestampedLines()
{
    gmkv::Logger::clear();
    gmkv::Logger::log(QStringLiteral("Hello"));

    const QString logText = gmkv::Logger::logText();
    QVERIFY(logText.contains(QStringLiteral("Hello")));
    QVERIFY(QRegularExpression(QStringLiteral(R"(\[\d{4}-\d{2}-\d{2}\]\[\d{2}:\d{2}:\d{2}\] Hello\n)")).match(logText).hasMatch());
}

void CoreTests::settingsShouldSaveAndReloadLegacyFormat()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    gmkv::Settings settings(tempDir.path());
    settings.mkvToolNixPath = QStringLiteral("/opt/mkvtoolnix");
    settings.chapterType = gmkv::MkvChapterType::Cue;
    settings.outputDirectory = QStringLiteral("/tmp/out");
    settings.defaultOutputDirectory = QStringLiteral("/tmp/default");
    settings.lockedOutputDirectory = true;
    settings.windowPosX = 10;
    settings.windowPosY = 20;
    settings.windowSizeWidth = 1024;
    settings.windowSizeHeight = 768;
    settings.jobMode = true;
    settings.windowState = gmkv::WindowState::Maximized;
    settings.showPopup = false;
    settings.showPopupInJobManager = false;
    settings.appendOnDragAndDrop = true;
    settings.overwriteExistingFiles = true;
    settings.disableTooltips = true;
    settings.darkMode = true;
    settings.disableBomForTextFiles = true;
    settings.useRawExtractionMode = true;
    settings.useFullRawExtractionMode = true;
    settings.culture = QStringLiteral("zh-cn");
    settings.filenamePatterns.videoTrackFilenamePattern = QStringLiteral("video");
    settings.save();

    QFile savedFile(settings.settingsFilePath());
    QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString savedText = QString::fromUtf8(savedFile.readAll());
    QVERIFY(savedText.contains(QStringLiteral("MKVToolnix Path:/opt/mkvtoolnix\n")));
    QVERIFY(savedText.contains(QStringLiteral("Chapter Type:CUE\n")));
    QVERIFY(savedText.contains(QStringLiteral("Window State:Maximized\n")));
    QVERIFY(savedText.contains(QStringLiteral("DarkMode:True\n")));

    gmkv::Settings loaded(tempDir.path());
    loaded.reload();

    QCOMPARE(loaded.mkvToolNixPath, QStringLiteral("/opt/mkvtoolnix"));
    QCOMPARE(loaded.chapterType, gmkv::MkvChapterType::Cue);
    QCOMPARE(loaded.windowSizeWidth, 1024);
    QCOMPARE(loaded.windowSizeHeight, 768);
    QCOMPARE(loaded.windowState, gmkv::WindowState::Maximized);
    QVERIFY(loaded.darkMode);
    QVERIFY(loaded.useFullRawExtractionMode);
    QCOMPARE(loaded.culture, QStringLiteral("zh-cn"));
    QCOMPARE(loaded.filenamePatterns.videoTrackFilenamePattern, QStringLiteral("video"));
}

void CoreTests::settingsShouldFallbackForMalformedValues()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile file(QDir(tempDir.path()).filePath(gmkv::Settings::settingsFileName()));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "Chapter Type:not-a-type\n";
    stream << "Initial Window Size Width:not-an-int\n";
    stream << "Show Popup:not-bool\n";
    stream << "Culture:   \n";
    file.close();

    gmkv::Settings settings(tempDir.path());
    settings.reload();

    QCOMPARE(settings.chapterType, gmkv::MkvChapterType::Xml);
    QCOMPARE(settings.windowSizeWidth, 640);
    QVERIFY(settings.showPopup);
    QCOMPARE(settings.culture, QStringLiteral("en"));
}

void CoreTests::translationPathServiceShouldNormalizeAliasesAndEnumerateFiles()
{
    QCOMPARE(gmkv::TranslationPathService::translationFileName(QStringLiteral("de")), QStringLiteral("gmkvextract-de.json"));
    QCOMPARE(gmkv::TranslationPathService::canonicalCultureCode(QStringLiteral("ZH_HANS")), QStringLiteral("zh-cn"));
    QCOMPARE(gmkv::TranslationPathService::canonicalCultureCode(QStringLiteral("zh-Hant")), QStringLiteral("zh-tw"));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QFile(QDir(tempDir.path()).filePath(QStringLiteral("gmkvextract-en.json"))).open(QIODevice::WriteOnly);
    QFile(QDir(tempDir.path()).filePath(QStringLiteral("gmkvextract-de.json"))).open(QIODevice::WriteOnly);
    QFile(QDir(tempDir.path()).filePath(QStringLiteral("en.json"))).open(QIODevice::WriteOnly);

    const QStringList files = gmkv::TranslationPathService::enumerateTranslationFiles(tempDir.path());
    QCOMPARE(QFileInfo(files.value(0)).fileName(), QStringLiteral("gmkvextract-de.json"));
    QCOMPARE(QFileInfo(files.value(1)).fileName(), QStringLiteral("gmkvextract-en.json"));
}

void CoreTests::localizationServiceShouldUseBuiltInEnglishFallback()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const gmkv::JsonLocalizationService service(tempDir.path());

    QVERIFY(service.availableCultures().contains(QStringLiteral("en")));
    QCOMPARE(service.getStringForCulture(QStringLiteral("UI.Common.Dialog.AreYouSureTitle"), QStringLiteral("ja")), QStringLiteral("Are you sure?"));

    const gmkv::TranslationFile builtIn = gmkv::JsonLocalizationService::createBuiltInEnglishTranslationFile();
    QCOMPARE(builtIn.metadata.culture, QStringLiteral("en"));
    QCOMPARE(builtIn.entries[QStringLiteral("UI.Common.Dialog.AreYouSureTitle")].translation, QStringLiteral("Are you sure?"));
    QVERIFY(builtIn.entries[QStringLiteral("UI.Common.Dialog.AreYouSureTitle")].isTranslated);
}

void CoreTests::localizationServiceShouldResolveNeutralAndChineseAliases()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    writeTranslationFile(tempDir.path(), QStringLiteral("de"), {
        { QStringLiteral("UI.Common.Dialog.AreYouSureTitle"), QStringLiteral("Sind Sie sicher?") },
    });
    writeTranslationFile(tempDir.path(), QStringLiteral("zh-cn"), {
        { QStringLiteral("UI.Common.Dialog.AreYouSureTitle"), QStringLiteral("Are you sure zh-cn?") },
    });
    writeTranslationFile(tempDir.path(), QStringLiteral("zh-tw"), {
        { QStringLiteral("UI.Common.Dialog.AreYouSureTitle"), QStringLiteral("Are you sure zh-tw?") },
    });

    const gmkv::JsonLocalizationService service(tempDir.path());

    QCOMPARE(service.resolveCultureName(QStringLiteral("de-DE")), QStringLiteral("de"));
    QCOMPARE(service.resolveCultureName(QStringLiteral("cn")), QStringLiteral("zh-tw"));
    QCOMPARE(service.resolveCultureName(QStringLiteral("zh-Hans")), QStringLiteral("zh-cn"));
    QCOMPARE(service.resolveCultureName(QStringLiteral("zh-Hant")), QStringLiteral("zh-tw"));
}

void CoreTests::localizationServiceShouldFormatPlaceholders()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const gmkv::JsonLocalizationService service(tempDir.path());

    QCOMPARE(
        service.getStringForCulture(
            QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroup"),
            QStringLiteral("en"),
            { QStringLiteral("Video Tracks"), QStringLiteral("1"), QStringLiteral("2") }),
        QStringLiteral("Check Video Tracks... (1/2)"));

    QCOMPARE(
        service.getStringForCulture(
            QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroup"),
            QStringLiteral("en"),
            { QStringLiteral("Video Tracks") }),
        QStringLiteral("!BadFormat:UI.MainForm2.ContextMenu.CheckTrackGroup!"));
}

void CoreTests::translationMaintenanceShouldCreateTemplateAndSynchronize()
{
    gmkv::TranslationFile master;
    master.metadata.culture = QStringLiteral("en");
    master.entries[QStringLiteral("Existing.Same")] = { QStringLiteral("Source 1"), QStringLiteral("Source 1"), true, QStringLiteral("Note 1") };
    master.entries[QStringLiteral("Existing.Changed")] = { QStringLiteral("Updated Source"), QStringLiteral("Updated Source"), true, QStringLiteral("Note 2") };
    master.entries[QStringLiteral("New.Key")] = { QStringLiteral("New Source"), QStringLiteral("New Source"), true, QStringLiteral("Note 3") };

    const gmkv::TranslationFile templ = gmkv::TranslationMaintenanceService::createTemplate(master, QStringLiteral("it"), QStringLiteral("Translator"));
    QCOMPARE(templ.metadata.culture, QStringLiteral("it"));
    QCOMPARE(templ.metadata.translator, QStringLiteral("Translator"));
    QVERIFY(!templ.entries[QStringLiteral("Existing.Same")].isTranslated);

    gmkv::TranslationFile target;
    target.metadata.culture = QStringLiteral("fr");
    target.entries[QStringLiteral("Existing.Same")] = { QStringLiteral("Source 1"), QStringLiteral("Traduction"), true, QStringLiteral("Old note") };
    target.entries[QStringLiteral("Existing.Changed")] = { QStringLiteral("Old Source"), QStringLiteral("Ancienne traduction"), true, QStringLiteral("Old note 2") };
    target.entries[QStringLiteral("Removed.Key")] = { QStringLiteral("Removed"), QStringLiteral("Supprime"), true, QStringLiteral("Old note 3") };

    const gmkv::TranslationSyncResult result = gmkv::TranslationMaintenanceService::synchronize(master, target);

    QCOMPARE(result.addedCount, 1);
    QCOMPARE(result.updatedCount, 1);
    QCOMPARE(result.removedCount, 1);
    QCOMPARE(result.translationFile.entries.size(), 3);
    QVERIFY(!result.translationFile.entries.contains(QStringLiteral("Removed.Key")));
    QCOMPARE(result.translationFile.entries[QStringLiteral("Existing.Same")].translation, QStringLiteral("Traduction"));
    QCOMPARE(result.translationFile.entries[QStringLiteral("Existing.Same")].notes, QStringLiteral("Note 1"));
    QCOMPARE(result.translationFile.entries[QStringLiteral("Existing.Changed")].translation, QStringLiteral("Updated Source"));
    QVERIFY(!result.translationFile.entries[QStringLiteral("Existing.Changed")].isTranslated);
}

void CoreTests::mkvToolNixShouldExposePlatformExecutableNamesAndLanguage()
{
    if (gmkv::Platform::isLinux()) {
        QCOMPARE(gmkv::MkvToolNix::executableName(gmkv::MkvTool::Merge), QStringLiteral("mkvmerge"));
        QCOMPARE(gmkv::MkvToolNix::executableName(gmkv::MkvTool::Info), QStringLiteral("mkvinfo"));
        QCOMPARE(gmkv::MkvToolNix::executableName(gmkv::MkvTool::Extract), QStringLiteral("mkvextract"));
        QCOMPARE(gmkv::MkvToolNix::uiLanguageCode(), QStringLiteral("en_US"));
    } else {
        QCOMPARE(gmkv::MkvToolNix::executableName(gmkv::MkvTool::Merge), QStringLiteral("mkvmerge.exe"));
        QCOMPARE(gmkv::MkvToolNix::uiLanguageCode(), QStringLiteral("en"));
    }

    QCOMPARE(gmkv::MkvToolNix::unescapeString(QStringLiteral(R"(name\swith\2quote\ccolon\hhash\bopen\Bclose)")),
             QStringLiteral("name with\"quote:colon#hash[open]close"));
}

void CoreTests::mkvToolNixShouldValidateToolDirectory()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QVERIFY(!gmkv::MkvToolNix::isToolDirectory(tempDir.path()));

    for (const gmkv::MkvTool tool : { gmkv::MkvTool::Merge, gmkv::MkvTool::Info, gmkv::MkvTool::Extract }) {
        QFile file(gmkv::MkvToolNix::executablePath(tempDir.path(), tool));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
    }

    QVERIFY(gmkv::MkvToolNix::isToolDirectory(tempDir.path()));
}

void CoreTests::toolLocatorShouldPreferExplicitAndSavedPaths()
{
    QTemporaryDir explicitDir;
    QTemporaryDir savedDir;
    QVERIFY(explicitDir.isValid());
    QVERIFY(savedDir.isValid());

    writeFakeToolSet(savedDir.path());

    gmkv::ToolLocatorInputs inputs;
    inputs.explicitPath = explicitDir.path();
    inputs.savedPath = savedDir.path();
    inputs.applicationPath = QString();
    inputs.linuxDefaultPath = QString();
    inputs.searchPath = false;

    const QString expectedSavedLocation = gmkv::Platform::isLinux()
        ? QFileInfo(gmkv::MkvToolNix::executablePath(savedDir.path(), gmkv::MkvTool::Merge)).absoluteFilePath()
        : QDir(savedDir.path()).absolutePath();
    QCOMPARE(gmkv::ToolLocator::locate(inputs), expectedSavedLocation);

    writeFakeToolSet(explicitDir.path());

    const QString expectedExplicitLocation = gmkv::Platform::isLinux()
        ? QFileInfo(gmkv::MkvToolNix::executablePath(explicitDir.path(), gmkv::MkvTool::Merge)).absoluteFilePath()
        : QDir(explicitDir.path()).absolutePath();
    QCOMPARE(gmkv::ToolLocator::locate(inputs), expectedExplicitLocation);
}

void CoreTests::toolLocatorShouldFindLinuxToolsOnConfiguredPath()
{
    if (!gmkv::Platform::isLinux()) {
        QSKIP("Linux direct executable lookup is Linux-only.");
    }

    QTemporaryDir toolDir;
    QVERIFY(toolDir.isValid());
    writeFakeToolSet(toolDir.path());

    gmkv::ToolLocatorInputs inputs;
    inputs.explicitPath = QString();
    inputs.savedPath = QString();
    inputs.applicationPath = QString();
    inputs.linuxDefaultPath = QString();
    inputs.searchDirectories = { toolDir.path() };

    const gmkv::MkvToolPaths tools = gmkv::ToolLocator::locateTools(inputs);
    QVERIFY(tools.isValid());
    QCOMPARE(tools.path(gmkv::MkvTool::Merge), QFileInfo(gmkv::MkvToolNix::executablePath(toolDir.path(), gmkv::MkvTool::Merge)).absoluteFilePath());
    QCOMPARE(tools.path(gmkv::MkvTool::Info), QFileInfo(gmkv::MkvToolNix::executablePath(toolDir.path(), gmkv::MkvTool::Info)).absoluteFilePath());
    QCOMPARE(tools.path(gmkv::MkvTool::Extract), QFileInfo(gmkv::MkvToolNix::executablePath(toolDir.path(), gmkv::MkvTool::Extract)).absoluteFilePath());
    QCOMPARE(gmkv::ToolLocator::locate(inputs), tools.path(gmkv::MkvTool::Merge));
}

void CoreTests::toolLocatorShouldUseLinuxMkvmergeFileAsDisplayLocation()
{
    if (!gmkv::Platform::isLinux()) {
        QSKIP("Linux direct executable lookup is Linux-only.");
    }

    QTemporaryDir toolDir;
    QVERIFY(toolDir.isValid());
    writeFakeToolSet(toolDir.path());

    gmkv::ToolLocatorInputs inputs;
    inputs.explicitPath = gmkv::MkvToolNix::executablePath(toolDir.path(), gmkv::MkvTool::Merge);
    inputs.savedPath = QString();
    inputs.applicationPath = QString();
    inputs.linuxDefaultPath = QString();
    inputs.searchPath = false;

    const QString expectedLocation = QFileInfo(inputs.explicitPath).absoluteFilePath();
    QCOMPARE(gmkv::ToolLocator::locate(inputs), expectedLocation);
}

void CoreTests::processRunnerShouldCaptureStdoutAndExitCode()
{
    const QString program = gmkv::Platform::isLinux() ? QStringLiteral("/bin/sh") : QStringLiteral("cmd");
    const QStringList arguments = gmkv::Platform::isLinux()
        ? QStringList{ QStringLiteral("-c"), QStringLiteral("printf 'hello\\nworld\\n'") }
        : QStringList{ QStringLiteral("/C"), QStringLiteral("echo hello&&echo world") };

    const gmkv::ProcessResult result = gmkv::ProcessRunner::run(
        program,
        arguments);

    QCOMPARE(result.exitCode, 0);
    QCOMPARE(result.standardOutputLines, QStringList({ QStringLiteral("hello"), QStringLiteral("world") }));
    QVERIFY(result.standardErrorLines.isEmpty());
    QVERIFY(!result.hasProcessError());
}

void CoreTests::mkvInfoParserShouldParseTextOutput()
{
    const QStringList lines = {
        QStringLiteral("|+ Segment information"),
        QStringLiteral("| + Timecode scale: 1000000"),
        QStringLiteral("| + Muxing application: libebml"),
        QStringLiteral("| + Writing application: mkvmerge"),
        QStringLiteral("| + Duration: 1364.905s (00:22:44.905)"),
        QStringLiteral("| + Date: Mon Jan 20 21:40:32 2014 UTC"),
        QStringLiteral("|+ Segment tracks"),
        QStringLiteral("| + A track"),
        QStringLiteral("|  + Track number: 1 (track ID for mkvmerge & mkvextract: 0)"),
        QStringLiteral("|  + Track type: video"),
        QStringLiteral("|  + Codec ID: V_MPEG4/ISO/AVC"),
        QStringLiteral("|  + CodecPrivate, length 41 (h.264 profile: High @L4.1)"),
        QStringLiteral("|  + Language: jpn"),
        QStringLiteral("|  + Name: Video"),
        QStringLiteral("|   + Pixel width: 1280"),
        QStringLiteral("|   + Pixel height: 720"),
        QStringLiteral("| + A track"),
        QStringLiteral("|  + Track number: 2 (track ID for mkvmerge & mkvextract: 1)"),
        QStringLiteral("|  + Track type: audio"),
        QStringLiteral("|  + Codec ID: A_AAC"),
        QStringLiteral("|   + Sampling frequency: 48000"),
        QStringLiteral("|   + Channels: 2"),
        QStringLiteral("|+ Attachments"),
        QStringLiteral("| + Attached"),
        QStringLiteral("|  + File name: cover.jpg"),
        QStringLiteral("|  + Mime type: image/jpeg"),
        QStringLiteral("|  + File data, size: 12345"),
        QStringLiteral("|+ Chapters"),
        QStringLiteral("| + EditionEntry"),
        QStringLiteral("|  + ChapterAtom"),
        QStringLiteral("|  + ChapterAtom"),
    };

    const QList<gmkv::SegmentPtr> segments = gmkv::MkvInfoParser::parseOutput(lines, QStringLiteral("/tmp/movie.mkv"));
    QCOMPARE(segments.size(), 5);

    const auto info = std::dynamic_pointer_cast<gmkv::SegmentInfo>(segments[0]);
    QVERIFY(info != nullptr);
    QCOMPARE(info->filename, QStringLiteral("movie.mkv"));
    QCOMPARE(info->timecodeScale, QStringLiteral("1000000"));
    QCOMPARE(info->duration, QStringLiteral("1364.905s (00:22:44.905)"));

    const auto video = std::dynamic_pointer_cast<gmkv::Track>(segments[1]);
    QVERIFY(video != nullptr);
    QCOMPARE(video->trackID, 0);
    QCOMPARE(video->codecPrivate, QStringLiteral("length 41 (h.264 profile: High @L4.1)"));
    QCOMPARE(video->videoPixelWidth, 1280);
    QCOMPARE(video->extraInfo, QStringLiteral("1280x720"));

    const auto audio = std::dynamic_pointer_cast<gmkv::Track>(segments[2]);
    QVERIFY(audio != nullptr);
    QCOMPARE(audio->trackType, gmkv::MkvTrackType::Audio);
    QCOMPARE(audio->audioSamplingFrequency, 48000);
    QCOMPARE(audio->audioChannels, 2);

    const auto attachment = std::dynamic_pointer_cast<gmkv::Attachment>(segments[3]);
    QVERIFY(attachment != nullptr);
    QCOMPARE(attachment->id, 1);
    QCOMPARE(attachment->filename, QStringLiteral("cover.jpg"));

    const auto chapter = std::dynamic_pointer_cast<gmkv::Chapter>(segments[4]);
    QVERIFY(chapter != nullptr);
    QCOMPARE(chapter->chapterCount, 2);
}

void CoreTests::mkvInfoParserShouldCalculateDelaysFromCheckOutput()
{
    QList<gmkv::SegmentPtr> segments;
    auto video = std::make_shared<gmkv::Track>();
    video->trackNumber = 1;
    video->trackType = gmkv::MkvTrackType::Video;
    segments.append(video);

    auto audio = std::make_shared<gmkv::Track>();
    audio->trackNumber = 2;
    audio->trackType = gmkv::MkvTrackType::Audio;
    segments.append(audio);

    QVERIFY(gmkv::MkvInfoParser::findAndSetDelaysFromCheckOutput(segments, {
        QStringLiteral("track number 1, 1 frame(s), timestamp 00:00:00.100000000"),
        QStringLiteral("track number 2, 1 frame(s), timestamp 0.080s"),
    }));

    QCOMPARE(video->delay, 100);
    QCOMPARE(video->effectiveDelay, 100);
    QCOMPARE(audio->delay, 80);
    QCOMPARE(audio->effectiveDelay, -20);
}

void CoreTests::segmentMergerShouldFillMissingInfoFromMkvInfo()
{
    QList<gmkv::SegmentPtr> mergeSegments;
    auto mergeTrack = std::make_shared<gmkv::Track>();
    mergeTrack->trackID = 0;
    mergeTrack->trackNumber = 1;
    mergeTrack->trackType = gmkv::MkvTrackType::Video;
    mergeTrack->codecID = QStringLiteral("V_MPEG4/ISO/AVC");
    mergeSegments.append(mergeTrack);

    QList<gmkv::SegmentPtr> infoSegments;
    auto info = std::make_shared<gmkv::SegmentInfo>();
    info->filename = QStringLiteral("movie.mkv");
    infoSegments.append(info);

    auto infoTrack = std::make_shared<gmkv::Track>();
    infoTrack->trackID = 0;
    infoTrack->trackType = gmkv::MkvTrackType::Video;
    infoTrack->codecPrivate = QStringLiteral("length 41");
    infoTrack->videoPixelWidth = 1920;
    infoTrack->videoPixelHeight = 1080;
    infoTrack->extraInfo = QStringLiteral("1920x1080");
    infoSegments.append(infoTrack);

    const QList<gmkv::SegmentPtr> merged = gmkv::SegmentMerger::mergeMkvMergeAndInfoSegments(mergeSegments, infoSegments);
    QCOMPARE(merged.size(), 2);
    QVERIFY(std::dynamic_pointer_cast<gmkv::SegmentInfo>(merged[0]) != nullptr);
    QCOMPARE(mergeTrack->codecPrivate, QStringLiteral("length 41"));
    QCOMPARE(mergeTrack->videoPixelWidth, 1920);
    QCOMPARE(mergeTrack->extraInfo, QStringLiteral("1920x1080"));
}

void CoreTests::segmentMergerShouldTranslateCodecPrivateData()
{
    QList<gmkv::SegmentPtr> mergeSegments;
    auto track = std::make_shared<gmkv::Track>();
    track->trackID = 0;
    track->trackType = gmkv::MkvTrackType::Video;
    track->codecID = QStringLiteral("V_MPEG4/ISO/AVC");
    track->codecPrivateData = QStringLiteral("0164001f");
    mergeSegments.append(track);

    gmkv::SegmentMerger::mergeMkvMergeAndInfoSegments(mergeSegments, {});
    QCOMPARE(track->codecPrivate, QStringLiteral("length 4 (h.264 profile: High @L3.1)"));
}

void CoreTests::mkvMergeParserShouldParseJsonIdentification()
{
    const QString json = QStringLiteral(R"({
        "container": {
            "recognized": true,
            "supported": true,
            "properties": {
                "date_utc": "2025-03-17T00:00:00Z",
                "duration": "5979008000000",
                "muxing_application": "libebml",
                "writing_application": "mkvmerge"
            }
        },
        "tracks": [
            {
                "id": 0,
                "type": "video",
                "properties": {
                    "number": 1,
                    "codec_id": "V_MPEG4/ISO/AVC",
                    "language": "eng",
                    "language_ietf": "en",
                    "track_name": "Main",
                    "pixel_dimensions": "1920x1080",
                    "minimum_timestamp": 100000000
                }
            },
            {
                "id": 1,
                "type": "audio",
                "properties": {
                    "number": 2,
                    "codec_id": "A_AC3",
                    "audio_channels": "6",
                    "audio_sampling_frequency": "48000",
                    "minimum_timestamp": 80000000
                }
            }
        ],
        "attachments": [
            { "id": 3, "file_name": "cover.jpg", "content_type": "image/jpeg", "size": "12345" }
        ],
        "chapters": [
            { "num_entries": 4 }
        ]
    })");

    const QList<gmkv::SegmentPtr> segments = gmkv::MkvMergeParser::parseJsonOutput(json, QStringLiteral("/tmp/movie.mkv"));

    QCOMPARE(segments.size(), 5);

    const auto info = std::dynamic_pointer_cast<gmkv::SegmentInfo>(segments[0]);
    QVERIFY(info != nullptr);
    QCOMPARE(info->filename, QStringLiteral("movie.mkv"));
    QCOMPARE(info->duration, QStringLiteral("5979.008s (01:39:39.008)"));
    QCOMPARE(info->muxingApplication, QStringLiteral("libebml"));
    QCOMPARE(info->writingApplication, QStringLiteral("mkvmerge"));

    const auto video = std::dynamic_pointer_cast<gmkv::Track>(segments[1]);
    QVERIFY(video != nullptr);
    QCOMPARE(video->trackID, 0);
    QCOMPARE(video->trackNumber, 1);
    QCOMPARE(video->trackType, gmkv::MkvTrackType::Video);
    QCOMPARE(video->codecID, QStringLiteral("V_MPEG4/ISO/AVC"));
    QCOMPARE(video->language, QStringLiteral("eng"));
    QCOMPARE(video->languageIetf, QStringLiteral("en"));
    QCOMPARE(video->trackName, QStringLiteral("Main"));
    QCOMPARE(video->videoPixelWidth, 1920);
    QCOMPARE(video->videoPixelHeight, 1080);

    const auto audio = std::dynamic_pointer_cast<gmkv::Track>(segments[2]);
    QVERIFY(audio != nullptr);
    QCOMPARE(audio->trackType, gmkv::MkvTrackType::Audio);
    QCOMPARE(audio->extraInfo, QStringLiteral("48000Hz, Ch: 6"));
    QCOMPARE(audio->audioChannels, 6);
    QCOMPARE(audio->audioSamplingFrequency, 48000);

    const auto attachment = std::dynamic_pointer_cast<gmkv::Attachment>(segments[3]);
    QVERIFY(attachment != nullptr);
    QCOMPARE(attachment->filename, QStringLiteral("cover.jpg"));
    QCOMPARE(attachment->mimeType, QStringLiteral("image/jpeg"));

    const auto chapter = std::dynamic_pointer_cast<gmkv::Chapter>(segments[4]);
    QVERIFY(chapter != nullptr);
    QCOMPARE(chapter->chapterCount, 4);
}

void CoreTests::mkvMergeParserShouldCalculateDelaysFromMinimumTimestamps()
{
    QList<gmkv::SegmentPtr> segments;
    auto video = std::make_shared<gmkv::Track>();
    video->trackType = gmkv::MkvTrackType::Video;
    video->minimumTimestamp = 100000000;
    segments.append(video);

    auto audio = std::make_shared<gmkv::Track>();
    audio->trackType = gmkv::MkvTrackType::Audio;
    audio->minimumTimestamp = 80000000;
    segments.append(audio);

    QVERIFY(gmkv::MkvMergeParser::findAndSetDelays(segments));
    QCOMPARE(video->delay, 100);
    QCOMPARE(video->effectiveDelay, 100);
    QCOMPARE(audio->delay, 80);
    QCOMPARE(audio->effectiveDelay, -20);
}

void CoreTests::writeTranslationFile(const QString& directory, const QString& culture, const QMap<QString, QString>& entries)
{
    gmkv::TranslationFile translationFile;
    translationFile.metadata.culture = culture;
    translationFile.metadata.creationDate = QDateTime(QDate(2026, 4, 18), QTime(0, 0), Qt::UTC);
    translationFile.metadata.lastEditDate = translationFile.metadata.creationDate;

    for (auto it = entries.begin(); it != entries.end(); ++it) {
        translationFile.entries[it.key()] = { it.value(), it.value(), true, QString() };
    }

    gmkv::TranslationFileService::saveFile(
        translationFile,
        QDir(directory).filePath(gmkv::TranslationPathService::translationFileName(culture)));
}

QTEST_MAIN(CoreTests)

#include "tst_Core.moc"
