#include "gmkvtoolnix/MkvMerge.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>

#include <limits>
#include <stdexcept>

namespace gmkv {

namespace {

MkvTrackType trackTypeFromString(const QString& value)
{
    if (value.compare(QStringLiteral("audio"), Qt::CaseInsensitive) == 0) {
        return MkvTrackType::Audio;
    }
    if (value.compare(QStringLiteral("subtitles"), Qt::CaseInsensitive) == 0) {
        return MkvTrackType::Subtitles;
    }

    return MkvTrackType::Video;
}

QString formatDateUtc(const QString& value)
{
    QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
    if (!dateTime.isValid()) {
        dateTime = QDateTime::fromString(value, QStringLiteral("yyyyMMddTHHmmssZ"));
    }
    if (!dateTime.isValid()) {
        return {};
    }

    return QLocale::c().toString(dateTime.toUTC(), QStringLiteral("ddd MMM dd HH:mm:ss yyyy 'UTC'"));
}

QString formatDuration(const QJsonValue& value)
{
    const QString durationText = value.isString()
        ? value.toString()
        : QString::number(static_cast<qint64>(value.toDouble()));

    bool ok = false;
    const qint64 durationNs = durationText.toLongLong(&ok);
    if (!ok) {
        return {};
    }

    const double seconds = static_cast<double>(durationNs) / 1000000000.0;
    const qint64 milliseconds = static_cast<qint64>(static_cast<double>(durationNs) / 1000000.0);
    const qint64 hours = (milliseconds / 3600000) % 24;
    const qint64 minutes = (milliseconds / 60000) % 60;
    const qint64 secs = (milliseconds / 1000) % 60;
    const qint64 millis = milliseconds % 1000;

    return QStringLiteral("%1s (%2:%3:%4.%5)")
        .arg(QString::number(seconds, 'f', 3))
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0'))
        .arg(millis, 3, 10, QLatin1Char('0'));
}

int jsonInt(const QJsonObject& object, const QString& key, int fallback = 0)
{
    const QJsonValue value = object.value(key);
    if (value.isDouble()) {
        return value.toInt(fallback);
    }

    bool ok = false;
    const int parsed = value.toString().toInt(&ok);
    return ok ? parsed : fallback;
}

qint64 jsonInt64(const QJsonObject& object, const QString& key, qint64 fallback = std::numeric_limits<qint64>::min())
{
    const QJsonValue value = object.value(key);
    if (value.isDouble()) {
        return static_cast<qint64>(value.toDouble());
    }

    bool ok = false;
    const qint64 parsed = value.toString().toLongLong(&ok);
    return ok ? parsed : fallback;
}

void applyInputFile(SegmentInfo& info, const QString& inputFile)
{
    if (inputFile.isEmpty()) {
        return;
    }

    const QFileInfo fileInfo(inputFile);
    info.directory = fileInfo.absolutePath();
    info.filename = fileInfo.fileName();
}

}

QList<SegmentPtr> MkvMergeParser::parseJsonOutput(const QString& output, const QString& inputFile)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(output.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        throw std::runtime_error(QStringLiteral("Invalid mkvmerge JSON output.").toStdString());
    }

    const QJsonObject root = document.object();
    QList<SegmentPtr> finalList;
    QList<SegmentPtr> tracks;
    QList<SegmentPtr> attachments;
    QList<SegmentPtr> chapters;

    const QJsonObject container = root.value(QStringLiteral("container")).toObject();
    if (!container.isEmpty()) {
        if (container.contains(QStringLiteral("recognized")) && !container.value(QStringLiteral("recognized")).toBool()) {
            throw std::runtime_error("The container of the file was not recognized.");
        }
        if (container.contains(QStringLiteral("supported")) && !container.value(QStringLiteral("supported")).toBool()) {
            throw std::runtime_error("The container of the file is not supported.");
        }

        auto segmentInfo = std::make_shared<SegmentInfo>();
        const QJsonObject properties = container.value(QStringLiteral("properties")).toObject();
        segmentInfo->date = formatDateUtc(properties.value(QStringLiteral("date_utc")).toString());
        segmentInfo->duration = formatDuration(properties.value(QStringLiteral("duration")));
        segmentInfo->muxingApplication = properties.value(QStringLiteral("muxing_application")).toString();
        segmentInfo->writingApplication = properties.value(QStringLiteral("writing_application")).toString();
        applyInputFile(*segmentInfo, inputFile);
        finalList.append(segmentInfo);
    }

