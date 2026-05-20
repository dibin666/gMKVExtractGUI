#include "gmkvtoolnix/FileName.h"

#include <QDir>
#include <QFileInfo>

namespace gmkv {

QString getOutputFilename(const QString& filename, bool overwriteExisting)
{
    QString candidate = filename;

    while (!overwriteExisting && QFileInfo::exists(candidate)) {
        const QFileInfo fileInfo(candidate);
        const QString directory = fileInfo.dir().path() == QStringLiteral(".")
            ? QString()
            : fileInfo.dir().path();
        QString baseName = fileInfo.completeBaseName();
        const QString suffix = fileInfo.suffix();
        const int lastDotIndex = baseName.lastIndexOf(QLatin1Char('.'));

        int outputFilenameCounter = 0;
        if (lastDotIndex > -1) {
            bool parsedCounter = false;
            const int parsedValue = baseName.mid(lastDotIndex + 1).toInt(&parsedCounter);
            if (parsedCounter) {
                outputFilenameCounter = parsedValue;
                baseName = baseName.left(lastDotIndex);
            }
        }

        const QString extension = suffix.isEmpty() ? QString() : QStringLiteral(".") + suffix;
        const QString nextFilename = QStringLiteral("%1.%2%3")
            .arg(baseName)
            .arg(outputFilenameCounter + 1)
            .arg(extension);

        candidate = directory.isEmpty()
            ? nextFilename
            : QDir(directory).filePath(nextFilename);
    }

    return candidate;
}

}
