#pragma once

#include <QList>
#include <QString>

namespace gmkv {

enum class MkvExtractGlobalOption
{
    ParseFully,
    Verbose,
    Quiet,
    UiLanguage,
    CommandLineCharset,
    OutputCharset,
    RedirectOutput,
    Help,
    Version,
    CheckForUpdates,
    GuiMode,
};

enum class MkvMergeOption
{
    Identify,
    IdentifyVerbose,
    UiLanguage,
    CommandLineCharset,
    OutputCharset,
    IdentificationFormat,
    Version,
};

enum class MkvInfoOption
{
    Gui,
    Checksum,
    CheckMode,
    Summary,
    TrackInfo,
    Hexdump,
    FullHexdump,
    Size,
    Verbose,
    Quiet,
    UiLanguage,
    CommandLineCharset,
    OutputCharset,
    RedirectOutput,
    Help,
    Version,
    CheckForUpdates,
    GuiMode,
    NoGui,
};

template <typename T>
struct OptionValue
{
    T option;
    QString parameter;
};

QString optionName(MkvExtractGlobalOption option);
QString optionName(MkvMergeOption option);
QString optionName(MkvInfoOption option);

template <typename T>
QString convertOptionValueListToString(const QList<OptionValue<T>>& optionValues)
{
    QString optionString;

    for (const OptionValue<T>& optionValue : optionValues) {
        optionString += QLatin1Char(' ');
        optionString += optionName(optionValue.option);

        if (!optionValue.parameter.trimmed().isEmpty()) {
            optionString += QLatin1Char(' ');
            optionString += optionValue.parameter;
        }
    }

    return optionString;
}

}
