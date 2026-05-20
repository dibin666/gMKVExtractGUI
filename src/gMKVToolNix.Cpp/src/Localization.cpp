#include "gmkvtoolnix/Localization.h"

#include "gmkvtoolnix/Log.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace gmkv {

namespace {

constexpr const char* FallbackCulture = "en";
constexpr const char* FileExtension = ".json";

QString fallbackCulture()
{
    return QStringLiteral("en");
}

QString fileExtension()
{
    return QStringLiteral(".json");
}

QStringList legacyCultureAliases(const QString& culture)
{
    const QString canonical = TranslationPathService::canonicalCultureCode(culture);
    if (canonical.compare(QStringLiteral("zh-tw"), Qt::CaseInsensitive) == 0) {
        return { QStringLiteral("cn") };
    }

    return {};
}

void addCultureCandidate(QStringList& candidates, const QString& culture)
{
    if (!culture.trimmed().isEmpty() && !candidates.contains(culture, Qt::CaseInsensitive)) {
        candidates.append(culture);
    }
}

QString legacyTranslationFilePath(const QString& directory, const QString& culture)
{
    return QDir(directory).filePath(culture.trimmed() + fileExtension());
}

QJsonObject metadataToJson(const Metadata& metadata)
{
    QJsonObject object;
    object[QStringLiteral("Culture")] = metadata.culture;
    object[QStringLiteral("Translator")] = metadata.translator.isNull()
        ? QJsonValue(QJsonValue::Null)
        : QJsonValue(metadata.translator);
    object[QStringLiteral("CreationDate")] = metadata.creationDate.toUTC().toString(Qt::ISODate);
    object[QStringLiteral("LastEditDate")] = metadata.lastEditDate.toUTC().toString(Qt::ISODate);
    return object;
}

Metadata metadataFromJson(const QJsonObject& object)
{
    Metadata metadata;
    metadata.culture = object.value(QStringLiteral("Culture")).toString();
    metadata.translator = object.value(QStringLiteral("Translator")).toString();
    metadata.creationDate = QDateTime::fromString(object.value(QStringLiteral("CreationDate")).toString(), Qt::ISODate);
    metadata.lastEditDate = QDateTime::fromString(object.value(QStringLiteral("LastEditDate")).toString(), Qt::ISODate);
    return metadata;
}

QJsonObject entryToJson(const TranslationEntry& entry)
{
    QJsonObject object;
    object[QStringLiteral("Source")] = entry.source;
    object[QStringLiteral("Translation")] = entry.translation;
    object[QStringLiteral("IsTranslated")] = entry.isTranslated;
    object[QStringLiteral("Notes")] = entry.notes.isNull()
        ? QJsonValue(QJsonValue::Null)
        : QJsonValue(entry.notes);
    return object;
}

TranslationEntry entryFromJson(const QJsonObject& object)
{
    TranslationEntry entry;
    entry.source = object.value(QStringLiteral("Source")).toString();
    entry.translation = object.value(QStringLiteral("Translation")).toString();
    entry.isTranslated = object.value(QStringLiteral("IsTranslated")).toBool();
    entry.notes = object.value(QStringLiteral("Notes")).toString();
    return entry;
}

class LocalizationState
{
public:
    QString translationDirectory;
    QString currentCulture = fallbackCulture();
    bool initialized = false;
    std::unique_ptr<JsonLocalizationService> service;
};

LocalizationState& localizationState()
{
    static LocalizationState state;
    return state;
}

}

QString TranslationPathService::filePrefix()
{
    return QStringLiteral("gmkvextract-");
}

QString TranslationPathService::translationFileName(const QString& culture)
{
    if (culture.trimmed().isEmpty()) {
        throw std::invalid_argument("A culture code is required.");
    }

    return filePrefix() + culture.trimmed() + fileExtension();
}

