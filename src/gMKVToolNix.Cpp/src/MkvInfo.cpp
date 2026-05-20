#include "gmkvtoolnix/MkvInfo.h"

#include "gmkvtoolnix/Log.h"
#include "gmkvtoolnix/MkvMerge.h"
#include "gmkvtoolnix/MkvToolNix.h"

#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace gmkv {

namespace {

enum class ParseState
{
    Searching,
    SegmentInfo,
    Tracks,
    Attachments,
    Chapters,
};

QString valueAfterColon(const QString& line)
{
    const int index = line.indexOf(QLatin1Char(':'));
    return index < 0 ? QString() : line.mid(index + 1).trimmed();
}

MkvTrackType trackTypeFromString(const QString& value)
{
    if (value.compare(QStringLiteral("audio"), Qt::CaseInsensitive) == 0) {
        return MkvTrackType::Audio;
    }
    if (value.compare(QStringLiteral("subtitles"), Qt::CaseInsensitive) == 0
        || value.compare(QStringLiteral("subtitle"), Qt::CaseInsensitive) == 0) {
        return MkvTrackType::Subtitles;
    }
    return MkvTrackType::Video;
}

void appendCurrent(QList<SegmentPtr>& segments, SegmentPtr& current)
{
    if (current) {
        segments.append(current);
        current.reset();
    }
}

int parseMillisecondsFromDelayLine(const QString& line)
{
    static const QRegularExpression decimalTimecode(QStringLiteral(R"(track number \d+, \d+ frame\(s\), timecode (\d+\.\d+)s)"));
    static const QRegularExpression decimalTimestamp(QStringLiteral(R"(track number \d+, \d+ frame\(s\), timestamp (\d+\.\d+)s)"));
    static const QRegularExpression hmsTimestamp(QStringLiteral(R"(track number \d+, \d+ frame\(s\), timestamp (\d{2}):(\d{2}):(\d{2}).(\d{9}))"));

    QRegularExpressionMatch match = decimalTimecode.match(line);
    if (!match.hasMatch()) {
        match = decimalTimestamp.match(line);
    }
    if (match.hasMatch()) {
        return static_cast<int>(std::llround(match.captured(1).toDouble() * 1000.0));
    }

    match = hmsTimestamp.match(line);
    if (match.hasMatch()) {
        const int hours = match.captured(1).toInt();
        const int minutes = match.captured(2).toInt();
        const int seconds = match.captured(3).toInt();
        const int milliseconds = match.captured(4).left(6).toInt() / 1000;
        return (((hours * 60 + minutes) * 60) + seconds) * 1000 + milliseconds;
    }

    return std::numeric_limits<int>::min();
}

int trackNumberFromDelayLine(const QString& line)
{
    static const QRegularExpression trackNumberRegex(QStringLiteral(R"(track number (\d+),)"));
    const QRegularExpressionMatch match = trackNumberRegex.match(line);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}

QList<std::shared_ptr<Track>> tracksFromSegments(const QList<SegmentPtr>& segments)
{
    QList<std::shared_ptr<Track>> tracks;
    for (const SegmentPtr& segment : segments) {
        if (auto track = std::dynamic_pointer_cast<Track>(segment)) {
            tracks.append(track);
        }
    }
    return tracks;
}

bool hasSegmentInfo(const QList<SegmentPtr>& segments)
{
    for (const SegmentPtr& segment : segments) {
        if (std::dynamic_pointer_cast<SegmentInfo>(segment)) {
            return true;
        }
    }
    return false;
}

bool hasCodecPrivateData(const QList<SegmentPtr>& segments)
{
    for (const SegmentPtr& segment : segments) {
        if (auto track = std::dynamic_pointer_cast<Track>(segment)) {
            if (!track->codecPrivateData.trimmed().isEmpty()) {
                return true;
            }
        }
    }
    return false;
}

QByteArray hexToBytes(const QString& hex)
{
    QByteArray bytes;
    const QString trimmed = hex.trimmed();
    if (trimmed.size() % 2 != 0) {
        return {};
    }

    for (int index = 0; index < trimmed.size(); index += 2) {
        bool ok = false;
        const char value = static_cast<char>(trimmed.mid(index, 2).toUInt(&ok, 16));
        if (!ok) {
            return {};
        }
        bytes.append(value);
    }
    return bytes;
}

QString h264ProfileName(int profileIdc)
{
    switch (profileIdc) {
    case 44: return QStringLiteral("CAVLC 4:4:4 Intra");
    case 66: return QStringLiteral("Baseline");
    case 77: return QStringLiteral("Main");
    case 83: return QStringLiteral("Scalable Baseline");
    case 86: return QStringLiteral("Scalable High");
    case 88: return QStringLiteral("Extended");
    case 100: return QStringLiteral("High");
    case 110: return QStringLiteral("High 10");
    case 118: return QStringLiteral("Multiview High");
    case 122: return QStringLiteral("High 4:2:2");
    case 128: return QStringLiteral("Stereo High");
    case 144: return QStringLiteral("High 4:4:4");
    case 244: return QStringLiteral("High 4:4:4 Predictive");
    default: return QStringLiteral("Unknown");
    }
}

QString codecPrivateText(const Track& track)
{
    if (track.codecPrivateData.trimmed().isEmpty() || !track.codecPrivate.trimmed().isEmpty()) {
        return {};
    }

    const QByteArray bytes = hexToBytes(track.codecPrivateData);
    if (bytes.isEmpty()) {
        return {};
    }

    if (track.trackType == MkvTrackType::Video && track.codecID == QStringLiteral("V_MPEG4/ISO/AVC") && bytes.size() > 3) {
        const int profileIdc = static_cast<unsigned char>(bytes[1]);
        const int levelIdc = static_cast<unsigned char>(bytes[3]);
        return QStringLiteral("length %1 (h.264 profile: %2 @L%3.%4)")
            .arg(bytes.size())
            .arg(h264ProfileName(profileIdc))
            .arg(levelIdc / 10)
            .arg(levelIdc % 10);
    }

    if (track.trackType == MkvTrackType::Audio && track.codecID == QStringLiteral("A_MS/ACM") && bytes.size() > 1) {
        return QStringLiteral("length %1 (format tag: 0x%2%3)")
            .arg(bytes.size())
            .arg(static_cast<unsigned char>(bytes[0]), 2, 16, QLatin1Char('0'))
            .arg(static_cast<unsigned char>(bytes[1]), 2, 16, QLatin1Char('0'));
    }

    return QStringLiteral("length %1").arg(bytes.size());
}

void applyCodecPrivateText(const QList<SegmentPtr>& segments)
{
    for (const SegmentPtr& segment : segments) {
        if (auto track = std::dynamic_pointer_cast<Track>(segment)) {
            const QString value = codecPrivateText(*track);
            if (!value.isEmpty()) {
                track->codecPrivate = value;
            }
        }
    }
}

void ensureProcessSucceeded(const QString& toolName, const ProcessResult& result)
{
    if (result.hasProcessError()) {
        throw std::runtime_error(result.errorString.toStdString());
    }
    if (result.exitCode > 1) {
        QStringList errors = result.standardErrorLines;
        errors.append(result.standardOutputLines);
        throw std::runtime_error(QStringLiteral("%1 exited with error code %2: %3")
            .arg(toolName)
            .arg(result.exitCode)
            .arg(errors.join(QLatin1Char('\n')))
            .toStdString());
    }
}

}

