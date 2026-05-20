#include "gmkvtoolnix/MkvExtract.h"

#include "gmkvtoolnix/ExtractionNaming.h"
#include "gmkvtoolnix/Log.h"
#include "gmkvtoolnix/MkvToolNix.h"
#include "gmkvtoolnix/OptionValue.h"

#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>

#include <algorithm>
#include <functional>
#include <utility>

namespace gmkv {

namespace {

bool versionAtLeast(const Version& version, int major, int minor = 0)
{
    return version.fileMajorPart > major
        || (version.fileMajorPart == major && version.fileMinorPart >= minor);
}

bool shouldExtractSegment(TimecodesExtractionMode timecodesMode, CuesExtractionMode cuesMode)
{
    return !((timecodesMode == TimecodesExtractionMode::OnlyTimecodes
                 && cuesMode == CuesExtractionMode::NoCues)
             || (timecodesMode == TimecodesExtractionMode::NoTimecodes
                 && cuesMode == CuesExtractionMode::OnlyCues))
        || (timecodesMode == TimecodesExtractionMode::OnlyTimecodes
            && cuesMode == CuesExtractionMode::OnlyCues);
}

TrackParameter makeParameter(
    MkvExtractMode extractMode,
    QStringList options,
    QStringList trackOutputs,
    bool writeOutputToFile,
    const ExtractSegmentsParameters& parameters,
    QString outputFilename = {})
{
    TrackParameter parameter;
    parameter.extractMode = extractMode;
    parameter.options = std::move(options);
    parameter.trackOutputs = std::move(trackOutputs);
    parameter.writeOutputToFile = writeOutputToFile;
    parameter.disableBomForTextFiles = parameters.disableBomForTextFiles;
    parameter.useRawExtractionMode = parameters.useRawExtractionMode;
    parameter.useFullRawExtractionMode = parameters.useFullRawExtractionMode;
    parameter.outputFilename = std::move(outputFilename);
    return parameter;
}

QString extractionSpec(int id, const QString& filename)
{
    return QStringLiteral("%1:%2").arg(id).arg(filename);
}

TrackParameter createTagsOrCueSheetParameter(
    MkvExtractMode mode,
    const ExtractSegmentsParameters& parameters,
    const Version& version)
{
    SegmentInfo contextSegment;
    const QString outputFilename = ExtractionNaming::outputFilename(
        contextSegment,
        parameters.outputDirectory,
        parameters.mkvFile,
        parameters.filenamePatterns,
        parameters.overwriteExistingFile,
        mode,
        parameters.chapterType);

    const bool writesToFileFromStdout = version.fileMajorPart < 17;
    return makeParameter(
        mode,
        {},
        writesToFileFromStdout ? QStringList{} : QStringList{ outputFilename },
        writesToFileFromStdout,
        parameters,
        writesToFileFromStdout ? outputFilename : QString());
}

QMap<QString, QString> environmentOverrides(const Version& version)
{
    if (!Platform::isLinux() || versionAtLeast(version, 9, 7)) {
        return {};
    }

    return {
        { QStringLiteral("LC_ALL"), QStringLiteral("en_US.UTF-8") },
        { QStringLiteral("LANG"), QStringLiteral("en_US.UTF-8") },
        { QStringLiteral("LC_MESSAGES"), QStringLiteral("en_US.UTF-8") },
    };
}

PlannedExtractCommand plannedCommand(const ExtractSegmentsParameters& parameters, const TrackParameter& parameter, const Version& version)
{
    return {
        parameter,
        MkvExtractPlanner::commandLineArguments(parameters.mkvFile, parameter, version),
        parameters.mkvFile,
        environmentOverrides(version),
    };
}

void consumeLines(QByteArray& buffer, const QByteArray& data, const std::function<void(const QString&)>& handler)
{
    buffer.append(data);

    int newlineIndex = buffer.indexOf('\n');
    while (newlineIndex >= 0) {
        QByteArray line = buffer.left(newlineIndex);
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        handler(QString::fromUtf8(line));
        buffer.remove(0, newlineIndex + 1);
        newlineIndex = buffer.indexOf('\n');
    }
}

void flushLineBuffer(QByteArray& buffer, const std::function<void(const QString&)>& handler)
{
    if (buffer.isEmpty()) {
        return;
    }

    if (buffer.endsWith('\r')) {
        buffer.chop(1);
    }

    handler(QString::fromUtf8(buffer));
    buffer.clear();
}

}

bool MkvExtractRunResult::succeeded() const
{
    return !aborted
        && errorString.isEmpty()
        && errors.isEmpty()
        && exitCode <= 1;
}

QString MkvExtractPlanner::modeName(MkvExtractMode extractMode)
{
    switch (extractMode) {
    case MkvExtractMode::Tracks:
        return QStringLiteral("tracks");
    case MkvExtractMode::Tags:
        return QStringLiteral("tags");
    case MkvExtractMode::Attachments:
        return QStringLiteral("attachments");
    case MkvExtractMode::Chapters:
        return QStringLiteral("chapters");
    case MkvExtractMode::CueSheet:
        return QStringLiteral("cuesheet");
    case MkvExtractMode::TimecodesV2:
        return QStringLiteral("timecodes_v2");
    case MkvExtractMode::Cues:
        return QStringLiteral("cues");
    case MkvExtractMode::TimestampsV2:
        return QStringLiteral("timestamps_v2");
    }

    return QStringLiteral("tracks");
}

QList<TrackParameter> MkvExtractPlanner::createTrackParameters(
    const Segment& segment,
    const ExtractSegmentsParameters& parameters,
    const Version& version)
{
    QList<TrackParameter> trackParameters;

    if (const auto* track = dynamic_cast<const Track*>(&segment)) {
        if (parameters.timecodesExtractionMode != TimecodesExtractionMode::NoTimecodes) {
            const MkvExtractMode timecodesMode = version.fileMajorPart >= 17
                ? MkvExtractMode::TimestampsV2
                : MkvExtractMode::TimecodesV2;

            trackParameters.append(makeParameter(
                timecodesMode,
                {},
                {
                    extractionSpec(
                        track->trackID,
                        ExtractionNaming::outputFilename(
                            segment,
                            parameters.outputDirectory,
                            parameters.mkvFile,
                            parameters.filenamePatterns,
                            parameters.overwriteExistingFile,
                            MkvExtractMode::TimestampsV2))
                },
                false,
                parameters));
        }

        if (parameters.cueExtractionMode != CuesExtractionMode::NoCues) {
            trackParameters.append(makeParameter(
                MkvExtractMode::Cues,
                {},
                {
                    extractionSpec(
                        track->trackID,
                        ExtractionNaming::outputFilename(
                            segment,
                            parameters.outputDirectory,
                            parameters.mkvFile,
                            parameters.filenamePatterns,
                            parameters.overwriteExistingFile,
                            MkvExtractMode::Cues))
                },
                false,
                parameters));
        }

        if (shouldExtractSegment(parameters.timecodesExtractionMode, parameters.cueExtractionMode)) {
            trackParameters.append(makeParameter(
                MkvExtractMode::Tracks,
                {},
                {
                    extractionSpec(
                        track->trackID,
                        ExtractionNaming::outputFilename(
                            segment,
                            parameters.outputDirectory,
                            parameters.mkvFile,
                            parameters.filenamePatterns,
                            parameters.overwriteExistingFile,
                            MkvExtractMode::Tracks))
                },
                false,
                parameters));
        }
    } else if (const auto* attachment = dynamic_cast<const Attachment*>(&segment)) {
        if (shouldExtractSegment(parameters.timecodesExtractionMode, parameters.cueExtractionMode)) {
            trackParameters.append(makeParameter(
                MkvExtractMode::Attachments,
                {},
                {
                    extractionSpec(
                        attachment->id,
                        ExtractionNaming::outputFilename(
                            segment,
                            parameters.outputDirectory,
                            parameters.mkvFile,
                            parameters.filenamePatterns,
                            parameters.overwriteExistingFile,
                            MkvExtractMode::Attachments))
                },
                false,
                parameters));
        }
    } else if (dynamic_cast<const Chapter*>(&segment) != nullptr) {
        if (shouldExtractSegment(parameters.timecodesExtractionMode, parameters.cueExtractionMode)) {
            const QString chapterFilename = ExtractionNaming::outputFilename(
                segment,
                parameters.outputDirectory,
                parameters.mkvFile,
                parameters.filenamePatterns,
                parameters.overwriteExistingFile,
                MkvExtractMode::Chapters,
                parameters.chapterType);

            const bool writesToFileFromStdout = version.fileMajorPart < 17;
            trackParameters.append(makeParameter(
                MkvExtractMode::Chapters,
                parameters.chapterType == MkvChapterType::Ogm ? QStringList{ QStringLiteral("--simple") } : QStringList{},
                writesToFileFromStdout ? QStringList{} : QStringList{ chapterFilename },
                writesToFileFromStdout,
                parameters,
                writesToFileFromStdout ? chapterFilename : QString()));
        }
    }

    return trackParameters;
}

QList<TrackParameter> MkvExtractPlanner::createGroupedTrackParameters(
    const ExtractSegmentsParameters& parameters,
    const Version& version)
{
    QList<TrackParameter> groupedParameters;

    for (const SegmentPtr& segment : parameters.segmentsToExtract) {
        if (!segment) {
            continue;
        }

        const QList<TrackParameter> segmentParameters = createTrackParameters(*segment, parameters, version);
        for (const TrackParameter& parameter : segmentParameters) {
            auto existing = std::find_if(groupedParameters.begin(), groupedParameters.end(), [&](const TrackParameter& grouped) {
                return grouped.extractMode == parameter.extractMode;
            });

            if (existing == groupedParameters.end()) {
                groupedParameters.append(parameter);
            } else {
                existing->trackOutputs.append(parameter.trackOutputs);
            }
        }
    }

    return groupedParameters;
}

QList<PlannedExtractCommand> MkvExtractPlanner::planSegments(
    const ExtractSegmentsParameters& parameters,
    const Version& version)
{
    QList<PlannedExtractCommand> commands;
    for (const TrackParameter& parameter : createGroupedTrackParameters(parameters, version)) {
        commands.append(plannedCommand(parameters, parameter, version));
    }
    return commands;
}

PlannedExtractCommand MkvExtractPlanner::planTags(
    const ExtractSegmentsParameters& parameters,
    const Version& version)
{
    const TrackParameter parameter = createTagsOrCueSheetParameter(MkvExtractMode::Tags, parameters, version);
    return plannedCommand(parameters, parameter, version);
}

PlannedExtractCommand MkvExtractPlanner::planCueSheet(
    const ExtractSegmentsParameters& parameters,
    const Version& version)
{
    const TrackParameter parameter = createTagsOrCueSheetParameter(MkvExtractMode::CueSheet, parameters, version);
    return plannedCommand(parameters, parameter, version);
}

QStringList MkvExtractPlanner::commandLineArguments(
    const QString& mkvFile,
    const TrackParameter& parameter,
    const Version& version)
{
    QStringList options;
    if (versionAtLeast(version, 9, 7)) {
        options.append(optionName(MkvExtractGlobalOption::GuiMode));
    }

    if (version.fileMajorPart >= 96 && parameter.disableBomForTextFiles) {
        options.append(QStringLiteral("--no-bom"));
    }

    options.append(optionName(MkvExtractGlobalOption::UiLanguage));
    options.append(MkvToolNix::uiLanguageCode());
    options.append(parameter.options);

    if (parameter.extractMode == MkvExtractMode::Tracks) {
        if (parameter.useFullRawExtractionMode) {
            options.append(QStringLiteral("--fullraw"));
        } else if (parameter.useRawExtractionMode) {
            options.append(QStringLiteral("--raw"));
        }
    }

    QStringList arguments;
    if (version.fileMajorPart >= 17) {
        arguments.append(mkvFile);
        arguments.append(modeName(parameter.extractMode));
        arguments.append(options);
        arguments.append(parameter.trackOutputs);
    } else {
        arguments.append(modeName(parameter.extractMode));
        arguments.append(mkvFile);
        arguments.append(options);
        arguments.append(parameter.trackOutputs);
    }

    return arguments;
}

MkvExtractRunner::MkvExtractRunner(QString mkvExtractProgram, QObject* parent)
    : QObject(parent)
    , m_mkvExtractProgram(std::move(mkvExtractProgram))
{
}

MkvExtractRunResult MkvExtractRunner::run(const QList<PlannedExtractCommand>& commands)
{
    m_abort.store(false);
    m_abortAllRequested.store(false);

    MkvExtractRunResult aggregateResult;
    for (const PlannedExtractCommand& command : commands) {
        if (m_abortAllRequested.load()) {
            aggregateResult.aborted = true;
            aggregateResult.errors.append(QStringLiteral("User aborted all the processes!"));
            break;
        }

        const MkvExtractRunResult commandResult = runCommand(command);
        aggregateResult.exitCode = commandResult.exitCode;
        aggregateResult.aborted = aggregateResult.aborted || commandResult.aborted;
        if (!commandResult.errorString.isEmpty()) {
            aggregateResult.errorString = commandResult.errorString;
        }
        aggregateResult.errors.append(commandResult.errors);
    }

    return aggregateResult;
}

MkvExtractRunResult MkvExtractRunner::runCommand(const PlannedExtractCommand& command)
{
    MkvExtractRunResult result;
    emit progressUpdated(0);
    emit trackUpdated(command.sourceFile, MkvExtractPlanner::modeName(command.parameter.extractMode));

    QFile outputFile(command.parameter.outputFilename);
    if (command.parameter.writeOutputToFile) {
        if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            result.errorString = outputFile.errorString();
            result.errors.append(result.errorString);
            return result;
        }
    }

    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    for (auto it = command.environment.begin(); it != command.environment.end(); ++it) {
        environment.insert(it.key(), it.value());
    }
    process.setProcessEnvironment(environment);

