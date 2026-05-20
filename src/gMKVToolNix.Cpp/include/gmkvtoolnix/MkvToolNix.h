#pragma once

#include "gmkvtoolnix/Version.h"

#include <QMap>
#include <QString>
#include <QStringList>

namespace gmkv {

enum class MkvTool
{
    Merge,
    Info,
    Extract,
    MergeGui,
    MergeNewGui,
};

struct MkvToolPaths
{
    QString mkvmergePath;
    QString mkvinfoPath;
    QString mkvextractPath;

    bool isValid() const;
    QString path(MkvTool tool) const;
    QString displayLocation() const;
};

struct ProcessResult
{
    int exitCode = -1;
    QStringList standardOutputLines;
    QStringList standardErrorLines;
    QString errorString;

    bool hasProcessError() const;
};

class Platform
{
public:
    static bool isLinux();
};

class MkvToolNix
{
public:
    static QString executableName(MkvTool tool);
    static QString executablePath(const QString& directory, MkvTool tool);
    static bool hasExecutable(const QString& directory, MkvTool tool);
    static bool isToolDirectory(const QString& directory);
    static MkvToolPaths toolPathsFromDirectory(const QString& directory);
    static MkvToolPaths toolPathsFromPath(const QStringList& searchDirectories = {});
    static MkvToolPaths toolPathsFromLocation(const QString& location, const QStringList& searchDirectories = {});
    static QString uiLanguageCode();
    static QString escapeString(QString value);
    static QString unescapeString(QString value);
};

struct ToolLocatorInputs
{
    QString explicitPath;
    QString savedPath;
    QString applicationPath;
    QString linuxDefaultPath = QStringLiteral("/usr/bin");
    QStringList searchDirectories;
    bool searchPath = true;
};

class ToolLocator
{
public:
    static QString locate(const ToolLocatorInputs& inputs);
    static MkvToolPaths locateTools(const ToolLocatorInputs& inputs);
};

class ProcessRunner
{
public:
    static ProcessResult run(const QString& program, const QStringList& arguments, const QMap<QString, QString>& environment = {});
};

class MkvToolVersionService
{
public:
    static Version readVersion(const QString& toolDirectory, MkvTool tool);
    static Version readVersion(const MkvToolPaths& toolPaths, MkvTool tool);
};

}