QString TranslationPathService::translationFilePath(const QString& directory, const QString& culture)
{
    if (directory.trimmed().isEmpty()) {
        throw std::invalid_argument("A translation directory is required.");
    }

    return QDir(directory).filePath(translationFileName(culture));
}

QString TranslationPathService::existingTranslationFilePath(const QString& directory, const QString& culture)
{
    for (const QString& candidateCulture : cultureLookupChain(culture)) {
        const QString preferredPath = translationFilePath(directory, candidateCulture);
        if (QFileInfo::exists(preferredPath)) {
            return preferredPath;
        }

        const QString legacyPath = legacyTranslationFilePath(directory, candidateCulture);
        if (QFileInfo::exists(legacyPath)) {
            return legacyPath;
        }
    }

    for (const QString& legacyCulture : legacyCultureAliases(culture)) {
        const QString preferredPath = translationFilePath(directory, legacyCulture);
        if (QFileInfo::exists(preferredPath)) {
            return preferredPath;
        }

        const QString legacyPath = legacyTranslationFilePath(directory, legacyCulture);
        if (QFileInfo::exists(legacyPath)) {
            return legacyPath;
        }
    }

    QString fallbackCulture = canonicalCultureCode(culture);
    if (fallbackCulture.trimmed().isEmpty()) {
        fallbackCulture = culture.trimmed();
    }

    return translationFilePath(directory, fallbackCulture);
}

QString TranslationPathService::masterFilePath(const QString& directory)
{
    return existingTranslationFilePath(directory, fallbackCulture());
}

QStringList TranslationPathService::enumerateTranslationFiles(const QString& directory)
{
    QDir translationDirectory(directory);
    if (directory.trimmed().isEmpty() || !translationDirectory.exists()) {
        return {};
    }

    const QStringList preferredFiles = translationDirectory.entryList(
        QStringList{ filePrefix() + QStringLiteral("*") + fileExtension() },
        QDir::Files,
        QDir::Name | QDir::IgnoreCase);

    const QStringList selectedFiles = preferredFiles.isEmpty()
        ? translationDirectory.entryList(QStringList{ QStringLiteral("*") + fileExtension() }, QDir::Files, QDir::Name | QDir::IgnoreCase)
        : preferredFiles;

    QStringList paths;
    for (const QString& file : selectedFiles) {
        paths.append(translationDirectory.filePath(file));
    }

    return paths;
}

bool TranslationPathService::tryGetCultureFromPath(const QString& path, QString* culture)
{
    if (culture == nullptr) {
        return false;
    }

    culture->clear();
    if (path.trimmed().isEmpty()) {
        return false;
    }

    QString fileName = QFileInfo(path).completeBaseName();
    if (fileName.trimmed().isEmpty()) {
        return false;
    }

    if (fileName.startsWith(filePrefix(), Qt::CaseInsensitive)) {
        fileName = fileName.mid(filePrefix().size());
    }

    *culture = fileName.trimmed();
    return !culture->isEmpty();
}

QString TranslationPathService::normalizeCultureCode(const QString& culture)
{
    return culture.trimmed().replace(QLatin1Char('_'), QLatin1Char('-')).toLower();
}

QString TranslationPathService::canonicalCultureCode(const QString& culture)
{
    const QString normalized = normalizeCultureCode(culture);
    if (normalized.isEmpty()) {
        return {};
    }

    static const QMap<QString, QString> cultureAliases = {
        { QStringLiteral("cn"), QStringLiteral("zh-tw") },
        { QStringLiteral("zh"), QStringLiteral("zh-cn") },
        { QStringLiteral("zh-chs"), QStringLiteral("zh-cn") },
        { QStringLiteral("zh-cn"), QStringLiteral("zh-cn") },
        { QStringLiteral("zh-hans"), QStringLiteral("zh-cn") },
        { QStringLiteral("zh-sg"), QStringLiteral("zh-cn") },
        { QStringLiteral("zh-cht"), QStringLiteral("zh-tw") },
        { QStringLiteral("zh-hant"), QStringLiteral("zh-tw") },
        { QStringLiteral("zh-hk"), QStringLiteral("zh-tw") },
        { QStringLiteral("zh-mo"), QStringLiteral("zh-tw") },
        { QStringLiteral("zh-tw"), QStringLiteral("zh-tw") },
    };

    return cultureAliases.value(normalized, normalized);
}

