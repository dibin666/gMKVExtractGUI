#pragma once

#include "gmkvtoolnix/Jobs.h"
#include "gmkvtoolnix/Segments.h"
#include "gmkvtoolnix/Settings.h"

#include <QList>
#include <QMainWindow>
#include <QString>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QDragEnterEvent;
class QDropEvent;
class QGroupBox;
class QLineEdit;
class QMenu;
class QPoint;
class QProgressBar;
class QPushButton;
class QTextEdit;
class QThread;
class QTreeWidget;
class QTreeWidgetItem;

namespace gmkv::gui {

class ExtractionController;
class JobManagerWindow;
class LogWindow;
class OptionsDialog;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    enum class SegmentSelectionType
    {
        All,
        Video,
        Audio,
        Subtitles,
        Chapters,
        Attachments,
    };

    enum class TrackFilter
    {
        Language,
        LanguageIetf,
        CodecId,
        ExtraInfo,
        Name,
        Forced,
    };

    struct LoadedFile
    {
        QString filename;
        QList<gmkv::SegmentPtr> segments;
    };

    struct AnalyzedFile
    {
        QString filename;
        QList<gmkv::SegmentPtr> segments;
        QString error;
    };

    void buildUi();
    void showLogWindow();
    void showJobManagerWindow();
    void showOptionsDialog();
    void browseMkvToolNixPath();
    void autoDetectMkvToolNixPath();
    void addInputFiles();
    void addInputFiles(const QStringList& filenames, bool append);
    void addInputFile(const QString& filename);
    void appendAnalyzedFile(const QString& filename, const QList<gmkv::SegmentPtr>& segments);
    void finishAnalyzedFiles(const QList<AnalyzedFile>& results);
    void clearInputFiles();
    void appendSegmentItem(QTreeWidgetItem* parent, int fileIndex, int segmentIndex, const gmkv::SegmentPtr& segment);
    void reindexInputTree();
    void browseOutputDirectory();
    void showOutputDirectoryContextMenu(const QPoint& position);
    void setOutputDirectoryAsDefault();
    void useDefaultOutputDirectory();
    void applyLocalization();
    void applySettingsToUi();
    void saveMainSettings();
    void handleTreeItemChanged(QTreeWidgetItem* item, int column);
    void updateSelectedFileInformation();
    void showInputTreeContextMenu(const QPoint& position);
    void setSegmentChecks(SegmentSelectionType selectionType, bool checked);
    void addTrackFilterMenu(QMenu* menu, SegmentSelectionType selectionType, TrackFilter filter, bool checked);
    void setTrackChecksByFilter(SegmentSelectionType selectionType, TrackFilter filter, const QString& value, bool checked);
    QString trackFilterValue(const gmkv::Track& track, TrackFilter filter) const;
    void updateParentCheckState(QTreeWidgetItem* parent);
    QTreeWidgetItem* selectedFileItem() const;
    QString selectedFilePath() const;
    void removeSelectedFile();
    void openSelectedFile();
    void openSelectedFileFolder();
    void addSelectedJobs();
    void extractSelectedJobs();
    QList<gmkv::Job> createJobsFromSelection();
    gmkv::Job createJob(int fileIndex, const QList<gmkv::SegmentPtr>& selectedSegments) const;
    void runJobs(const QList<gmkv::Job>& jobs, bool showCompletionPopup);
    void setExtractionControlsEnabled(bool extracting);
    void abortExtraction();
    void abortAllExtractions();

    QLineEdit* m_mkvToolNixPathEdit = nullptr;
    QTreeWidget* m_inputTree = nullptr;
    QGroupBox* m_selectedFileInfoGroup = nullptr;
    QTextEdit* m_selectedFileInfoText = nullptr;
    QLineEdit* m_outputDirectoryEdit = nullptr;
    QComboBox* m_chapterTypeCombo = nullptr;
    QComboBox* m_extractionModeCombo = nullptr;
    QCheckBox* m_appendOnDragAndDropCheckBox = nullptr;
    QCheckBox* m_overwriteExistingFilesCheckBox = nullptr;
    QCheckBox* m_disableTooltipsCheckBox = nullptr;
    QCheckBox* m_useSourceDirectoryCheckBox = nullptr;
    QCheckBox* m_popupCheckBox = nullptr;
    QCheckBox* m_darkModeCheckBox = nullptr;
    QProgressBar* m_currentProgress = nullptr;
    QProgressBar* m_totalProgress = nullptr;
    QPushButton* m_addJobsButton = nullptr;
    QPushButton* m_extractButton = nullptr;
    QPushButton* m_abortButton = nullptr;
    QPushButton* m_abortAllButton = nullptr;
    ExtractionController* m_extractionController = nullptr;
    LogWindow* m_logWindow = nullptr;
    JobManagerWindow* m_jobManagerWindow = nullptr;
    QList<LoadedFile> m_loadedFiles;
    gmkv::Settings m_settings;
    QThread* m_analysisThread = nullptr;
    bool m_showCompletionPopupForCurrentRun = false;
    bool m_updatingTreeChecks = false;
};

}
