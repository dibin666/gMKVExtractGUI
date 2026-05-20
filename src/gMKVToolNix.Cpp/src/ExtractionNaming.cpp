#include "gmkvtoolnix/ExtractionNaming.h"

#include "gmkvtoolnix/FileName.h"

#include <QDir>
#include <QFileInfo>

#include <stdexcept>

namespace gmkv {

namespace {

QString paddedNumber(int value, int width)
{
    return QStringLiteral("%1").arg(value, width, 10, QLatin1Char('0'));
}

QString sanitizeFilename(QString value)
{
    static const QString invalidChars = QStringLiteral("<>:\"/\\|?*");
    for (const QChar character : invalidChars) {
        value.replace(character, QLatin1Char('_'));
    }

    return value;
}

QString chapterExtension(MkvChapterType chapterType)
{
    switch (chapterType) {
    case MkvChapterType::Xml:
        return QStringLiteral("xml");
    case MkvChapterType::Ogm:
        return QStringLiteral("txt");
    case MkvChapterType::Cue:
        return QStringLiteral("cue");
    case MkvChapterType::Pbf:
        return QStringLiteral("pbf");
    }

    return QStringLiteral("xml");
}

QString outputPath(const QString& outputDirectory, const QString& filename)
{
    return outputDirectory.isEmpty()
        ? filename
        : QDir(outputDirectory).filePath(filename);
}

}

QString ExtractionNaming::replaceFilenamePlaceholders(const Segment& segment, const QString& mkvFile, const QString& filenamePattern)
{
    const QFileInfo mkvFileInfo(mkvFile);
    QString finalFilename = filenamePattern;

    finalFilename.replace(QString::fromLatin1(FilenamePatterns::FilenameNoExt), mkvFileInfo.completeBaseName());
    finalFilename.replace(QString::fromLatin1(FilenamePatterns::Filename), mkvFileInfo.fileName());

    if (const auto* track = dynamic_cast<const Track*>(&segment)) {
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackNumber_000), paddedNumber(track->trackNumber, 3));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackNumber_00), paddedNumber(track->trackNumber, 2));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackNumber_0), paddedNumber(track->trackNumber, 1));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackNumber), QString::number(track->trackNumber));

        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackID_000), paddedNumber(track->trackID, 3));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackID_00), paddedNumber(track->trackID, 2));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackID_0), paddedNumber(track->trackID, 1));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackID), QString::number(track->trackID));

        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackName), track->trackName);
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackLanguage), track->language);
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackLanguageIetf), track->languageIetf);
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackCodecID), track->codecID);
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackCodecPrivate), track->codecPrivate);
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackDelay), QString::number(track->delay));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackEffectiveDelay), QString::number(track->effectiveDelay));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::TrackForced), track->forced ? QStringLiteral("FORCED") : QString());

        if (track->trackType == MkvTrackType::Video) {
            finalFilename.replace(QString::fromLatin1(FilenamePatterns::VideoPixelWidth), QString::number(track->videoPixelWidth));
            finalFilename.replace(QString::fromLatin1(FilenamePatterns::VideoPixelHeight), QString::number(track->videoPixelHeight));
        } else if (track->trackType == MkvTrackType::Audio) {
            finalFilename.replace(QString::fromLatin1(FilenamePatterns::AudioSamplingFrequency), QString::number(track->audioSamplingFrequency));
            finalFilename.replace(QString::fromLatin1(FilenamePatterns::AudioChannels), QString::number(track->audioChannels));
        }
    } else if (const auto* attachment = dynamic_cast<const Attachment*>(&segment)) {
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::AttachmentID_000), paddedNumber(attachment->id, 3));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::AttachmentID_00), paddedNumber(attachment->id, 2));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::AttachmentID_0), paddedNumber(attachment->id, 1));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::AttachmentID), QString::number(attachment->id));
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::AttachmentFilename), attachment->filename);
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::AttachmentMimeType), attachment->mimeType);
        finalFilename.replace(QString::fromLatin1(FilenamePatterns::AttachmentFileSize), attachment->fileSize);
    }

    finalFilename = sanitizeFilename(finalFilename);
    finalFilename.replace(QString::fromLatin1(FilenamePatterns::DirectorySeparator), QString(QDir::separator()));
    return finalFilename.trimmed();
}