QStringList TranslationPathService::cultureLookupChain(const QString& culture)
{
    QString currentCulture = normalizeCultureCode(culture);
    if (currentCulture.isEmpty()) {
        return {};
    }

    QStringList candidates;
    while (!currentCulture.isEmpty()) {
        addCultureCandidate(candidates, currentCulture);
        addCultureCandidate(candidates, canonicalCultureCode(currentCulture));

        const int separatorIndex = currentCulture.lastIndexOf(QLatin1Char('-'));
        if (separatorIndex < 0) {
            break;
        }

        currentCulture = currentCulture.left(separatorIndex);
    }

    return candidates;
}

QString TranslationPathService::resolveAvailableCulture(const QStringList& availableCultures, const QString& requestedCulture)
{
    for (const QString& candidateCulture : cultureLookupChain(requestedCulture)) {
        for (const QString& availableCulture : availableCultures) {
            if (availableCulture.compare(candidateCulture, Qt::CaseInsensitive) == 0) {
                return availableCulture;
            }
        }
    }

    return {};
}

TranslationFile TranslationFileService::loadFile(const QString& path)
{
    if (path.trimmed().isEmpty()) {
        throw std::invalid_argument("A translation file path is required.");
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error(QStringLiteral("Translation file not found: %1").arg(path).toStdString());
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        throw std::runtime_error(QStringLiteral("Translation file '%1' is not valid.").arg(path).toStdString());
    }

    const QJsonObject root = document.object();
    if (!root.value(QStringLiteral("Metadata")).isObject() || !root.value(QStringLiteral("Entries")).isObject()) {
        throw std::runtime_error(QStringLiteral("Translation file '%1' is not valid.").arg(path).toStdString());
    }

    TranslationFile translationFile;
    translationFile.metadata = metadataFromJson(root.value(QStringLiteral("Metadata")).toObject());

    const QJsonObject entries = root.value(QStringLiteral("Entries")).toObject();
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it.value().isObject()) {
            translationFile.entries[it.key()] = entryFromJson(it.value().toObject());
        }
    }

    return translationFile;
}

void TranslationFileService::saveFile(const TranslationFile& file, const QString& path)
{
    if (path.trimmed().isEmpty()) {
        throw std::invalid_argument("A translation file path is required.");
    }

    const QFileInfo fileInfo(path);
    if (!fileInfo.path().isEmpty()) {
        QDir().mkpath(fileInfo.path());
    }

    QJsonObject root;
    root[QStringLiteral("Metadata")] = metadataToJson(file.metadata);

    QJsonObject entries;
    for (auto it = file.entries.begin(); it != file.entries.end(); ++it) {
        entries[it.key()] = entryToJson(it.value());
    }
    root[QStringLiteral("Entries")] = entries;

    QSaveFile saveFile(path);
    if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error(QStringLiteral("Could not save translation file: %1").arg(path).toStdString());
    }

    saveFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!saveFile.commit()) {
        throw std::runtime_error(QStringLiteral("Could not commit translation file: %1").arg(path).toStdString());
    }
}

TranslationFile TranslationMaintenanceService::createTemplate(const TranslationFile& master, const QString& culture, const QString& translator)
{
    if (culture.trimmed().isEmpty()) {
        throw std::invalid_argument("A culture code is required.");
    }

    TranslationFile newFile;
    newFile.metadata.culture = culture;
    newFile.metadata.translator = translator;
    newFile.metadata.creationDate = QDateTime::currentDateTimeUtc();
    newFile.metadata.lastEditDate = newFile.metadata.creationDate;

    for (auto it = master.entries.begin(); it != master.entries.end(); ++it) {
        TranslationEntry entry;
        entry.source = it.value().source;
        entry.translation = it.value().source;
        entry.isTranslated = false;
        entry.notes = it.value().notes;
        newFile.entries[it.key()] = entry;
    }

    return newFile;
}