    Logger::log(QStringLiteral("\"%1\" %2").arg(m_mkvExtractProgram, command.arguments.join(QLatin1Char(' '))));

    process.start(m_mkvExtractProgram, command.arguments);

    if (!process.waitForStarted()) {
        result.errorString = process.errorString();
        result.errors.append(result.errorString);
        return result;
    }

    QByteArray stdoutBuffer;
    QByteArray stderrBuffer;
    const auto stdoutHandler = [&](const QString& line) {
        handleProcessLine(line, command.parameter.writeOutputToFile, &outputFile, result);
    };
    const auto stderrHandler = [&](const QString& line) {
        handleProcessLine(line, false, nullptr, result);
    };

    while (process.state() != QProcess::NotRunning) {
        if (m_abort.load() || m_abortAllRequested.load()) {
            process.kill();
            process.waitForFinished(5000);
            result.aborted = true;
            break;
        }

        process.waitForReadyRead(100);
        consumeLines(stdoutBuffer, process.readAllStandardOutput(), stdoutHandler);
        consumeLines(stderrBuffer, process.readAllStandardError(), stderrHandler);
    }

    process.waitForFinished();
    consumeLines(stdoutBuffer, process.readAllStandardOutput(), stdoutHandler);
    consumeLines(stderrBuffer, process.readAllStandardError(), stderrHandler);
    flushLineBuffer(stdoutBuffer, stdoutHandler);
    flushLineBuffer(stderrBuffer, stderrHandler);

