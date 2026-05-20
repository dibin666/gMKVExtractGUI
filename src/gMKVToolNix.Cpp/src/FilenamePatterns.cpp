#include "gmkvtoolnix/FilenamePatterns.h"

namespace gmkv {

QString FilenamePatterns::defaultVideoTrackFilenamePattern()
{
    return QStringLiteral("{FilenameNoExt}_track{TrackNumber}_[{Language}]");
}

QString FilenamePatterns::defaultAudioTrackFilenamePattern()
{
    return QStringLiteral("{FilenameNoExt}_track{TrackNumber}_[{Language}]_DELAY {EffectiveDelay}ms");
}

QString FilenamePatterns::defaultSubtitleTrackFilenamePattern()
{
    return QStringLiteral("{FilenameNoExt}_track{TrackNumber}_[{Language}]");
}

QString FilenamePatterns::defaultChapterFilenamePattern()
{
    return QStringLiteral("{FilenameNoExt}_chapters");
}

QString FilenamePatterns::defaultAttachmentFilenamePattern()
{
    return QStringLiteral("{AttachmentFilename}");
}

QString FilenamePatterns::defaultTagsFilenamePattern()
{
    return QStringLiteral("{FilenameNoExt}_tags");
}

}