TranslationSyncResult TranslationMaintenanceService::synchronize(const TranslationFile& master, TranslationFile target)
{
    QMap<QString, TranslationEntry> syncedEntries;
    int addedCount = 0;
    int updatedCount = 0;

    for (auto it = master.entries.begin(); it != master.entries.end(); ++it) {
        const QString& key = it.key();
        const TranslationEntry& masterValue = it.value();

        if (target.entries.contains(key)) {
            TranslationEntry targetEntry = target.entries[key];
            if (targetEntry.source != masterValue.source) {
                targetEntry.source = masterValue.source;
                targetEntry.translation = masterValue.source;
                targetEntry.isTranslated = false;
                targetEntry.notes = masterValue.notes;
                updatedCount++;
            } else {
                targetEntry.notes = masterValue.notes;
            }
            syncedEntries[key] = targetEntry;
        } else {
            TranslationEntry entry;
            entry.source = masterValue.source;
            entry.translation = masterValue.source;
            entry.isTranslated = false;
            entry.notes = masterValue.notes;
            syncedEntries[key] = entry;
            addedCount++;
        }
    }

    int removedCount = 0;
    for (auto it = target.entries.begin(); it != target.entries.end(); ++it) {
        if (!master.entries.contains(it.key())) {
            removedCount++;
        }
    }

    target.entries = syncedEntries;
    target.metadata.lastEditDate = QDateTime::currentDateTimeUtc();

    TranslationSyncResult result;
    result.translationFile = target;
    result.addedCount = addedCount;
    result.updatedCount = updatedCount;
    result.removedCount = removedCount;
    return result;
}

JsonLocalizationService::JsonLocalizationService(const QString& translationFolder, const QMap<QString, QString>& englishDefaults)
{
    seedEnglishDefaults(englishDefaults);
    loadAllTranslations(translationFolder);
}

QMap<QString, QString> JsonLocalizationService::defaultEnglishDefaults()
{
    return {
        { QStringLiteral("UI.Common.Dialog.AreYouSureTitle"), QStringLiteral("Are you sure?") },
        { QStringLiteral("UI.Common.Dialog.ErrorTitle"), QStringLiteral("Error!") },
        { QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroup"), QStringLiteral("Check {0}... ({1}/{2})") },
        { QStringLiteral("UI.MainForm2.ContextMenu.CheckTrackGroupByFilter"), QStringLiteral("Check {0} by {1} ({2})...") },
        { QStringLiteral("UI.MainForm2.ContextMenu.UncheckTrackGroupByFilter"), QStringLiteral("Uncheck {0} by {1} ({2})...") },
        { QStringLiteral("UI.MainForm2.OutputDirectory.UseDefaultWithValue"), QStringLiteral("Use Currently Set Default Directory: ({0})") },
        { QStringLiteral("UI.OptionsForm.Patterns.Group"), QStringLiteral("Filename Patterns") },
        { QStringLiteral("UI.TranslationEditor.Actions.Load"), QStringLiteral("Load") },
    };
}

TranslationFile JsonLocalizationService::createBuiltInEnglishTranslationFile()
{
    TranslationFile translationFile;
    translationFile.metadata.culture = fallbackCulture();
    translationFile.metadata.creationDate = QDateTime::currentDateTimeUtc();
    translationFile.metadata.lastEditDate = translationFile.metadata.creationDate;

    const QMap<QString, QString> defaults = defaultEnglishDefaults();
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        TranslationEntry entry;
        entry.source = it.value();
        entry.translation = it.value();
        entry.isTranslated = true;
        translationFile.entries[it.key()] = entry;
    }

    return translationFile;
}

