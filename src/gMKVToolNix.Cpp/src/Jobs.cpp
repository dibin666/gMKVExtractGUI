#include "gmkvtoolnix/Jobs.h"

#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>
#include <stdexcept>

namespace gmkv {

namespace {

bool stringEquals(const QString& left, const QString& right)
{
    return left.compare(right, Qt::CaseInsensitive) == 0;
}

bool boolFromString(const QString& value)
{
    return value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
}

QString boolString(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString trackTypeString(MkvTrackType trackType)
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

void writeTextElement(QXmlStreamWriter& writer, const QString& name, const QString& value)
{
    writer.writeTextElement(name, value);
}

void writeTextElement(QXmlStreamWriter& writer, const QString& name, int value)
{
    writer.writeTextElement(name, QString::number(value));
}

void writeTextElement(QXmlStreamWriter& writer, const QString& name, qint64 value)
{
    writer.writeTextElement(name, QString::number(value));
}

void writeSegment(QXmlStreamWriter& writer, const SegmentPtr& segment)
{
    if (const auto track = std::dynamic_pointer_cast<Track>(segment)) {
        writer.writeStartElement(QStringLiteral("Segment"));
        writer.writeAttribute(QStringLiteral("Type"), QStringLiteral("Track"));
        writeTextElement(writer, QStringLiteral("TrackNumber"), track->trackNumber);
        writeTextElement(writer, QStringLiteral("TrackID"), track->trackID);
        writeTextElement(writer, QStringLiteral("TrackType"), trackTypeString(track->trackType));
        writeTextElement(writer, QStringLiteral("CodecID"), track->codecID);
        writeTextElement(writer, QStringLiteral("CodecPrivate"), track->codecPrivate);
        writeTextElement(writer, QStringLiteral("CodecPrivateData"), track->codecPrivateData);
        writeTextElement(writer, QStringLiteral("Forced"), boolString(track->forced));
        writeTextElement(writer, QStringLiteral("Language"), track->language);
        writeTextElement(writer, QStringLiteral("LanguageIetf"), track->languageIetf);
        writeTextElement(writer, QStringLiteral("TrackName"), track->trackName);
        writeTextElement(writer, QStringLiteral("ExtraInfo"), track->extraInfo);
        writeTextElement(writer, QStringLiteral("Delay"), track->delay);
        writeTextElement(writer, QStringLiteral("EffectiveDelay"), track->effectiveDelay);
        writeTextElement(writer, QStringLiteral("MinimumTimestamp"), track->minimumTimestamp);
        writeTextElement(writer, QStringLiteral("VideoPixelWidth"), track->videoPixelWidth);
        writeTextElement(writer, QStringLiteral("VideoPixelHeight"), track->videoPixelHeight);
        writeTextElement(writer, QStringLiteral("AudioSamplingFrequency"), track->audioSamplingFrequency);
        writeTextElement(writer, QStringLiteral("AudioChannels"), track->audioChannels);
        writer.writeEndElement();
    } else if (const auto attachment = std::dynamic_pointer_cast<Attachment>(segment)) {
        writer.writeStartElement(QStringLiteral("Segment"));
        writer.writeAttribute(QStringLiteral("Type"), QStringLiteral("Attachment"));
        writeTextElement(writer, QStringLiteral("ID"), attachment->id);
        writeTextElement(writer, QStringLiteral("Filename"), attachment->filename);
        writeTextElement(writer, QStringLiteral("MimeType"), attachment->mimeType);
        writeTextElement(writer, QStringLiteral("FileSize"), attachment->fileSize);
        writer.writeEndElement();
    } else if (const auto chapter = std::dynamic_pointer_cast<Chapter>(segment)) {
        writer.writeStartElement(QStringLiteral("Segment"));
        writer.writeAttribute(QStringLiteral("Type"), QStringLiteral("Chapter"));
        writeTextElement(writer, QStringLiteral("ChapterCount"), chapter->chapterCount);
        writer.writeEndElement();
    }
}

QString segmentType(QXmlStreamReader& reader)
{
    QString type = reader.attributes().value(QStringLiteral("Type")).toString();
    if (type.isEmpty()) {
        type = reader.attributes().value(QStringLiteral("type")).toString();
    }
    if (type.isEmpty()) {
        type = reader.attributes().value(QStringLiteral("xsi:type")).toString();
    }
    if (type.isEmpty()) {
        type = reader.name().toString();
    }
    return type;
}

SegmentPtr readTrack(QXmlStreamReader& reader)
{
    auto track = std::make_shared<Track>();
    while (reader.readNextStartElement()) {
        const QString name = reader.name().toString();
        const QString value = reader.readElementText();
        if (name == QStringLiteral("TrackNumber")) track->trackNumber = value.toInt();
        else if (name == QStringLiteral("TrackID")) track->trackID = value.toInt();
        else if (name == QStringLiteral("TrackType")) track->trackType = trackTypeFromString(value);
        else if (name == QStringLiteral("CodecID")) track->codecID = value;
        else if (name == QStringLiteral("CodecPrivate")) track->codecPrivate = value;
        else if (name == QStringLiteral("CodecPrivateData")) track->codecPrivateData = value;
        else if (name == QStringLiteral("Forced")) track->forced = boolFromString(value);
        else if (name == QStringLiteral("Language")) track->language = value;
        else if (name == QStringLiteral("LanguageIetf")) track->languageIetf = value;
        else if (name == QStringLiteral("TrackName")) track->trackName = value;
        else if (name == QStringLiteral("ExtraInfo")) track->extraInfo = value;
        else if (name == QStringLiteral("Delay")) track->delay = value.toInt();
        else if (name == QStringLiteral("EffectiveDelay")) track->effectiveDelay = value.toInt();
        else if (name == QStringLiteral("MinimumTimestamp")) track->minimumTimestamp = value.toLongLong();
        else if (name == QStringLiteral("VideoPixelWidth")) track->videoPixelWidth = value.toInt();
        else if (name == QStringLiteral("VideoPixelHeight")) track->videoPixelHeight = value.toInt();
        else if (name == QStringLiteral("AudioSamplingFrequency")) track->audioSamplingFrequency = value.toInt();
        else if (name == QStringLiteral("AudioChannels")) track->audioChannels = value.toInt();
    }
    return track;
}

SegmentPtr readAttachment(QXmlStreamReader& reader)
{
    auto attachment = std::make_shared<Attachment>();
    while (reader.readNextStartElement()) {
        const QString name = reader.name().toString();
        const QString value = reader.readElementText();
        if (name == QStringLiteral("ID")) attachment->id = value.toInt();
        else if (name == QStringLiteral("Filename")) attachment->filename = value;
        else if (name == QStringLiteral("MimeType")) attachment->mimeType = value;
        else if (name == QStringLiteral("FileSize")) attachment->fileSize = value;
    }
    return attachment;
}

SegmentPtr readChapter(QXmlStreamReader& reader)
{
    auto chapter = std::make_shared<Chapter>();
    while (reader.readNextStartElement()) {
        const QString name = reader.name().toString();
        const QString value = reader.readElementText();
        if (name == QStringLiteral("ChapterCount")) {
            chapter->chapterCount = value.toInt();
        }
    }
    return chapter;
}

SegmentPtr readSegment(QXmlStreamReader& reader)
{
    const QString type = segmentType(reader);
    if (type.contains(QStringLiteral("Track"), Qt::CaseInsensitive)) {
        return readTrack(reader);
    }
    if (type.contains(QStringLiteral("Attachment"), Qt::CaseInsensitive)) {
        return readAttachment(reader);
    }
    if (type.contains(QStringLiteral("Chapter"), Qt::CaseInsensitive)) {
        return readChapter(reader);
    }

    reader.skipCurrentElement();
    return {};
}

void writeFilenamePatterns(QXmlStreamWriter& writer, const FilenamePatterns& patterns)
{
    writer.writeStartElement(QStringLiteral("FilenamePatterns"));
    writeTextElement(writer, QStringLiteral("VideoTrackFilenamePattern"), patterns.videoTrackFilenamePattern);
    writeTextElement(writer, QStringLiteral("AudioTrackFilenamePattern"), patterns.audioTrackFilenamePattern);
    writeTextElement(writer, QStringLiteral("SubtitleTrackFilenamePattern"), patterns.subtitleTrackFilenamePattern);
    writeTextElement(writer, QStringLiteral("ChapterFilenamePattern"), patterns.chapterFilenamePattern);
    writeTextElement(writer, QStringLiteral("AttachmentFilenamePattern"), patterns.attachmentFilenamePattern);
    writeTextElement(writer, QStringLiteral("TagsFilenamePattern"), patterns.tagsFilenamePattern);
    writer.writeEndElement();
}

void readFilenamePatterns(QXmlStreamReader& reader, FilenamePatterns& patterns)
{
    while (reader.readNextStartElement()) {
        const QString name = reader.name().toString();
        const QString value = reader.readElementText();
        if (name == QStringLiteral("VideoTrackFilenamePattern")) patterns.videoTrackFilenamePattern = value;
        else if (name == QStringLiteral("AudioTrackFilenamePattern")) patterns.audioTrackFilenamePattern = value;
        else if (name == QStringLiteral("SubtitleTrackFilenamePattern")) patterns.subtitleTrackFilenamePattern = value;
        else if (name == QStringLiteral("ChapterFilenamePattern")) patterns.chapterFilenamePattern = value;
        else if (name == QStringLiteral("AttachmentFilenamePattern")) patterns.attachmentFilenamePattern = value;
        else if (name == QStringLiteral("TagsFilenamePattern")) patterns.tagsFilenamePattern = value;
    }
}

void writeParameters(QXmlStreamWriter& writer, const ExtractSegmentsParameters& parameters)
{
    writer.writeStartElement(QStringLiteral("ParametersList"));
    writeTextElement(writer, QStringLiteral("MKVFile"), parameters.mkvFile);
    writer.writeStartElement(QStringLiteral("MKVSegmentsToExtract"));
    for (const SegmentPtr& segment : parameters.segmentsToExtract) {
        writeSegment(writer, segment);
    }
    writer.writeEndElement();
    writeTextElement(writer, QStringLiteral("OutputDirectory"), parameters.outputDirectory);
    writeTextElement(writer, QStringLiteral("ChapterType"), toSettingsString(parameters.chapterType));
    writeTextElement(writer, QStringLiteral("TimecodesExtractionMode"), parameters.timecodesExtractionMode == TimecodesExtractionMode::OnlyTimecodes ? QStringLiteral("OnlyTimecodes") : parameters.timecodesExtractionMode == TimecodesExtractionMode::WithTimecodes ? QStringLiteral("WithTimecodes") : QStringLiteral("NoTimecodes"));
    writeTextElement(writer, QStringLiteral("CueExtractionMode"), parameters.cueExtractionMode == CuesExtractionMode::OnlyCues ? QStringLiteral("OnlyCues") : parameters.cueExtractionMode == CuesExtractionMode::WithCues ? QStringLiteral("WithCues") : QStringLiteral("NoCues"));
    writeFilenamePatterns(writer, parameters.filenamePatterns);
    writeTextElement(writer, QStringLiteral("DisableBomForTextFiles"), boolString(parameters.disableBomForTextFiles));
    writeTextElement(writer, QStringLiteral("UseRawExtractionMode"), boolString(parameters.useRawExtractionMode));
    writeTextElement(writer, QStringLiteral("UseFullRawExtractionMode"), boolString(parameters.useFullRawExtractionMode));
    writeTextElement(writer, QStringLiteral("OverwriteExistingFile"), boolString(parameters.overwriteExistingFile));
    writer.writeEndElement();
}

void readSegments(QXmlStreamReader& reader, ExtractSegmentsParameters& parameters)
{
    while (reader.readNextStartElement()) {
        SegmentPtr segment = readSegment(reader);
        if (segment) {
            parameters.segmentsToExtract.append(segment);
        }
    }
}

ExtractSegmentsParameters readParameters(QXmlStreamReader& reader)
{
    ExtractSegmentsParameters parameters;
    while (reader.readNextStartElement()) {
        const QString name = reader.name().toString();
        if (name == QStringLiteral("MKVSegmentsToExtract")) {
            readSegments(reader, parameters);
            continue;
        }
        if (name == QStringLiteral("FilenamePatterns")) {
            readFilenamePatterns(reader, parameters.filenamePatterns);
            continue;
        }

        const QString value = reader.readElementText();
        if (name == QStringLiteral("MKVFile")) parameters.mkvFile = value;
        else if (name == QStringLiteral("OutputDirectory")) parameters.outputDirectory = value;
        else if (name == QStringLiteral("ChapterType")) parameters.chapterType = chapterTypeFromString(value);
        else if (name == QStringLiteral("TimecodesExtractionMode")) {
            parameters.timecodesExtractionMode = value == QStringLiteral("OnlyTimecodes") ? TimecodesExtractionMode::OnlyTimecodes : value == QStringLiteral("WithTimecodes") ? TimecodesExtractionMode::WithTimecodes : TimecodesExtractionMode::NoTimecodes;
        } else if (name == QStringLiteral("CueExtractionMode")) {
            parameters.cueExtractionMode = value == QStringLiteral("OnlyCues") ? CuesExtractionMode::OnlyCues : value == QStringLiteral("WithCues") ? CuesExtractionMode::WithCues : CuesExtractionMode::NoCues;
        } else if (name == QStringLiteral("DisableBomForTextFiles")) parameters.disableBomForTextFiles = boolFromString(value);
        else if (name == QStringLiteral("UseRawExtractionMode")) parameters.useRawExtractionMode = boolFromString(value);
        else if (name == QStringLiteral("UseFullRawExtractionMode")) parameters.useFullRawExtractionMode = boolFromString(value);
        else if (name == QStringLiteral("OverwriteExistingFile")) parameters.overwriteExistingFile = boolFromString(value);
    }
    return parameters;
}

void writeJob(QXmlStreamWriter& writer, const Job& job)
{
    writer.writeStartElement(QStringLiteral("Job"));
    writeTextElement(writer, QStringLiteral("ExtractionMode"), toString(job.extractionMode));
    writeTextElement(writer, QStringLiteral("MKVToolnixPath"), job.mkvToolNixPath);
    writeParameters(writer, job.parameters);
    writer.writeEndElement();
}

Job readJob(QXmlStreamReader& reader)
{
    Job job;
    while (reader.readNextStartElement()) {
        const QString name = reader.name().toString();
        if (name == QStringLiteral("ParametersList")) {
            job.parameters = readParameters(reader);
            continue;
        }
        const QString value = reader.readElementText();
        if (name == QStringLiteral("ExtractionMode")) job.extractionMode = extractionModeFromString(value);
        else if (name == QStringLiteral("MKVToolnixPath")) job.mkvToolNixPath = value;
    }
    return job;
}

void writeJobInfo(QXmlStreamWriter& writer, const JobInfo& jobInfo)
{
    writer.writeStartElement(QStringLiteral("gMKVJobInfo"));
    writeJob(writer, jobInfo.job);
    if (jobInfo.startTime.isValid()) {
        writeTextElement(writer, QStringLiteral("StartTime"), jobInfo.startTime.toUTC().toString(Qt::ISODateWithMs));
    }
    if (jobInfo.endTime.isValid()) {
        writeTextElement(writer, QStringLiteral("EndTime"), jobInfo.endTime.toUTC().toString(Qt::ISODateWithMs));
    }
    writeTextElement(writer, QStringLiteral("State"), toString(jobInfo.state));
    writer.writeEndElement();
}

JobInfo readJobInfo(QXmlStreamReader& reader)
{
    JobInfo jobInfo;
    while (reader.readNextStartElement()) {
        const QString name = reader.name().toString();
        if (name == QStringLiteral("Job")) {
            jobInfo.job = readJob(reader);
            continue;
        }
        const QString value = reader.readElementText();
        if (name == QStringLiteral("StartTime")) jobInfo.startTime = QDateTime::fromString(value, Qt::ISODate);
        else if (name == QStringLiteral("EndTime")) jobInfo.endTime = QDateTime::fromString(value, Qt::ISODate);
        else if (name == QStringLiteral("State")) jobInfo.state = jobStateFromString(value);
    }
    return jobInfo;
}

QString tracksSummary(const QList<SegmentPtr>& segments)
{
    QString summary;
    for (const SegmentPtr& segment : segments) {
        if (const auto track = std::dynamic_pointer_cast<Track>(segment)) {
            summary += QStringLiteral("[%1:%2]").arg(trackTypeString(track->trackType).left(3), QString::number(track->trackID));
        } else if (const auto attachment = std::dynamic_pointer_cast<Attachment>(segment)) {
            summary += QStringLiteral("[Att:%1]").arg(attachment->id);
        } else if (std::dynamic_pointer_cast<Chapter>(segment)) {
            summary += QStringLiteral("[Chap]");
        }
    }
    return summary;
}

bool segmentsEqual(const SegmentPtr& left, const SegmentPtr& right)
{
    if (const auto leftTrack = std::dynamic_pointer_cast<Track>(left)) {
        const auto rightTrack = std::dynamic_pointer_cast<Track>(right);
        return rightTrack
            && leftTrack->trackID == rightTrack->trackID
            && leftTrack->trackNumber == rightTrack->trackNumber
            && leftTrack->trackType == rightTrack->trackType
            && stringEquals(leftTrack->codecID, rightTrack->codecID)
            && stringEquals(leftTrack->language, rightTrack->language);
    }
    if (const auto leftAttachment = std::dynamic_pointer_cast<Attachment>(left)) {
        const auto rightAttachment = std::dynamic_pointer_cast<Attachment>(right);
        return rightAttachment
            && leftAttachment->id == rightAttachment->id
            && stringEquals(leftAttachment->filename, rightAttachment->filename);
    }
    return std::dynamic_pointer_cast<Chapter>(left) && std::dynamic_pointer_cast<Chapter>(right);
}

}

QString Job::toString() const
{
    const QString fileName = QFileInfo(parameters.mkvFile).fileName();
    const QString tracks = tracksSummary(parameters.segmentsToExtract);
    const QString mode = [&]() {
        switch (extractionMode) {
        case FormMkvExtractionMode::Tracks: return QStringLiteral("Tracks %1").arg(tracks);
        case FormMkvExtractionMode::CueSheet: return QStringLiteral("Cue Sheet");
        case FormMkvExtractionMode::Tags: return QStringLiteral("Tags");
        case FormMkvExtractionMode::Timecodes: return QStringLiteral("Timecodes %1").arg(tracks);
        case FormMkvExtractionMode::TracksAndTimecodes: return QStringLiteral("Tracks/Timecodes %1").arg(tracks);
        case FormMkvExtractionMode::Cues: return QStringLiteral("Cues %1").arg(tracks);
        case FormMkvExtractionMode::TracksAndCues: return QStringLiteral("Tracks/Cues %1").arg(tracks);
        case FormMkvExtractionMode::TracksAndCuesAndTimecodes: return QStringLiteral("Tracks/Cues/Timecodes %1").arg(tracks);
        }
        return QStringLiteral("Unknown job!!!");
    }();

    return QStringLiteral("%1 \r\n%2 \r\n%3").arg(mode, fileName, parameters.outputDirectory);
}

void JobInfo::reset()
{
    startTime = {};
    endTime = {};
    state = JobState::Ready;
}

QString toString(FormMkvExtractionMode extractionMode)
{
    switch (extractionMode) {
    case FormMkvExtractionMode::Tracks: return QStringLiteral("Tracks");
    case FormMkvExtractionMode::CueSheet: return QStringLiteral("Cue_Sheet");
    case FormMkvExtractionMode::Tags: return QStringLiteral("Tags");
    case FormMkvExtractionMode::Timecodes: return QStringLiteral("Timecodes");
    case FormMkvExtractionMode::TracksAndTimecodes: return QStringLiteral("Tracks_And_Timecodes");
    case FormMkvExtractionMode::Cues: return QStringLiteral("Cues");
    case FormMkvExtractionMode::TracksAndCues: return QStringLiteral("Tracks_And_Cues");
    case FormMkvExtractionMode::TracksAndCuesAndTimecodes: return QStringLiteral("Tracks_And_Cues_And_Timecodes");
    }
    return QStringLiteral("Tracks");
}

FormMkvExtractionMode extractionModeFromString(const QString& value, FormMkvExtractionMode fallback)
{
    for (const FormMkvExtractionMode mode : {
             FormMkvExtractionMode::Tracks,
             FormMkvExtractionMode::CueSheet,
             FormMkvExtractionMode::Tags,
             FormMkvExtractionMode::Timecodes,
             FormMkvExtractionMode::TracksAndTimecodes,
             FormMkvExtractionMode::Cues,
             FormMkvExtractionMode::TracksAndCues,
             FormMkvExtractionMode::TracksAndCuesAndTimecodes,
         }) {
        if (toString(mode).compare(value, Qt::CaseInsensitive) == 0) {
            return mode;
        }
    }
    return fallback;
}

QString toString(JobState state)
{
    switch (state) {
    case JobState::Ready: return QStringLiteral("Ready");
    case JobState::Pending: return QStringLiteral("Pending");
    case JobState::Running: return QStringLiteral("Running");
    case JobState::Completed: return QStringLiteral("Completed");
    case JobState::Failed: return QStringLiteral("Failed");
    }
    return QStringLiteral("Ready");
}

JobState jobStateFromString(const QString& value, JobState fallback)
{
    for (const JobState state : { JobState::Ready, JobState::Pending, JobState::Running, JobState::Completed, JobState::Failed }) {
        if (toString(state).compare(value, Qt::CaseInsensitive) == 0) {
            return state;
        }
    }
    return fallback;
}

bool jobsEqual(const Job& left, const Job& right)
{
    if (left.extractionMode != right.extractionMode
        || !stringEquals(left.mkvToolNixPath, right.mkvToolNixPath)
        || !stringEquals(left.parameters.mkvFile, right.parameters.mkvFile)
        || !stringEquals(left.parameters.outputDirectory, right.parameters.outputDirectory)
        || left.parameters.chapterType != right.parameters.chapterType
        || left.parameters.timecodesExtractionMode != right.parameters.timecodesExtractionMode
        || left.parameters.cueExtractionMode != right.parameters.cueExtractionMode
        || left.parameters.segmentsToExtract.size() != right.parameters.segmentsToExtract.size()) {
        return false;
    }

    QList<bool> matched(right.parameters.segmentsToExtract.size(), false);
    for (const SegmentPtr& leftSegment : left.parameters.segmentsToExtract) {
        bool found = false;
        for (qsizetype index = 0; index < right.parameters.segmentsToExtract.size(); ++index) {
            if (!matched[index] && segmentsEqual(leftSegment, right.parameters.segmentsToExtract[index])) {
                matched[index] = true;
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

bool JobQueue::addJob(const JobInfo& jobInfo)
{
    const bool exists = std::any_of(m_jobs.cbegin(), m_jobs.cend(), [&](const JobInfo& existing) {
        return jobsEqual(existing.job, jobInfo.job);
    });
    if (exists) {
        return false;
    }

    m_jobs.append(jobInfo);
    return true;
}

QList<JobInfo> JobQueue::jobs() const
{
    return m_jobs;
}

void JobQueue::setJobs(QList<JobInfo> jobs)
{
    m_jobs = std::move(jobs);
}

void JobQueue::clear()
{
    m_jobs.clear();
}

void JobXmlService::save(const QList<JobInfo>& jobs, const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        throw std::runtime_error(file.errorString().toStdString());
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(QStringLiteral("ArrayOfGMKVJobInfo"));
    writer.writeDefaultNamespace(QStringLiteral("http://www.gmkvextractgui.org/jobs"));
    writer.writeNamespace(QStringLiteral("http://www.w3.org/2001/XMLSchema-instance"), QStringLiteral("xsi"));
    writer.writeNamespace(QStringLiteral("http://www.w3.org/2001/XMLSchema"), QStringLiteral("xsd"));
    for (const JobInfo& job : jobs) {
        writeJobInfo(writer, job);
    }
    writer.writeEndElement();
    writer.writeEndDocument();
}

QList<JobInfo> JobXmlService::load(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error(file.errorString().toStdString());
    }

    QXmlStreamReader reader(&file);
    QList<JobInfo> jobs;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name() == QStringLiteral("gMKVJobInfo")) {
            jobs.append(readJobInfo(reader));
        }
    }

    if (reader.hasError()) {
        throw std::runtime_error(reader.errorString().toStdString());
    }
    return jobs;
}

}