QList<SegmentPtr> MkvInfoParser::parseOutput(const QStringList& outputLines, const QString& inputFile)
{
    QList<SegmentPtr> segments;
    SegmentPtr current;
    ParseState state = ParseState::Searching;
    int attachmentID = 1;

    for (const QString& line : outputLines) {
        if (line.trimmed().isEmpty()) {
            continue;
        }

        if (line.contains(QStringLiteral("Segment information"))) {
            appendCurrent(segments, current);
            current = std::make_shared<SegmentInfo>();
            state = ParseState::SegmentInfo;
            continue;
        }
        if (line.contains(QStringLiteral("Segment tracks"))) {
            state = ParseState::Tracks;
            continue;
        }
        if (line.contains(QStringLiteral("Attachments"))) {
            state = ParseState::Attachments;
            continue;
        }
        if (line.contains(QStringLiteral("Chapters"))) {
            state = ParseState::Chapters;
            continue;
        }

        switch (state) {
        case ParseState::Searching:
            break;
        case ParseState::SegmentInfo: {
            auto info = std::dynamic_pointer_cast<SegmentInfo>(current);
            if (!info) break;
            if (line.contains(QStringLiteral("Timecode scale:"))) info->timecodeScale = valueAfterColon(line);
            else if (line.contains(QStringLiteral("Muxing application:"))) info->muxingApplication = valueAfterColon(line);
            else if (line.contains(QStringLiteral("Writing application:"))) info->writingApplication = valueAfterColon(line);
            else if (line.contains(QStringLiteral("Duration:"))) info->duration = valueAfterColon(line);
            else if (line.contains(QStringLiteral("Date:"))) info->date = valueAfterColon(line);
            break;
        }
        case ParseState::Tracks: {
            if (line.contains(QStringLiteral("+ A track"))) {
                appendCurrent(segments, current);
                current = std::make_shared<Track>();
                break;
            }

            auto track = std::dynamic_pointer_cast<Track>(current);
            if (!track) break;
            if (line.contains(QStringLiteral("Track number:"))) {
                static const QRegularExpression modern(QStringLiteral(R"(Track number:\s*(\d+).*mkvmerge & mkvextract:\s*(\d+)\))"));
                const QRegularExpressionMatch match = modern.match(line);
                if (match.hasMatch()) {
                    track->trackNumber = match.captured(1).toInt();
                    track->trackID = match.captured(2).toInt();
                } else {
                    track->trackNumber = valueAfterColon(line).toInt();
                    track->trackID = track->trackNumber;
                }
            } else if (line.contains(QStringLiteral("Track type:"))) track->trackType = trackTypeFromString(valueAfterColon(line));
            else if (line.contains(QStringLiteral("Codec ID:"))) track->codecID = valueAfterColon(line);
            else if (line.contains(QStringLiteral("Language:"))) track->language = valueAfterColon(line);
            else if (line.contains(QStringLiteral("Name:"))) track->trackName = valueAfterColon(line);
            else if (line.contains(QStringLiteral("Pixel width:"))) {
                track->videoPixelWidth = valueAfterColon(line).toInt();
                track->extraInfo = valueAfterColon(line);
            } else if (line.contains(QStringLiteral("Pixel height:"))) {
                track->videoPixelHeight = valueAfterColon(line).toInt();
                track->extraInfo += QStringLiteral("x") + valueAfterColon(line);
            } else if (line.contains(QStringLiteral("Sampling frequency:"))) {
                track->audioSamplingFrequency = valueAfterColon(line).toInt();
                track->extraInfo = valueAfterColon(line);
            } else if (line.contains(QStringLiteral("Channels:"))) {
                track->audioChannels = valueAfterColon(line).toInt();
                track->extraInfo += QStringLiteral(", Ch:") + valueAfterColon(line);
            } else if (line.contains(QStringLiteral("CodecPrivate,"))) {
                const int commaIndex = line.indexOf(QLatin1Char(','));
                track->codecPrivate = commaIndex < 0 ? QString() : line.mid(commaIndex + 1).trimmed();
            }
            break;
        }
        case ParseState::Attachments: {
            if (line.contains(QStringLiteral("Attached"))) {
                appendCurrent(segments, current);
                auto attachment = std::make_shared<Attachment>();
                attachment->id = attachmentID++;
                current = attachment;
                break;
            }

            auto attachment = std::dynamic_pointer_cast<Attachment>(current);
            if (!attachment) break;
            if (line.contains(QStringLiteral("File name:"))) attachment->filename = valueAfterColon(line);
            else if (line.contains(QStringLiteral("File data, size:"))) attachment->fileSize = valueAfterColon(line);
            else if (line.contains(QStringLiteral("Mime type:"))) attachment->mimeType = valueAfterColon(line);
            break;
        }
        case ParseState::Chapters: {
            if (line.contains(QStringLiteral("EditionEntry"))) {
                appendCurrent(segments, current);
                current = std::make_shared<Chapter>();
                break;
            }

            auto chapter = std::dynamic_pointer_cast<Chapter>(current);
            if (chapter && line.contains(QStringLiteral("ChapterAtom"))) {
                chapter->chapterCount += 1;
            }
            break;
        }
        }
    }

    appendCurrent(segments, current);

    if (!inputFile.isEmpty()) {
        const QFileInfo fileInfo(inputFile);
        for (const SegmentPtr& segment : segments) {
            if (auto info = std::dynamic_pointer_cast<SegmentInfo>(segment)) {
                info->directory = fileInfo.absolutePath();
                info->filename = fileInfo.fileName();
                break;
            }
        }
    }

    return segments;
}