QStringList JsonLocalizationService::availableCultures() const
{
    QStringList cultures = m_runtimeCache.keys();
    cultures.sort(Qt::CaseInsensitive);
    return cultures;
}

QString JsonLocalizationService::resolveCultureName(const QString& cultureName) const
{
    const QString targetCulture = cultureName.trimmed().isEmpty()
        ? fallbackCulture()
        : cultureName;

    for (const QString& candidateCulture : TranslationPathService::cultureLookupChain(targetCulture)) {
        QString resolvedCulture;
        if (tryResolveCultureName(candidateCulture, &resolvedCulture)) {
            return resolvedCulture;
        }
    }

    return fallbackCulture();
}

QString JsonLocalizationService::getStringForCulture(const QString& key, const QString& cultureName) const
{
    const QString resolvedCulture = resolveCultureName(cultureName);
    const auto cultureIt = m_runtimeCache.constFind(resolvedCulture);
    if (cultureIt != m_runtimeCache.constEnd()) {
        const auto valueIt = cultureIt->constFind(key);
        if (valueIt != cultureIt->constEnd()) {
            return *valueIt;
        }
    }

    if (resolvedCulture.contains(QLatin1Char('-'))) {
        const QString neutralCulture = resolvedCulture.split(QLatin1Char('-')).first();
        const auto neutralIt = m_runtimeCache.constFind(neutralCulture);
        if (neutralIt != m_runtimeCache.constEnd()) {
            const auto valueIt = neutralIt->constFind(key);
            if (valueIt != neutralIt->constEnd()) {
                return *valueIt;
            }
        }
    }

    const auto fallbackIt = m_runtimeCache.constFind(fallbackCulture());
    if (fallbackIt != m_runtimeCache.constEnd()) {
        const auto valueIt = fallbackIt->constFind(key);
        if (valueIt != fallbackIt->constEnd()) {
            return *valueIt;
        }
    }

    return QStringLiteral("!%1!").arg(key);
}

QString JsonLocalizationService::getStringForCulture(const QString& key, const QString& cultureName, const QStringList& formatArgs) const
{
    return formatString(key, cultureName, getStringForCulture(key, cultureName), formatArgs);
}

void JsonLocalizationService::seedEnglishDefaults(const QMap<QString, QString>& englishDefaults)
{
    m_runtimeCache[fallbackCulture()] = englishDefaults;
}

void JsonLocalizationService::loadAllTranslations(const QString& translationFolder)
{
    if (!QDir(translationFolder).exists()) {
        Logger::log(QStringLiteral("Translation folder not found: %1. Using embedded English defaults only.").arg(translationFolder));
        return;
    }

    for (const QString& file : TranslationPathService::enumerateTranslationFiles(translationFolder)) {
        try {
            const TranslationFile translationFile = TranslationFileService::loadFile(file);
            const QString culture = TranslationPathService::canonicalCultureCode(translationFile.metadata.culture);
            if (culture.trimmed().isEmpty()) {
                continue;
            }

            QMap<QString, QString> flatDictionary = culture.compare(fallbackCulture(), Qt::CaseInsensitive) == 0
                ? m_runtimeCache.value(fallbackCulture())
                : QMap<QString, QString>();

            for (auto it = translationFile.entries.begin(); it != translationFile.entries.end(); ++it) {
                const TranslationEntry& entry = it.value();
                flatDictionary[it.key()] = !entry.translation.trimmed().isEmpty()
                    ? entry.translation
                    : entry.source;
            }

            m_runtimeCache[culture] = flatDictionary;
        } catch (const std::exception& ex) {
            Logger::log(QStringLiteral("Failed to load %1: %2").arg(file, QString::fromUtf8(ex.what())));
        }
    }
}

