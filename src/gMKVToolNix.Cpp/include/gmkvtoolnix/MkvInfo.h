#pragma once

#include "gmkvtoolnix/Segments.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace gmkv {

class MkvInfoParser
{
public:
    static QList<SegmentPtr> parseOutput(const QStringList& outputLines, const QString& inputFile = QString());
    static bool findAndSetDelaysFromCheckOutput(const QList<SegmentPtr>& segments, const QStringList& outputLines);
};

class SegmentMerger
{
public:
    static QList<SegmentPtr> mergeMkvMergeAndInfoSegments(QList<SegmentPtr> mkvMergeSegments, const QList<SegmentPtr>& mkvInfoSegments);
};

class SegmentAnalyzer
{
public:
    static QList<SegmentPtr> analyzeFile(const QString& mkvToolNixDirectory, const QString& inputFile);
};

}