bool MkvInfoParser::findAndSetDelaysFromCheckOutput(const QList<SegmentPtr>& segments, const QStringList& outputLines)
{
    const QList<std::shared_ptr<Track>> tracks = tracksFromSegments(segments);
    bool hasVideo = false;
    for (const auto& track : tracks) {
        hasVideo = hasVideo || track->trackType == MkvTrackType::Video;
    }

    if (!hasVideo) {
        for (const auto& track : tracks) {
            if (track->trackType == MkvTrackType::Audio) {
                track->delay = 0;
                track->effectiveDelay = 0;
            }
        }
        return true;
    }

    int videoDelay = std::numeric_limits<int>::min();
    for (const QString& line : outputLines) {
        const int trackNumber = trackNumberFromDelayLine(line);
        if (trackNumber < 0) {
            continue;
        }

        const int delay = parseMillisecondsFromDelayLine(line);
        if (delay == std::numeric_limits<int>::min()) {
            continue;
        }

        for (const auto& track : tracks) {
            if (track->trackNumber == trackNumber && track->trackType != MkvTrackType::Subtitles) {
                track->delay = delay;
                if (track->trackType == MkvTrackType::Video && videoDelay == std::numeric_limits<int>::min()) {
                    videoDelay = delay;
                }
                break;
            }
        }
    }

    for (const auto& track : tracks) {
        if (track->trackType == MkvTrackType::Subtitles) {
            continue;
        }

        if (track->delay == std::numeric_limits<int>::min()) {
            track->delay = 0;
        }
        track->effectiveDelay = videoDelay == std::numeric_limits<int>::min()
            ? track->delay
            : (track->trackType == MkvTrackType::Video ? videoDelay : track->delay - videoDelay);
    }

    return true;
}