    result.exitCode = process.exitCode();
    Logger::log(QStringLiteral("Exit code: %1").arg(result.exitCode));

    if (process.error() != QProcess::UnknownError && !result.aborted) {
        result.errorString = process.errorString();
    }

    if (result.aborted || result.exitCode < 0) {
        result.aborted = true;
        result.errors.append(QStringLiteral("User aborted the current process!"));
        m_abort.store(false);
    } else if (result.exitCode > 1) {
        result.errors.append(QStringLiteral("Mkvextract exited with error code %1!").arg(result.exitCode));
    }

    return result;
}

void MkvExtractRunner::abort()
{
    m_abort.store(true);
}

void MkvExtractRunner::abortAll()
{
    m_abortAllRequested.store(true);
    abort();
}

int MkvExtractRunner::progressFromLine(const QString& line, bool* ok)
{
    static const QRegularExpression guiProgress(QStringLiteral(R"(#GUI#progress\s+(\d+)%?)"));
    static const QRegularExpression textProgress(QStringLiteral(R"(Progress:\s*(\d+)%?)"));

    QRegularExpressionMatch match = guiProgress.match(line);
    if (!match.hasMatch()) {
        match = textProgress.match(line);
    }

    if (!match.hasMatch()) {
        if (ok != nullptr) {
            *ok = false;
        }
        return 0;
    }

    bool parsed = false;
    const int progress = match.captured(1).toInt(&parsed);
    if (ok != nullptr) {
        *ok = parsed;
    }
    return progress;
}

