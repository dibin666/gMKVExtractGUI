#pragma once

#include "gmkvtoolnix/Segments.h"

#include <QList>
#include <QString>

namespace gmkv {

class MkvMergeParser
{
public:
    static QList<SegmentPtr> parseJsonOutput(const QString& output, const QString& inputFile = QString());
    static bool findAndSetDelays(const QList<SegmentPtr>& segments);
};

}
