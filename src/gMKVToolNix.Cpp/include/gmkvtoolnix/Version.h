#pragma once

#include <QStringList>

namespace gmkv {

struct Version
{
    int fileMajorPart = 0;
    int fileMinorPart = 0;
    int filePrivatePart = 0;

    friend bool operator==(const Version& left, const Version& right)
    {
        return left.fileMajorPart == right.fileMajorPart
            && left.fileMinorPart == right.fileMinorPart
            && left.filePrivatePart == right.filePrivatePart;
    }
};

Version parseVersionOutput(const QStringList& mkvtoolnixOutputLines);

}