QString MkvExtractRunner::errorFromLine(const QString& line, bool* ok)
{
    static const QRegularExpression guiError(QStringLiteral(R"(#GUI#error\s+(.+))"));
    static const QRegularExpression textError(QStringLiteral(R"(Error:\s*(.+))"));

    QRegularExpressionMatch match = guiError.match(line);
    if (!match.hasMatch()) {
        match = textError.match(line);
    }

    if (!match.hasMatch()) {
        if (ok != nullptr) {
            *ok = false;
        }
        return {};
    }

    if (ok != nullptr) {
        *ok = true;
    }
    return match.captured(1).trimmed();
}

void MkvExtractRunner::handleProcessLine(
    const QString& line,
    bool writeLineToOutputFile,
    QFile* outputFile,
    MkvExtractRunResult& result)
{
    if (line.trimmed().isEmpty()) {
        return;
    }

    Logger::log(line);

    if (writeLineToOutputFile && outputFile != nullptr) {
        outputFile->write(line.toUtf8());
        outputFile->write("\n");
    }

    bool ok = false;
    const int progress = progressFromLine(line, &ok);
    if (ok) {
        emit progressUpdated(progress);
        return;
    }

    const QString error = errorFromLine(line, &ok);
    if (ok) {
        result.errors.append(error);
    }
}

}
