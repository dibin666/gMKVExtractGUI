#include "gmkvtoolnix/MkvToolNix.h"

#include "gmkvtoolnix/Log.h"
#include "gmkvtoolnix/OptionValue.h"

#include <QDir>
#include <QFileInfo>
#include <QOperatingSystemVersion>
#include <QProcess>
#include <QProcessEnvironment>
#ifdef Q_OS_WIN
#include <QSettings>
#endif
#include <QStandardPaths>

#include <stdexcept>

namespace gmkv {

namespace {

QStringList splitProcessLines(const QByteArray& data)
{
    QString text = QString::fromUtf8(data);
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    QStringList lines = text.split(QLatin1Char('\n'));
    if (!lines.isEmpty() && lines.last().isEmpty()) {
        lines.removeLast();
    }

    return lines;
}

QString firstValidToolDirectory(const QStringList& candidates)
{
    for (const QString& candidate : candidates) {
        if (!candidate.trimmed().isEmpty() && MkvToolNix::isToolDirectory(candidate)) {
            return QDir(candidate).absolutePath();
        }
    }

    return {};
}

QString trimWindowsExecutableValue(QString value)
{
    value = value.trimmed();
    if (value.isEmpty()) {
        return {};
    }

    if (value.startsWith(QLatin1Char('"'))) {
        const qsizetype closingQuote = value.indexOf(QLatin1Char('"'), 1);
        if (closingQuote > 1) {
            return value.mid(1, closingQuote - 1);
        }

        return value.mid(1);
    }

    const QString lowerValue = value.toLower();
    const qsizetype executableIndex = lowerValue.indexOf(QStringLiteral(".exe"));
    if (executableIndex >= 0) {
        return value.left(executableIndex + 4);
    }

    const qsizetype commaIndex = value.lastIndexOf(QLatin1Char(','));
    if (commaIndex > 0) {
        const QString suffix = value.mid(commaIndex + 1).trimmed();
        bool numericSuffix = !suffix.isEmpty();
        for (const QChar ch : suffix) {
            if (!ch.isDigit()) {
                numericSuffix = false;
                break;
            }
        }

        if (numericSuffix) {
            return value.left(commaIndex).trimmed();
        }
    }

    return value;
}

QString registryValueToDirectory(const QString& value)
{
    const QString path = trimWindowsExecutableValue(value);
    if (path.isEmpty()) {
        return {};
    }

    if (path.toLower().endsWith(QStringLiteral(".exe"))) {
        return QFileInfo(path).absolutePath();
    }

    const QFileInfo info(path);
    if (info.exists() && info.isFile()) {
        return info.absolutePath();
    }

    return QDir(path).absolutePath();
}

void appendRegistryCandidate(QStringList& candidates, const QString& value)
{
    const QString directory = registryValueToDirectory(value);
    if (!directory.isEmpty() && !candidates.contains(directory, Qt::CaseInsensitive)) {
        candidates.append(directory);
    }
}

QStringList windowsRegistryToolDirectories()
{
    QStringList candidates;

#ifdef Q_OS_WIN
    const QStringList uninstallKeys = {
        QStringLiteral(R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\MKVToolNix)"),
        QStringLiteral(R"(HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\MKVToolNix)"),
    };

    for (const QString& key : uninstallKeys) {
        QSettings settings(key, QSettings::NativeFormat);
        appendRegistryCandidate(candidates, settings.value(QStringLiteral("DisplayIcon")).toString());
        appendRegistryCandidate(candidates, settings.value(QStringLiteral("InstallLocation")).toString());
    }

    const QStringList currentUserGuiKeys = {
        QStringLiteral(R"(HKEY_CURRENT_USER\Software\mkvmergeGUI\GUI)"),
        QStringLiteral(R"(HKEY_CURRENT_USER\Software\MKVToolNix\GUI)"),
    };

    for (const QString& key : currentUserGuiKeys) {
        QSettings settings(key, QSettings::NativeFormat);
        appendRegistryCandidate(candidates, settings.value(QStringLiteral("mkvmerge_executable")).toString());
    }
#endif

    return candidates;
}

}

bool ProcessResult::hasProcessError() const
{
    return !errorString.isEmpty();
}

bool Platform::isLinux()
{
    return QOperatingSystemVersion::currentType() != QOperatingSystemVersion::Windows;
}

QString MkvToolNix::executableName(MkvTool tool)
{
    const bool linux = Platform::isLinux();

    switch (tool) {
    case MkvTool::Merge:
    case MkvTool::MergeNewGui:
        return linux ? QStringLiteral("mkvmerge") : QStringLiteral("mkvmerge.exe");
    case MkvTool::Info:
        return linux ? QStringLiteral("mkvinfo") : QStringLiteral("mkvinfo.exe");
    case MkvTool::Extract:
        return linux ? QStringLiteral("mkvextract") : QStringLiteral("mkvextract.exe");
    case MkvTool::MergeGui:
        return linux ? QStringLiteral("mmg") : QStringLiteral("mmg.exe");
    }

    return {};
}

QString MkvToolNix::executablePath(const QString& directory, MkvTool tool)
{
    return QDir(directory).filePath(executableName(tool));
}

bool MkvToolNix::hasExecutable(const QString& directory, MkvTool tool)
{
    const QFileInfo executable(executablePath(directory, tool));
    return executable.exists() && executable.isFile();
}

bool MkvToolNix::isToolDirectory(const QString& directory)
{
    return QDir(directory).exists()
        && hasExecutable(directory, MkvTool::Merge)
        && hasExecutable(directory, MkvTool::Info)
        && hasExecutable(directory, MkvTool::Extract);
}

QString MkvToolNix::uiLanguageCode()
{
    return Platform::isLinux() ? QStringLiteral("en_US") : QStringLiteral("en");
}

QString MkvToolNix::escapeString(QString value)
{
    return value
        .replace(QStringLiteral(" "), QStringLiteral(R"(\s)"))
        .replace(QStringLiteral("\""), QStringLiteral(R"(\2)"))
        .replace(QStringLiteral(":"), QStringLiteral(R"(\c)"))
        .replace(QStringLiteral("#"), QStringLiteral(R"(\h)"))
        .replace(QStringLiteral("\\"), QStringLiteral(R"(\\)"))
        .replace(QStringLiteral("["), QStringLiteral(R"(\b)"))
        .replace(QStringLiteral("]"), QStringLiteral(R"(\B)"));
}

QString MkvToolNix::unescapeString(QString value)
{
    return value
        .replace(QStringLiteral(R"(\s)"), QStringLiteral(" "))
        .replace(QStringLiteral(R"(\2)"), QStringLiteral("\""))
        .replace(QStringLiteral(R"(\c)"), QStringLiteral(":"))
        .replace(QStringLiteral(R"(\h)"), QStringLiteral("#"))
        .replace(QStringLiteral(R"(\\)"), QStringLiteral("\\"))
        .replace(QStringLiteral(R"(\b)"), QStringLiteral("["))
        .replace(QStringLiteral(R"(\B)"), QStringLiteral("]"));
}

QString ToolLocator::locate(const ToolLocatorInputs& inputs)
{
    const QString directMatch = firstValidToolDirectory({
        inputs.explicitPath,
        inputs.savedPath,
        inputs.applicationPath,
    });
    if (!directMatch.isEmpty()) {
        Logger::log(QStringLiteral("Found MKVToolNix in: %1").arg(directMatch));
        return directMatch;
    }

    const QString registryMatch = firstValidToolDirectory(windowsRegistryToolDirectories());
    if (!registryMatch.isEmpty()) {
        Logger::log(QStringLiteral("Found MKVToolNix in Windows registry: %1").arg(registryMatch));
        return registryMatch;
    }

    if (Platform::isLinux()) {
        const QString linuxDefaultMatch = firstValidToolDirectory({ inputs.linuxDefaultPath });
        if (!linuxDefaultMatch.isEmpty()) {
            Logger::log(QStringLiteral("Found MKVToolNix in: %1").arg(linuxDefaultMatch));
            return linuxDefaultMatch;
        }
    }

    if (inputs.searchPath) {
        const QString mkvmergePath = QStandardPaths::findExecutable(MkvToolNix::executableName(MkvTool::Merge));
        if (!mkvmergePath.isEmpty()) {
            const QString pathDirectory = QFileInfo(mkvmergePath).absolutePath();
            if (MkvToolNix::isToolDirectory(pathDirectory)) {
                Logger::log(QStringLiteral("Found MKVToolNix in PATH: %1").arg(pathDirectory));
                return pathDirectory;
            }
        }
    }

    return {};
}

ProcessResult ProcessRunner::run(const QString& program, const QStringList& arguments, const QMap<QString, QString>& environment)
{
    QProcess process;
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    for (auto it = environment.begin(); it != environment.end(); ++it) {
        processEnvironment.insert(it.key(), it.value());
    }
    process.setProcessEnvironment(processEnvironment);
    process.start(program, arguments);

    ProcessResult result;
    if (!process.waitForStarted()) {
        result.errorString = process.errorString();
        return result;
    }

    process.waitForFinished(-1);
    result.exitCode = process.exitCode();
    result.standardOutputLines = splitProcessLines(process.readAllStandardOutput());
    result.standardErrorLines = splitProcessLines(process.readAllStandardError());

    if (process.error() != QProcess::UnknownError) {
        result.errorString = process.errorString();
    }

    return result;
}

Version MkvToolVersionService::readVersion(const QString& toolDirectory, MkvTool tool)
{
    const QString program = MkvToolNix::executablePath(toolDirectory, tool);
    if (!QFileInfo::exists(program)) {
        throw std::runtime_error(QStringLiteral("Could not find %1").arg(program).toStdString());
    }

    const ProcessResult result = ProcessRunner::run(
        program,
        {
            QStringLiteral("--version"),
            QStringLiteral("--ui-language"),
            MkvToolNix::uiLanguageCode(),
        });

    Logger::log(QStringLiteral("\"%1\" --version --ui-language %2").arg(program, MkvToolNix::uiLanguageCode()));
    Logger::log(QStringLiteral("Exit code: %1").arg(result.exitCode));

    if (result.hasProcessError()) {
        throw std::runtime_error(result.errorString.toStdString());
    }

    if (result.exitCode > 1) {
        throw std::runtime_error(QStringLiteral("MKVToolNix exited with error code %1").arg(result.exitCode).toStdString());
    }

    QStringList versionLines = result.standardOutputLines;
    versionLines.append(result.standardErrorLines);
    return parseVersionOutput(versionLines);
}

}