QString ExtractionNaming::outputFilename(
    const Segment& segment,
    const QString& outputDirectory,
    const QString& mkvFile,
    const FilenamePatterns& filenamePatterns,
    bool overwriteExistingFile,
    MkvExtractMode extractMode,
    MkvChapterType chapterType)
{
    const QFileInfo mkvFileInfo(mkvFile);
    QString outputFilename;

    switch (extractMode) {
    case MkvExtractMode::Tracks: {
        const auto* track = dynamic_cast<const Track*>(&segment);
        if (track == nullptr) {
            throw std::runtime_error("Called outputFilename without track.");
        }

        QString extension;
        QString replacedFilePattern;
        if (track->trackType == MkvTrackType::Video) {
            extension = videoFileExtensionFromCodecID(*track);
            replacedFilePattern = replaceFilenamePlaceholders(segment, mkvFile, filenamePatterns.videoTrackFilenamePattern);
        } else if (track->trackType == MkvTrackType::Audio) {
            extension = audioFileExtensionFromCodecID(*track);
            replacedFilePattern = replaceFilenamePlaceholders(segment, mkvFile, filenamePatterns.audioTrackFilenamePattern);
        } else {
            extension = subtitleFileExtensionFromCodecID(*track);
            replacedFilePattern = replaceFilenamePlaceholders(segment, mkvFile, filenamePatterns.subtitleTrackFilenamePattern);
        }

        outputFilename = outputPath(outputDirectory, QStringLiteral("%1.%2").arg(replacedFilePattern, extension));
        break;
    }
    case MkvExtractMode::Tags:
        outputFilename = outputPath(outputDirectory, QStringLiteral("%1.xml")
            .arg(replaceFilenamePlaceholders(segment, mkvFile, filenamePatterns.tagsFilenamePattern)));
        break;
    case MkvExtractMode::Attachments:
        if (dynamic_cast<const Attachment*>(&segment) == nullptr) {
            throw std::runtime_error("Called outputFilename without attachment.");
        }
        outputFilename = outputPath(
            outputDirectory,
            replaceFilenamePlaceholders(segment, mkvFile, filenamePatterns.attachmentFilenamePattern));
        break;
    case MkvExtractMode::Chapters:
        outputFilename = outputPath(outputDirectory, QStringLiteral("%1.%2")
            .arg(replaceFilenamePlaceholders(segment, mkvFile, filenamePatterns.chapterFilenamePattern), chapterExtension(chapterType)));
        break;
    case MkvExtractMode::CueSheet:
        outputFilename = outputPath(outputDirectory, QStringLiteral("%1_cuesheet.cue").arg(mkvFileInfo.completeBaseName()));
        break;
    case MkvExtractMode::TimecodesV2:
    case MkvExtractMode::TimestampsV2: {
        const auto* track = dynamic_cast<const Track*>(&segment);
        if (track == nullptr) {
            throw std::runtime_error("Called outputFilename without track/timestamps.");
        }
        outputFilename = outputPath(outputDirectory, QStringLiteral("%1_track%2_[%3].tc.txt")
            .arg(mkvFileInfo.completeBaseName())
            .arg(track->trackNumber)
            .arg(track->language));
        break;
    }
    case MkvExtractMode::Cues: {
        const auto* track = dynamic_cast<const Track*>(&segment);
        if (track == nullptr) {
            throw std::runtime_error("Called outputFilename without track/cues.");
        }
        outputFilename = outputPath(outputDirectory, QStringLiteral("%1_track%2_[%3].cue")
            .arg(mkvFileInfo.completeBaseName())
            .arg(track->trackNumber)
            .arg(track->language));
        break;
    }
    }

    return getOutputFilename(outputFilename, overwriteExistingFile);
}

QString ExtractionNaming::videoFileExtensionFromCodecID(const Track& track)
{
    const QString codec = track.codecID.toUpper();
    if (codec.contains(QStringLiteral("V_MS/VFW/FOURCC"))) return QStringLiteral("avi");
    if (codec.contains(QStringLiteral("V_UNCOMPRESSED"))) return QStringLiteral("raw");
    if (codec.contains(QStringLiteral("V_MPEG4/ISO/"))) return QStringLiteral("avc");
    if (codec.contains(QStringLiteral("V_MPEGH/ISO/HEVC"))) return QStringLiteral("hevc");
    if (codec.contains(QStringLiteral("V_AV1"))) return QStringLiteral("av1");
    if (codec.contains(QStringLiteral("V_MPEG4/MS/V3"))) return QStringLiteral("mp4");
    if (codec.contains(QStringLiteral("V_MPEG1"))) return QStringLiteral("mpg");
    if (codec.contains(QStringLiteral("V_MPEG2"))) return QStringLiteral("mpg");
    if (codec.contains(QStringLiteral("V_REAL/"))) return QStringLiteral("rm");
    if (codec.contains(QStringLiteral("V_QUICKTIME"))) return QStringLiteral("mov");
    if (codec.contains(QStringLiteral("V_THEORA"))) return QStringLiteral("ogv");
    if (codec.contains(QStringLiteral("V_PRORES"))) return QStringLiteral("mov");
    if (codec.contains(QStringLiteral("V_VP"))) return QStringLiteral("ivf");
    if (codec.contains(QStringLiteral("V_DIRAC"))) return QStringLiteral("drc");
    return QStringLiteral("mkv");
}