QList<SegmentPtr> SegmentMerger::mergeMkvMergeAndInfoSegments(QList<SegmentPtr> mkvMergeSegments, const QList<SegmentPtr>& mkvInfoSegments)
{
    const bool mergeHasSegmentInfo = hasSegmentInfo(mkvMergeSegments);
    const bool mergeHasCodecPrivateData = hasCodecPrivateData(mkvMergeSegments);

    if (!mergeHasSegmentInfo) {
        for (const SegmentPtr& segment : mkvInfoSegments) {
            if (std::dynamic_pointer_cast<SegmentInfo>(segment)) {
                mkvMergeSegments.prepend(segment);
                break;
            }
        }
    }

    if (!mergeHasCodecPrivateData) {
        const QList<std::shared_ptr<Track>> mergeTracks = tracksFromSegments(mkvMergeSegments);
        const QList<std::shared_ptr<Track>> infoTracks = tracksFromSegments(mkvInfoSegments);
        for (const auto& mergeTrack : mergeTracks) {
            for (const auto& infoTrack : infoTracks) {
                if (mergeTrack->trackID != infoTrack->trackID) {
                    continue;
                }

                if (!infoTrack->codecPrivate.trimmed().isEmpty()) {
                    mergeTrack->codecPrivate = infoTrack->codecPrivate;
                }
                if (mergeTrack->trackType == MkvTrackType::Video) {
                    mergeTrack->videoPixelWidth = std::max(mergeTrack->videoPixelWidth, infoTrack->videoPixelWidth);
                    mergeTrack->videoPixelHeight = std::max(mergeTrack->videoPixelHeight, infoTrack->videoPixelHeight);
                } else if (mergeTrack->trackType == MkvTrackType::Audio) {
                    mergeTrack->audioChannels = std::max(mergeTrack->audioChannels, infoTrack->audioChannels);
                    mergeTrack->audioSamplingFrequency = std::max(mergeTrack->audioSamplingFrequency, infoTrack->audioSamplingFrequency);
                }
                if (!infoTrack->extraInfo.trimmed().isEmpty()) {
                    mergeTrack->extraInfo = infoTrack->extraInfo;
                }
                break;
            }
        }
    }

    MkvMergeParser::findAndSetDelays(mkvMergeSegments);
    applyCodecPrivateText(mkvMergeSegments);

    return mkvMergeSegments;
}

