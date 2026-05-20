#pragma once

#include "gmkvtoolnix/FilenamePatterns.h"
#include "gmkvtoolnix/Segments.h"

#include <QString>

namespace gmkv {

class ExtractionNaming
{
public:
    static QString replaceFilenamePlaceholders(const Segment& segment, const QString& mkvFile, const QString& filenamePattern);
    static QString outputFilename(
        const Segment& segment,
        const QString& outputDirectory,
        const QString& mkvFile,
        const FilenamePatterns& filenamePatterns,
        bool overwriteExistingFile,
        MkvExtractMode extractMode,
        MkvChapterType chapterType = MkvChapterType::Xml);

    static QString videoFileExtensionFromCodecID(const Track& track);
    static QString audioFileExtensionFromCodecID(const Track& track);
    static QString subtitleFileExtensionFromCodecID(const Track& track);
};

}
