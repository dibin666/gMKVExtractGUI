#include "gmkvtoolnix/Version.h"

#include <QRegularExpression>

namespace gmkv {

Version parseVersionOutput(const QStringList& mkvtoolnixOutputLines)
{
    static const QRegularExpression versionRegex(
        QStringLiteral(R"(\bv(?<major>\d+)\.(?<minor>\d+)(?:\.(?<patch>\d+))?\b)"));

    for (const QString& outputLine : mkvtoolnixOutputLines) {
        if (outputLine.trimmed().isEmpty()) {
            continue;
        }

        const QRegularExpressionMatch match = versionRegex.match(outputLine);
        if (!match.hasMatch()) {
            continue;
        }

        Version version;
        version.fileMajorPart = match.captured(QStringLiteral("major")).toInt();
        version.fileMinorPart = match.captured(QStringLiteral("minor")).toInt();

        const QString patch = match.captured(QStringLiteral("patch"));
        version.filePrivatePart = patch.isEmpty() ? 0 : patch.toInt();
        return version;
    }

    return {};
}

}
