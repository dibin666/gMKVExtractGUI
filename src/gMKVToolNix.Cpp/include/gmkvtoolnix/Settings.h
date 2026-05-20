#pragma once

#include "gmkvtoolnix/FilenamePatterns.h"
#include "gmkvtoolnix/Segments.h"

#include <QString>

namespace gmkv {

enum class WindowState
{
    Normal,
    Minimized,
    Maximized,
};

class Settings
{
public:
    explicit Settings(const QString& appPath, const QString& userAppDataPath = QString());

    static QString settingsFileName();
    QString settingsPath() const;
    QString settingsFilePath() const;

    void reload();
    void save() const;

    QString mkvToolNixPath;
    MkvChapterType chapterType = MkvChapterType::Xml;
    bool lockedOutputDirectory = false;
    QString outputDirectory;
    QString defaultOutputDirectory;
    int windowPosX = 0;
    int windowPosY = 0;
    int windowSizeWidth = 640;
    int windowSizeHeight = 600;
    bool jobMode = false;
    WindowState windowState = WindowState::Normal;
    bool showPopup = true;
    bool showPopupInJobManager = true;
    bool appendOnDragAndDrop = false;
    bool overwriteExistingFiles = false;
    bool disableTooltips = false;
    bool darkMode = false;
    bool disableBomForTextFiles = false;
    bool useRawExtractionMode = false;
    bool useFullRawExtractionMode = false;
    QString culture = QStringLiteral("en");
    FilenamePatterns filenamePatterns;

private:
    QString valueAfterColon(const QString& line) const;
    bool parseBool(const QString& value, bool fallback) const;
    int parseInt(const QString& value, int fallback) const;
    WindowState parseWindowState(const QString& value, WindowState fallback) const;
    QString windowStateToString(WindowState state) const;
    QString boolToString(bool value) const;
    QString defaultUserAppDataPath() const;
    QString resolveSettingsPath(const QString& appPath, const QString& userAppDataPath) const;

    QString m_settingsPath;
};

}
