#include "gmkvtoolnix/Settings.h"

#include "gmkvtoolnix/Log.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

namespace gmkv {

Settings::Settings(const QString& appPath, const QString& userAppDataPath)
    : m_settingsPath(resolveSettingsPath(appPath, userAppDataPath))
{
    Logger::log(QStringLiteral("Detected settings path: %1").arg(m_settingsPath));
}

QString Settings::settingsFileName()
{
    return QStringLiteral("gMKVExtractGUI.ini");
}

QString Settings::settingsPath() const
{
    return m_settingsPath;
}

QString Settings::settingsFilePath() const
{
    return QDir(m_settingsPath).filePath(settingsFileName());
}

void Settings::reload()
{
    QFile file(settingsFilePath());
    if (!file.exists()) {
        Logger::log(QStringLiteral("Settings file '%1' not found! Saving defaults...").arg(settingsFilePath()));
        save();
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Logger::log(QStringLiteral("Error opening settings file '%1': %2").arg(settingsFilePath(), file.errorString()));
        return;
    }

    Logger::log(QStringLiteral("Begin loading settings..."));
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    while (!stream.atEnd()) {
        const QString line = stream.readLine();

        if (line.startsWith(QStringLiteral("MKVToolnix Path:"))) {
            mkvToolNixPath = valueAfterColon(line);
        } else if (line.startsWith(QStringLiteral("Chapter Type:"))) {
            chapterType = chapterTypeFromString(valueAfterColon(line), MkvChapterType::Xml);
        } else if (line.startsWith(QStringLiteral("Output Directory:"))) {
            outputDirectory = valueAfterColon(line);
        } else if (line.startsWith(QStringLiteral("Default Output Directory:"))) {
            defaultOutputDirectory = valueAfterColon(line);
        } else if (line.startsWith(QStringLiteral("Lock Output Directory:"))) {
            lockedOutputDirectory = parseBool(valueAfterColon(line), false);
        } else if (line.startsWith(QStringLiteral("Initial Window Position X:"))) {
            windowPosX = parseInt(valueAfterColon(line), 0);
        } else if (line.startsWith(QStringLiteral("Initial Window Position Y:"))) {
            windowPosY = parseInt(valueAfterColon(line), 0);
        } else if (line.startsWith(QStringLiteral("Initial Window Size Width:"))) {
            windowSizeWidth = parseInt(valueAfterColon(line), 640);
        } else if (line.startsWith(QStringLiteral("Initial Window Size Height:"))) {
            windowSizeHeight = parseInt(valueAfterColon(line), 600);
        } else if (line.startsWith(QStringLiteral("Job Mode:"))) {
            jobMode = parseBool(valueAfterColon(line), false);
        } else if (line.startsWith(QStringLiteral("Window State:"))) {
            windowState = parseWindowState(valueAfterColon(line), WindowState::Normal);
        } else if (line.startsWith(QStringLiteral("Show Popup:"))) {
            showPopup = parseBool(valueAfterColon(line), true);
        } else if (line.startsWith(QStringLiteral("Show Popup In Job Manager:"))) {
            showPopupInJobManager = parseBool(valueAfterColon(line), true);
        } else if (line.startsWith(QStringLiteral("VideoTrackFilenamePattern:"))) {
            const QString value = valueAfterColon(line);
            if (!value.trimmed().isEmpty()) {
                filenamePatterns.videoTrackFilenamePattern = value;
            }
        } else if (line.startsWith(QStringLiteral("AudioTrackFilenamePattern:"))) {
            const QString value = valueAfterColon(line);
            if (!value.trimmed().isEmpty()) {
                filenamePatterns.audioTrackFilenamePattern = value;
            }
        } else if (line.startsWith(QStringLiteral("SubtitleTrackFilenamePattern:"))) {
            const QString value = valueAfterColon(line);
            if (!value.trimmed().isEmpty()) {
                filenamePatterns.subtitleTrackFilenamePattern = value;
            }
        } else if (line.startsWith(QStringLiteral("ChapterFilenamePattern:"))) {
            const QString value = valueAfterColon(line);
            if (!value.trimmed().isEmpty()) {
                filenamePatterns.chapterFilenamePattern = value;
            }
        } else if (line.startsWith(QStringLiteral("AttachmentFilenamePattern:"))) {
            const QString value = valueAfterColon(line);
            if (!value.trimmed().isEmpty()) {
                filenamePatterns.attachmentFilenamePattern = value;
            }
        } else if (line.startsWith(QStringLiteral("TagsFilenamePattern:"))) {
            const QString value = valueAfterColon(line);
            if (!value.trimmed().isEmpty()) {
                filenamePatterns.tagsFilenamePattern = value;
            }
        } else if (line.startsWith(QStringLiteral("Append On Drag and Drop:"))) {
            appendOnDragAndDrop = parseBool(valueAfterColon(line), false);
        } else if (line.startsWith(QStringLiteral("Overwrite Existing Files:"))) {
            overwriteExistingFiles = parseBool(valueAfterColon(line), false);
        } else if (line.startsWith(QStringLiteral("Disable Tooltips:"))) {
            disableTooltips = parseBool(valueAfterColon(line), false);
        } else if (line.startsWith(QStringLiteral("DarkMode:"))) {
            darkMode = parseBool(valueAfterColon(line), false);
        } else if (line.startsWith(QStringLiteral("DisableBomForTextFiles:"))) {
            disableBomForTextFiles = parseBool(valueAfterColon(line), false);
        } else if (line.startsWith(QStringLiteral("UseRawExtractionMode:"))) {
            useRawExtractionMode = parseBool(valueAfterColon(line), false);
        } else if (line.startsWith(QStringLiteral("UseFullRawExtractionMode:"))) {
            useFullRawExtractionMode = parseBool(valueAfterColon(line), false);
        } else if (line.startsWith(QStringLiteral("Culture:"))) {
            culture = valueAfterColon(line).trimmed();
            if (culture.isEmpty()) {
                culture = QStringLiteral("en");
            }
        }
    }

    Logger::log(QStringLiteral("Finished loading settings!"));
}

void Settings::save() const
{
    QDir().mkpath(m_settingsPath);

    QFile file(settingsFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        Logger::log(QStringLiteral("Error saving settings file '%1': %2").arg(settingsFilePath(), file.errorString()));
        return;
    }

    Logger::log(QStringLiteral("Saving settings..."));
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    stream << "MKVToolnix Path:" << mkvToolNixPath << '\n';
    stream << "Chapter Type:" << toSettingsString(chapterType) << '\n';
    stream << "Output Directory:" << outputDirectory << '\n';
    stream << "Default Output Directory:" << defaultOutputDirectory << '\n';
    stream << "Lock Output Directory:" << boolToString(lockedOutputDirectory) << '\n';
    stream << "Initial Window Position X:" << windowPosX << '\n';
    stream << "Initial Window Position Y:" << windowPosY << '\n';
    stream << "Initial Window Size Width:" << windowSizeWidth << '\n';
    stream << "Initial Window Size Height:" << windowSizeHeight << '\n';
    stream << "Job Mode:" << boolToString(jobMode) << '\n';
    stream << "Window State:" << windowStateToString(windowState) << '\n';
    stream << "Show Popup:" << boolToString(showPopup) << '\n';
    stream << "Show Popup In Job Manager:" << boolToString(showPopupInJobManager) << '\n';
    stream << "Append On Drag and Drop:" << boolToString(appendOnDragAndDrop) << '\n';
    stream << "Overwrite Existing Files:" << boolToString(overwriteExistingFiles) << '\n';
    stream << "Disable Tooltips:" << boolToString(disableTooltips) << '\n';
    stream << "DarkMode:" << boolToString(darkMode) << '\n';
    stream << "DisableBomForTextFiles:" << boolToString(disableBomForTextFiles) << '\n';
    stream << "UseRawExtractionMode:" << boolToString(useRawExtractionMode) << '\n';
    stream << "UseFullRawExtractionMode:" << boolToString(useFullRawExtractionMode) << '\n';
    stream << "Culture:" << culture << '\n';
    stream << "VideoTrackFilenamePattern:" << filenamePatterns.videoTrackFilenamePattern << '\n';
    stream << "AudioTrackFilenamePattern:" << filenamePatterns.audioTrackFilenamePattern << '\n';
    stream << "SubtitleTrackFilenamePattern:" << filenamePatterns.subtitleTrackFilenamePattern << '\n';
    stream << "ChapterFilenamePattern:" << filenamePatterns.chapterFilenamePattern << '\n';
    stream << "AttachmentFilenamePattern:" << filenamePatterns.attachmentFilenamePattern << '\n';
    stream << "TagsFilenamePattern:" << filenamePatterns.tagsFilenamePattern << '\n';
}

QString Settings::valueAfterColon(const QString& line) const
{
    const int colonIndex = line.indexOf(QLatin1Char(':'));
    return colonIndex < 0 ? QString() : line.mid(colonIndex + 1);
}

bool Settings::parseBool(const QString& value, bool fallback) const
{
    const QString normalized = value.trimmed();
    if (normalized.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
        return true;
    }
    if (normalized.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) {
        return false;
    }

    return fallback;
}

int Settings::parseInt(const QString& value, int fallback) const
{
    bool ok = false;
    const int parsed = value.trimmed().toInt(&ok);
    return ok ? parsed : fallback;
}

WindowState Settings::parseWindowState(const QString& value, WindowState fallback) const
{
    const QString normalized = value.trimmed();
    if (normalized.compare(QStringLiteral("Normal"), Qt::CaseInsensitive) == 0) {
        return WindowState::Normal;
    }
    if (normalized.compare(QStringLiteral("Minimized"), Qt::CaseInsensitive) == 0) {
        return WindowState::Minimized;
    }
    if (normalized.compare(QStringLiteral("Maximized"), Qt::CaseInsensitive) == 0) {
        return WindowState::Maximized;
    }

    return fallback;
}

QString Settings::windowStateToString(WindowState state) const
{
    switch (state) {
    case WindowState::Normal:
        return QStringLiteral("Normal");
    case WindowState::Minimized:
        return QStringLiteral("Minimized");
    case WindowState::Maximized:
        return QStringLiteral("Maximized");
    }

    return QStringLiteral("Normal");
}

QString Settings::boolToString(bool value) const
{
    return value ? QStringLiteral("True") : QStringLiteral("False");
}

QString Settings::defaultUserAppDataPath() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return path.isEmpty() ? QDir::homePath() : path;
}

QString Settings::resolveSettingsPath(const QString& appPath, const QString& userAppDataPath) const
{
    const QString candidatePath = appPath.isEmpty() ? QDir::currentPath() : appPath;
    QDir().mkpath(candidatePath);

    QFile probe(QDir(candidatePath).filePath(settingsFileName()));
    if (probe.open(QIODevice::ReadWrite | QIODevice::Append)) {
        probe.close();
        return candidatePath;
    }

    const QString fallbackPath = userAppDataPath.isEmpty() ? defaultUserAppDataPath() : userAppDataPath;
    QDir().mkpath(fallbackPath);
    return fallbackPath;
}

}