bool JsonLocalizationService::tryResolveCultureName(const QString& cultureName, QString* resolvedCulture) const
{
    for (auto it = m_runtimeCache.begin(); it != m_runtimeCache.end(); ++it) {
        if (it.key().compare(cultureName, Qt::CaseInsensitive) == 0) {
            if (resolvedCulture != nullptr) {
                *resolvedCulture = it.key();
            }
            return true;
        }
    }

    return false;
}

QString JsonLocalizationService::formatString(const QString& key, const QString& cultureName, const QString& format, const QStringList& formatArgs) const
{
    QString output = format;
    static const QRegularExpression placeholderRegex(QStringLiteral(R"(\{(\d+)\})"));
    QRegularExpressionMatchIterator iterator = placeholderRegex.globalMatch(format);

    while (iterator.hasNext()) {
        const QRegularExpressionMatch match = iterator.next();
        const int index = match.captured(1).toInt();
        if (index < 0 || index >= formatArgs.size()) {
            Logger::log(QStringLiteral("Localization format error for key '%1' in culture '%2'. Format: '%3'.")
                .arg(key, cultureName, format));
            return QStringLiteral("!BadFormat:%1!").arg(key);
        }
    }

    for (int index = 0; index < formatArgs.size(); ++index) {
        output.replace(QStringLiteral("{%1}").arg(index), formatArgs[index]);
    }

    return output;
}

void LocalizationManager::initialize(const QString& translationDirectory, const QString& culture)
{
    LocalizationState& state = localizationState();
    if (state.initialized) {
        Logger::log(QStringLiteral("LocalizationManager already initialized."));
        return;
    }

    state.translationDirectory = translationDirectory;
    Logger::log(QStringLiteral("Initializing LocalizationManager..."));
    reload(culture);
    Logger::log(QStringLiteral("LocalizationManager initialized successfully with culture: %1").arg(state.currentCulture));
}

void LocalizationManager::reload(const QString& culture)
{
    LocalizationState& state = localizationState();
    const QString targetCulture = culture.trimmed().isEmpty() ? state.currentCulture : culture;

    state.service = std::make_unique<JsonLocalizationService>(state.translationDirectory);
    const QString resolvedCulture = state.service->resolveCultureName(targetCulture);
    if (resolvedCulture.compare(targetCulture, Qt::CaseInsensitive) != 0) {
        Logger::log(QStringLiteral("Requested culture '%1' is not available. Falling back to '%2'.").arg(targetCulture, resolvedCulture));
    }

    state.currentCulture = resolvedCulture;
    state.initialized = true;
    Logger::log(QStringLiteral("LocalizationManager reloaded successfully with culture: %1").arg(state.currentCulture));
}

bool LocalizationManager::isInitialized()
{
    return localizationState().initialized;
}

QString LocalizationManager::currentCulture()
{
    return localizationState().currentCulture;
}

QStringList LocalizationManager::availableCultures()
{
    ensureInitialized();
    return localizationState().service->availableCultures();
}

QString LocalizationManager::getString(const QString& key)
{
    ensureInitialized();
    return localizationState().service->getStringForCulture(key, localizationState().currentCulture);
}

QString LocalizationManager::getString(const QString& key, const QStringList& formatArgs)
{
    ensureInitialized();
    return localizationState().service->getStringForCulture(key, localizationState().currentCulture, formatArgs);
}

QString LocalizationManager::getStringForCulture(const QString& key, const QString& culture)
{
    ensureInitialized();
    return localizationState().service->getStringForCulture(key, culture);
}

QString LocalizationManager::getStringForCulture(const QString& key, const QString& culture, const QStringList& formatArgs)
{
    ensureInitialized();
    return localizationState().service->getStringForCulture(key, culture, formatArgs);
}

void LocalizationManager::ensureInitialized()
{
    if (!localizationState().initialized || !localizationState().service) {
        throw std::logic_error("LocalizationManager not initialized. Call initialize() first.");
    }
}

}