QString ExtractionNaming::audioFileExtensionFromCodecID(const Track& track)
{
    const QString codec = track.codecID.toUpper();
    if (codec.contains(QStringLiteral("A_MPEG/L3"))) return QStringLiteral("mp3");
    if (codec.contains(QStringLiteral("A_MPEG/L2"))) return QStringLiteral("mp2");
    if (codec.contains(QStringLiteral("A_MPEG/L1"))) return QStringLiteral("mpa");
    if (codec.contains(QStringLiteral("A_PCM"))) return QStringLiteral("wav");
    if (codec.contains(QStringLiteral("A_MPC"))) return QStringLiteral("mpc");
    if (codec.contains(QStringLiteral("A_AC3"))) return QStringLiteral("ac3");
    if (codec.contains(QStringLiteral("A_EAC3"))) return QStringLiteral("eac3");
    if (codec.contains(QStringLiteral("A_ALAC"))) return QStringLiteral("caf");
    if (codec.contains(QStringLiteral("A_DTS"))) return QStringLiteral("dts");
    if (codec.contains(QStringLiteral("A_VORBIS"))) return QStringLiteral("ogg");
    if (codec.contains(QStringLiteral("A_FLAC"))) return QStringLiteral("flac");
    if (codec.contains(QStringLiteral("A_REAL"))) return QStringLiteral("ra");
    if (codec.contains(QStringLiteral("A_MS/ACM"))) return QStringLiteral("wav");
    if (codec.contains(QStringLiteral("A_AAC"))) return QStringLiteral("aac");
    if (codec.contains(QStringLiteral("A_QUICKTIME"))) return QStringLiteral("mov");
    if (codec.contains(QStringLiteral("A_TRUEHD"))) return QStringLiteral("thd");
    if (codec.contains(QStringLiteral("A_TTA1"))) return QStringLiteral("tta");
    if (codec.contains(QStringLiteral("A_WAVPACK4"))) return QStringLiteral("wv");
    if (codec.contains(QStringLiteral("A_OPUS"))) return QStringLiteral("opus");
    if (codec.contains(QStringLiteral("A_MLP"))) return QStringLiteral("mlp");
    return QStringLiteral("mka");
}

QString ExtractionNaming::subtitleFileExtensionFromCodecID(const Track& track)
{
    const QString codec = track.codecID.toUpper();
    if (codec.contains(QStringLiteral("S_TEXT/UTF8"))) return QStringLiteral("srt");
    if (codec.contains(QStringLiteral("S_TEXT/ASCII"))) return QStringLiteral("srt");
    if (codec.contains(QStringLiteral("S_TEXT/SSA"))) return QStringLiteral("ass");
    if (codec.contains(QStringLiteral("S_TEXT/ASS"))) return QStringLiteral("ass");
    if (codec.contains(QStringLiteral("S_TEXT/USF"))) return QStringLiteral("usf");
    if (codec.contains(QStringLiteral("S_TEXT/WEBVTT"))) return QStringLiteral("webvtt");
    if (codec.contains(QStringLiteral("S_IMAGE/BMP"))) return QStringLiteral("sub");
    if (codec.contains(QStringLiteral("S_VOBSUB"))) return QStringLiteral("sub");
    if (codec.contains(QStringLiteral("S_DVBSUB"))) return QStringLiteral("dvbsub");
    if (codec.contains(QStringLiteral("S_HDMV/PGS"))) return QStringLiteral("sup");
    if (codec.contains(QStringLiteral("S_HDMV/TEXTST"))) return QStringLiteral("textst");
    if (codec.contains(QStringLiteral("S_KATE"))) return QStringLiteral("ogg");
    return QStringLiteral("sub");
}

}
