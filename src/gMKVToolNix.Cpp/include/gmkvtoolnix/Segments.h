#pragma once

#include <QString>

#include <limits>
#include <memory>

namespace gmkv {

enum class MkvTrackType
{
    Video,
    Audio,
    Subtitles,
};

enum class MkvChapterType
{
    Xml,
    Ogm,
    Cue,
    Pbf,
};

enum class MkvExtractMode
{
    Tracks,
    Tags,
    Attachments,
    Chapters,
    CueSheet,
    TimecodesV2,
    Cues,
    TimestampsV2,
};

enum class TimecodesExtractionMode
{
    NoTimecodes,
    WithTimecodes,
    OnlyTimecodes,
};

enum class CuesExtractionMode
{
    NoCues,
    WithCues,
    OnlyCues,
};

class Segment
{
public:
    virtual ~Segment() = default;
};

using SegmentPtr = std::shared_ptr<Segment>;

struct SegmentInfo : public Segment
{
    QString timecodeScale;
    QString muxingApplication;
    QString writingApplication;
    QString duration;
    QString date;
    QString filename;
    QString directory;

    QString path() const;
};

struct Track : public Segment
{
    int trackNumber = 0;
    int trackID = 0;
    MkvTrackType trackType = MkvTrackType::Video;
    QString codecID;
    QString codecPrivate;
    QString codecPrivateData;
    bool forced = false;
    QString language;
    QString languageIetf;
    QString trackName;
    QString extraInfo;
    int delay = std::numeric_limits<int>::min();
    int effectiveDelay = std::numeric_limits<int>::min();
    qint64 minimumTimestamp = std::numeric_limits<qint64>::min();
    int videoPixelWidth = 0;
    int videoPixelHeight = 0;
    int audioSamplingFrequency = 0;
    int audioChannels = 0;

    QString toString() const;
};

struct Attachment : public Segment
{
    int id = 0;
    QString filename;
    QString mimeType;
    QString fileSize;

    QString toString() const;
};

struct Chapter : public Segment
{
    int chapterCount = 0;

    QString toString() const;
};

QString toDisplayString(MkvTrackType trackType);
QString toSettingsString(MkvChapterType chapterType);
MkvChapterType chapterTypeFromString(const QString& value, MkvChapterType fallback = MkvChapterType::Xml);

}
