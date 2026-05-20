#pragma once

#include <QString>

namespace gmkv {

struct FilenamePatterns
{
    static constexpr const char* FilenameNoExt = "{FilenameNoExt}";
    static constexpr const char* Filename = "{Filename}";
    static constexpr const char* DirectorySeparator = "{DirSeparator}";
    static constexpr const char* TrackNumber = "{TrackNumber}";
    static constexpr const char* TrackNumber_0 = "{TrackNumber:0}";
    static constexpr const char* TrackNumber_00 = "{TrackNumber:00}";
    static constexpr const char* TrackNumber_000 = "{TrackNumber:000}";
    static constexpr const char* TrackID = "{TrackID}";
    static constexpr const char* TrackID_0 = "{TrackID:0}";
    static constexpr const char* TrackID_00 = "{TrackID:00}";
    static constexpr const char* TrackID_000 = "{TrackID:000}";
    static constexpr const char* TrackName = "{TrackName}";
    static constexpr const char* TrackLanguage = "{Language}";
    static constexpr const char* TrackLanguageIetf = "{LanguageIETF}";
    static constexpr const char* TrackCodecID = "{CodecID}";
    static constexpr const char* TrackCodecPrivate = "{CodecPrivate}";
    static constexpr const char* TrackDelay = "{Delay}";
    static constexpr const char* TrackEffectiveDelay = "{EffectiveDelay}";
    static constexpr const char* TrackForced = "{TrackForced}";
    static constexpr const char* VideoPixelWidth = "{PixelWidth}";
    static constexpr const char* VideoPixelHeight = "{PixelHeight}";
    static constexpr const char* AudioSamplingFrequency = "{SamplingFrequency}";
    static constexpr const char* AudioChannels = "{Channels}";
    static constexpr const char* AttachmentID = "{AttachmentID}";
    static constexpr const char* AttachmentID_0 = "{AttachmentID:0}";
    static constexpr const char* AttachmentID_00 = "{AttachmentID:00}";
    static constexpr const char* AttachmentID_000 = "{AttachmentID:000}";
    static constexpr const char* AttachmentFilename = "{AttachmentFilename}";
    static constexpr const char* AttachmentMimeType = "{MimeType}";
    static constexpr const char* AttachmentFileSize = "{AttachmentFileSize}";

    QString videoTrackFilenamePattern = defaultVideoTrackFilenamePattern();
    QString audioTrackFilenamePattern = defaultAudioTrackFilenamePattern();
    QString subtitleTrackFilenamePattern = defaultSubtitleTrackFilenamePattern();
    QString chapterFilenamePattern = defaultChapterFilenamePattern();
    QString attachmentFilenamePattern = defaultAttachmentFilenamePattern();
    QString tagsFilenamePattern = defaultTagsFilenamePattern();

    static QString defaultVideoTrackFilenamePattern();
    static QString defaultAudioTrackFilenamePattern();
    static QString defaultSubtitleTrackFilenamePattern();
    static QString defaultChapterFilenamePattern();
    static QString defaultAttachmentFilenamePattern();
    static QString defaultTagsFilenamePattern();
};

}
