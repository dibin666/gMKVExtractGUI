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
    bool searchPath = true;
};

class ToolLocator
{
public:
    static QString locate(const ToolLocatorInputs& inputs);
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
};

}
