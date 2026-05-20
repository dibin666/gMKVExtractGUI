#pragma once

#include <QDateTime>
#include <QMap>
#include <QString>
#include <QStringList>

namespace gmkv {

struct Metadata
{
    QString culture;
    QString translator;
    QDateTime creationDate;
    QDateTime lastEditDate;
};

struct TranslationEntry
{
    QString source;
    QString translation;
    bool isTranslated = false;
    QString notes;
};

struct TranslationFile
{
    Metadata metadata;
    QMap<QString, TranslationEntry> entries;
};

struct TranslationSyncResult
{
    TranslationFile translationFile;
    int addedCount = 0;
    int updatedCount = 0;
    int removedCount = 0;
};

class TranslationPathService
{
public:
    static QString filePrefix();
    static QString translationFileName(const QString& culture);
    static QString translationFilePath(const QString& directory, const QString& culture);
    static QString existingTranslationFilePath(const QString& directory, const QString& culture);
    static QString masterFilePath(const QString& directory);
    static QStringList enumerateTranslationFiles(const QString& directory);
    static bool tryGetCultureFromPath(const QString& path, QString* culture);
    static QString normalizeCultureCode(const QString& culture);
    static QString canonicalCultureCode(const QString& culture);
    static QStringList cultureLookupChain(const QString& culture);
    static QString resolveAvailableCulture(const QStringList& availableCultures, const QString& requestedCulture);
};

class TranslationFileService
{
public:
    static TranslationFile loadFile(const QString& path);
    static void saveFile(const TranslationFile& file, const QString& path);
};

class TranslationMaintenanceService
{
public:
    static TranslationFile createTemplate(const TranslationFile& master, const QString& culture, const QString& translator = QString());
    static TranslationSyncResult synchronize(const TranslationFile& master, TranslationFile target);
};

class JsonLocalizationService
{
public:
    explicit JsonLocalizationService(const QString& translationFolder, const QMap<QString, QString>& englishDefaults = defaultEnglishDefaults());

    static QMap<QString, QString> defaultEnglishDefaults();
    static TranslationFile createBuiltInEnglishTranslationFile();

    QStringList availableCultures() const;
    QString resolveCultureName(const QString& cultureName) const;
    QString getStringForCulture(const QString& key, const QString& cultureName) const;
    QString getStringForCulture(const QString& key, const QString& cultureName, const QStringList& formatArgs) const;

private:
    void seedEnglishDefaults(const QMap<QString, QString>& englishDefaults);
    void loadAllTranslations(const QString& translationFolder);
    bool tryResolveCultureName(const QString& cultureName, QString* resolvedCulture) const;
    QString formatString(const QString& key, const QString& cultureName, const QString& format, const QStringList& formatArgs) const;

    QMap<QString, QMap<QString, QString>> m_runtimeCache;
};

class LocalizationManager
{
public:
    static void initialize(const QString& translationDirectory, const QString& culture = QStringLiteral("en"));
    static void reload(const QString& culture = QString());
    static bool isInitialized();
    static QString currentCulture();
    static QStringList availableCultures();
    static QString getString(const QString& key);
    static QString getString(const QString& key, const QStringList& formatArgs);
    static QString getStringForCulture(const QString& key, const QString& culture);
    static QString getStringForCulture(const QString& key, const QString& culture, const QStringList& formatArgs);

private:
    static void ensureInitialized();
};

}