QList<SegmentPtr> SegmentAnalyzer::analyzeFile(const QString& mkvToolNixDirectory, const QString& inputFile)
{
    return analyzeFile(MkvToolNix::toolPathsFromLocation(mkvToolNixDirectory), inputFile);
}

QList<SegmentPtr> SegmentAnalyzer::analyzeFile(const MkvToolPaths& toolPaths, const QString& inputFile)
{
    if (!toolPaths.isValid()) {
        throw std::runtime_error("Could not find mkvmerge, mkvinfo, and mkvextract.");
    }

    const QString mkvmerge = toolPaths.path(MkvTool::Merge);
    const QString mkvinfo = toolPaths.path(MkvTool::Info);

    const ProcessResult mergeResult = ProcessRunner::run(
        mkvmerge,
        {
            QStringLiteral("--identify"),
            QStringLiteral("--identification-format"),
            QStringLiteral("json"),
            QStringLiteral("--ui-language"),
            MkvToolNix::uiLanguageCode(),
            inputFile,
        });
    Logger::log(QStringLiteral("\"%1\" --identify --identification-format json --ui-language %2 \"%3\"")
        .arg(mkvmerge, MkvToolNix::uiLanguageCode(), inputFile));
    ensureProcessSucceeded(QStringLiteral("mkvmerge"), mergeResult);

    QList<SegmentPtr> mergedSegments = MkvMergeParser::parseJsonOutput(mergeResult.standardOutputLines.join(QLatin1Char('\n')), inputFile);

    if (!hasSegmentInfo(mergedSegments) || !hasCodecPrivateData(mergedSegments)) {
        const ProcessResult infoResult = ProcessRunner::run(
            mkvinfo,
            {
                QStringLiteral("--ui-language"),
                MkvToolNix::uiLanguageCode(),
                inputFile,
            });
        Logger::log(QStringLiteral("\"%1\" --ui-language %2 \"%3\"").arg(mkvinfo, MkvToolNix::uiLanguageCode(), inputFile));
        ensureProcessSucceeded(QStringLiteral("mkvinfo"), infoResult);

        QStringList infoLines = infoResult.standardOutputLines;
        infoLines.append(infoResult.standardErrorLines);
        mergedSegments = SegmentMerger::mergeMkvMergeAndInfoSegments(
            mergedSegments,
            MkvInfoParser::parseOutput(infoLines, inputFile));
    } else {
        MkvMergeParser::findAndSetDelays(mergedSegments);
        applyCodecPrivateText(mergedSegments);
    }

    bool delaysKnown = true;
    for (const auto& track : tracksFromSegments(mergedSegments)) {
        if (track->trackType != MkvTrackType::Subtitles && track->delay == std::numeric_limits<int>::min()) {
            delaysKnown = false;
            break;
        }
    }

    if (!delaysKnown) {
        const ProcessResult checkResult = ProcessRunner::run(
            mkvinfo,
            {
                QStringLiteral("--check-mode"),
                QStringLiteral("--ui-language"),
                MkvToolNix::uiLanguageCode(),
                inputFile,
            });
        Logger::log(QStringLiteral("\"%1\" --check-mode --ui-language %2 \"%3\"").arg(mkvinfo, MkvToolNix::uiLanguageCode(), inputFile));
        ensureProcessSucceeded(QStringLiteral("mkvinfo"), checkResult);

        QStringList checkLines = checkResult.standardOutputLines;
        checkLines.append(checkResult.standardErrorLines);
        MkvInfoParser::findAndSetDelaysFromCheckOutput(mergedSegments, checkLines);
    }

    return mergedSegments;
}

}