    const QJsonArray trackArray = root.value(QStringLiteral("tracks")).toArray();
    for (const QJsonValue& trackValue : trackArray) {
        const QJsonObject trackObject = trackValue.toObject();
        const QJsonObject properties = trackObject.value(QStringLiteral("properties")).toObject();
        auto track = std::make_shared<Track>();

        track->trackID = jsonInt(trackObject, QStringLiteral("id"));
        track->trackType = trackTypeFromString(trackObject.value(QStringLiteral("type")).toString());
        track->codecID = properties.value(QStringLiteral("codec_id")).toString();
        track->codecPrivateData = properties.value(QStringLiteral("codec_private_data")).toString();
        track->trackName = properties.value(QStringLiteral("track_name")).toString();
        track->forced = properties.value(QStringLiteral("forced_track")).toBool(false);
        track->language = properties.value(QStringLiteral("language")).toString();
        track->languageIetf = properties.value(QStringLiteral("language_ietf")).toString();
        track->minimumTimestamp = jsonInt64(properties, QStringLiteral("minimum_timestamp"));
        track->trackNumber = jsonInt(properties, QStringLiteral("number"));

        const QString videoDimensions = properties.value(QStringLiteral("pixel_dimensions")).toString();
        if (!videoDimensions.trimmed().isEmpty()) {
            track->extraInfo = videoDimensions;
            const QStringList dimensions = videoDimensions.split(QLatin1Char('x'));
            if (dimensions.size() == 2) {
                track->videoPixelWidth = dimensions[0].toInt();
                track->videoPixelHeight = dimensions[1].toInt();
            }
        }

        const QString audioChannels = properties.value(QStringLiteral("audio_channels")).toString();
        const QString audioFrequency = properties.value(QStringLiteral("audio_sampling_frequency")).toString();
        if (!audioChannels.trimmed().isEmpty() || !audioFrequency.trimmed().isEmpty()) {
            if (!audioChannels.trimmed().isEmpty() && !audioFrequency.trimmed().isEmpty()) {
                track->extraInfo = QStringLiteral("%1Hz, Ch: %2").arg(audioFrequency, audioChannels);
            }
            track->audioChannels = audioChannels.toInt();
            track->audioSamplingFrequency = audioFrequency.toInt();
        }

        tracks.append(track);
    }

    const QJsonArray attachmentArray = root.value(QStringLiteral("attachments")).toArray();
    for (const QJsonValue& attachmentValue : attachmentArray) {
        const QJsonObject object = attachmentValue.toObject();
        auto attachment = std::make_shared<Attachment>();
        attachment->id = jsonInt(object, QStringLiteral("id"));
        attachment->filename = object.value(QStringLiteral("file_name")).toString();
        attachment->mimeType = object.value(QStringLiteral("content_type")).toString();
        attachment->fileSize = object.value(QStringLiteral("size")).isString()
            ? object.value(QStringLiteral("size")).toString()
            : QString::number(static_cast<qint64>(object.value(QStringLiteral("size")).toDouble()));
        attachments.append(attachment);
    }

    const QJsonArray chapterArray = root.value(QStringLiteral("chapters")).toArray();
    if (!chapterArray.isEmpty()) {
        int chapterCount = 0;
        for (const QJsonValue& chapterValue : chapterArray) {
            if (chapterValue.isObject()) {
                chapterCount += jsonInt(chapterValue.toObject(), QStringLiteral("num_entries"), 1);
            } else if (chapterValue.isDouble()) {
                chapterCount += chapterValue.toInt();
            } else {
                chapterCount++;
            }
        }

        auto chapter = std::make_shared<Chapter>();
        chapter->chapterCount = chapterCount;
        chapters.append(chapter);
    }

    finalList.append(tracks);
    finalList.append(attachments);
    finalList.append(chapters);
    return finalList;
}

bool MkvMergeParser::findAndSetDelays(const QList<SegmentPtr>& segments)
{
    QList<std::shared_ptr<Track>> tracks;
    for (const SegmentPtr& segment : segments) {
        if (auto track = std::dynamic_pointer_cast<Track>(segment)) {
            tracks.append(track);
        }
    }

    bool hasVideo = false;
    for (const auto& track : tracks) {
        if (track->trackType == MkvTrackType::Video) {
            hasVideo = true;
            break;
        }
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
    for (const auto& track : tracks) {
        if (track->trackType == MkvTrackType::Video) {
            if (track->minimumTimestamp == std::numeric_limits<qint64>::min()) {
                return false;
            }
            videoDelay = static_cast<int>(track->minimumTimestamp / 1000000);
            track->delay = videoDelay;
            track->effectiveDelay = videoDelay;
            break;
        }
    }

    if (videoDelay == std::numeric_limits<int>::min()) {
        return false;
    }

    for (const auto& track : tracks) {
        if (track->trackType == MkvTrackType::Audio) {
            if (track->minimumTimestamp == std::numeric_limits<qint64>::min()) {
                return false;
            }
            track->delay = static_cast<int>(track->minimumTimestamp / 1000000);
            track->effectiveDelay = track->delay - videoDelay;
        }
    }

    return true;
}

}
