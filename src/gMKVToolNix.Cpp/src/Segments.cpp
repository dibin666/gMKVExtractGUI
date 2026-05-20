#include "gmkvtoolnix/Segments.h"

#include <QDir>

namespace gmkv {

QString SegmentInfo::path() const
{
    return QDir(directory).filePath(filename);
}

QString Track::toString() const
{
    QString output = QStringLiteral("Track %1 [TID %2][%3][%4]")
        .arg(trackNumber)
        .arg(trackID)
        .arg(toDisplayString(trackType), codecID);

    if (!trackName.trimmed().isEmpty()) {
        output += QStringLiteral("[%1]").arg(trackName);
    }

    if (!language.trimmed().isEmpty()) {
        output += QStringLiteral("[%1]").arg(language);
    }

    if (!languageIetf.trimmed().isEmpty()) {
        output += QStringLiteral("[%1]").arg(languageIetf);
    }

    if (forced) {
        output += QStringLiteral("[FORCED]");
    }

    if (!extraInfo.trimmed().isEmpty()) {
        output += QStringLiteral("[%1]").arg(extraInfo);
    }

    if (!codecPrivate.trimmed().isEmpty()) {
        output += QStringLiteral("[%1]").arg(codecPrivate);
    }

    if (trackType != MkvTrackType::Subtitles) {
        output += QStringLiteral("[%1 ms][%2 ms]").arg(delay).arg(effectiveDelay);
    }

    return output;
}

QString Attachment::toString() const
{
    return QStringLiteral("Attachment %1 [%2][%3][%4 bytes]")
        .arg(id)
        .arg(filename, mimeType, fileSize);
}

QString Chapter::toString() const
{
    const QString entryString = chapterCount > 1
        ? QStringLiteral("entries")
        : QStringLiteral("entry");

    return QStringLiteral("Chapters %1 %2").arg(chapterCount).arg(entryString);
}

QString toDisplayString(MkvTrackType trackType)
{
    switch (trackType) {
    case MkvTrackType::Video:
        return QStringLiteral("video");
    case MkvTrackType::Audio:
        return QStringLiteral("audio");
    case MkvTrackType::Subtitles:
        return QStringLiteral("subtitles");
    }

    return QStringLiteral("video");
}

QString toSettingsString(MkvChapterType chapterType)
{
    switch (chapterType) {
    case MkvChapterType::Xml:
        return QStringLiteral("XML");
    case MkvChapterType::Ogm:
        return QStringLiteral("OGM");
    case MkvChapterType::Cue:
        return QStringLiteral("CUE");
    case MkvChapterType::Pbf:
        return QStringLiteral("PBF");
    }

    return QStringLiteral("XML");
}

MkvChapterType chapterTypeFromString(const QString& value, MkvChapterType fallback)
{
    const QString normalized = value.trimmed();
    if (normalized.compare(QStringLiteral("XML"), Qt::CaseInsensitive) == 0) {
        return MkvChapterType::Xml;
    }
    if (normalized.compare(QStringLiteral("OGM"), Qt::CaseInsensitive) == 0) {
        return MkvChapterType::Ogm;
    }
    if (normalized.compare(QStringLiteral("CUE"), Qt::CaseInsensitive) == 0) {
        return MkvChapterType::Cue;
    }
    if (normalized.compare(QStringLiteral("PBF"), Qt::CaseInsensitive) == 0) {
        return MkvChapterType::Pbf;
    }

    return fallback;
}

}
