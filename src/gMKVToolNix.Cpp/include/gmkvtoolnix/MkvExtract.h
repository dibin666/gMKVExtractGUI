#pragma once

#include "gmkvtoolnix/FilenamePatterns.h"
#include "gmkvtoolnix/Segments.h"
#include "gmkvtoolnix/Version.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

#include <atomic>

class QFile;

namespace gmkv {

struct TrackParameter
{
    MkvExtractMode extractMode = MkvExtractMode::Tracks;
    QStringList options;
    QStringList trackOutputs;
    bool writeOutputToFile = false;
    bool disableBomForTextFiles = false;
    bool useRawExtractionMode = false;
    bool useFullRawExtractionMode = false;
    QString outputFilename;
};

struct ExtractSegmentsParameters
{
    QString mkvFile;
    QList<SegmentPtr> segmentsToExtract;
    QString outputDirectory;
    MkvChapterType chapterType = MkvChapterType::Xml;
    TimecodesExtractionMode timecodesExtractionMode = TimecodesExtractionMode::NoTimecodes;
    CuesExtractionMode cueExtractionMode = CuesExtractionMode::NoCues;
    FilenamePatterns filenamePatterns;
    bool disableBomForTextFiles = false;
    bool useRawExtractionMode = false;
    bool useFullRawExtractionMode = false;
    bool overwriteExistingFile = false;
};

struct PlannedExtractCommand
{
    TrackParameter parameter;
    QStringList arguments;
    QString sourceFile;
    QMap<QString, QString> environment;
};

struct MkvExtractRunResult
{
    int exitCode = 0;
    QStringList errors;
    QString errorString;
    bool aborted = false;

    bool succeeded() const;
};

class MkvExtractPlanner
{
public:
    static QString modeName(MkvExtractMode extractMode);

    static QList<TrackParameter> createTrackParameters(
        const Segment& segment,
        const ExtractSegmentsParameters& parameters,
        const Version& version);

    static QList<TrackParameter> createGroupedTrackParameters(
        const ExtractSegmentsParameters& parameters,
        const Version& version);

    static QList<PlannedExtractCommand> planSegments(
        const ExtractSegmentsParameters& parameters,
        const Version& version);

    static PlannedExtractCommand planTags(
        const ExtractSegmentsParameters& parameters,
        const Version& version);

    static PlannedExtractCommand planCueSheet(
        const ExtractSegmentsParameters& parameters,
        const Version& version);

    static QStringList commandLineArguments(
        const QString& mkvFile,
        const TrackParameter& parameter,
        const Version& version);
};

class MkvExtractRunner final : public QObject
{
    Q_OBJECT

public:
    explicit MkvExtractRunner(QString mkvExtractProgram, QObject* parent = nullptr);

    MkvExtractRunResult run(const QList<PlannedExtractCommand>& commands);
    MkvExtractRunResult runCommand(const PlannedExtractCommand& command);

    void abort();
    void abortAll();

    static int progressFromLine(const QString& line, bool* ok = nullptr);
    static QString errorFromLine(const QString& line, bool* ok = nullptr);

signals:
    void progressUpdated(int progress);
    void trackUpdated(const QString& filename, const QString& trackName);

private:
    void handleProcessLine(
        const QString& line,
        bool writeLineToOutputFile,
        QFile* outputFile,
        MkvExtractRunResult& result);

    QString m_mkvExtractProgram;
    std::atomic_bool m_abort = false;
    std::atomic_bool m_abortAllRequested = false;
};

}
