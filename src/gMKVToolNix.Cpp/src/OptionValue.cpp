#include "gmkvtoolnix/OptionValue.h"

namespace gmkv {

namespace {

QString mkvOption(QString option)
{
    option.replace(QLatin1Char('_'), QLatin1Char('-'));
    return QStringLiteral("--") + option;
}

}

QString optionName(MkvExtractGlobalOption option)
{
    switch (option) {
    case MkvExtractGlobalOption::ParseFully:
        return mkvOption(QStringLiteral("parse_fully"));
    case MkvExtractGlobalOption::Verbose:
        return mkvOption(QStringLiteral("verbose"));
    case MkvExtractGlobalOption::Quiet:
        return mkvOption(QStringLiteral("quiet"));
    case MkvExtractGlobalOption::UiLanguage:
        return mkvOption(QStringLiteral("ui_language"));
    case MkvExtractGlobalOption::CommandLineCharset:
        return mkvOption(QStringLiteral("command_line_charset"));
    case MkvExtractGlobalOption::OutputCharset:
        return mkvOption(QStringLiteral("output_charset"));
    case MkvExtractGlobalOption::RedirectOutput:
        return mkvOption(QStringLiteral("redirect_output"));
    case MkvExtractGlobalOption::Help:
        return mkvOption(QStringLiteral("help"));
    case MkvExtractGlobalOption::Version:
        return mkvOption(QStringLiteral("version"));
    case MkvExtractGlobalOption::CheckForUpdates:
        return mkvOption(QStringLiteral("check_for_updates"));
    case MkvExtractGlobalOption::GuiMode:
        return mkvOption(QStringLiteral("gui_mode"));
    }

    return {};
}

QString optionName(MkvMergeOption option)
{
    switch (option) {
    case MkvMergeOption::Identify:
        return mkvOption(QStringLiteral("identify"));
    case MkvMergeOption::IdentifyVerbose:
        return mkvOption(QStringLiteral("identify_verbose"));
    case MkvMergeOption::UiLanguage:
        return mkvOption(QStringLiteral("ui_language"));
    case MkvMergeOption::CommandLineCharset:
        return mkvOption(QStringLiteral("command_line_charset"));
    case MkvMergeOption::OutputCharset:
        return mkvOption(QStringLiteral("output_charset"));
    case MkvMergeOption::IdentificationFormat:
        return mkvOption(QStringLiteral("identification_format"));
    case MkvMergeOption::Version:
        return mkvOption(QStringLiteral("version"));
    }

    return {};
}

QString optionName(MkvInfoOption option)
{
    switch (option) {
    case MkvInfoOption::Gui:
        return mkvOption(QStringLiteral("gui"));
    case MkvInfoOption::Checksum:
        return mkvOption(QStringLiteral("checksum"));
    case MkvInfoOption::CheckMode:
        return mkvOption(QStringLiteral("check_mode"));
    case MkvInfoOption::Summary:
        return mkvOption(QStringLiteral("summary"));
    case MkvInfoOption::TrackInfo:
        return mkvOption(QStringLiteral("track_info"));
    case MkvInfoOption::Hexdump:
        return mkvOption(QStringLiteral("hexdump"));
    case MkvInfoOption::FullHexdump:
        return mkvOption(QStringLiteral("full_hexdump"));
    case MkvInfoOption::Size:
        return mkvOption(QStringLiteral("size"));
    case MkvInfoOption::Verbose:
        return mkvOption(QStringLiteral("verbose"));
    case MkvInfoOption::Quiet:
        return mkvOption(QStringLiteral("quiet"));
    case MkvInfoOption::UiLanguage:
        return mkvOption(QStringLiteral("ui_language"));
    case MkvInfoOption::CommandLineCharset:
        return mkvOption(QStringLiteral("command_line_charset"));
    case MkvInfoOption::OutputCharset:
        return mkvOption(QStringLiteral("output_charset"));
    case MkvInfoOption::RedirectOutput:
        return mkvOption(QStringLiteral("redirect_output"));
    case MkvInfoOption::Help:
        return mkvOption(QStringLiteral("help"));
    case MkvInfoOption::Version:
        return mkvOption(QStringLiteral("version"));
    case MkvInfoOption::CheckForUpdates:
        return mkvOption(QStringLiteral("check_for_updates"));
    case MkvInfoOption::GuiMode:
        return mkvOption(QStringLiteral("gui_mode"));
    case MkvInfoOption::NoGui:
        return mkvOption(QStringLiteral("no_gui"));
    }

    return {};
}

}
