#pragma once

#include "gmkvtoolnix/Jobs.h"
#include "gmkvtoolnix/Settings.h"

#include <QDialog>
#include <QList>
#include <QStringList>

#include <functional>

class QCloseEvent;
class QCheckBox;
class QLabel;
class QPoint;
class QProgressBar;
class QPushButton;
class QTableWidget;

namespace gmkv::gui {

class ExtractionController;

class JobManagerWindow final : public QDialog
{
    Q_OBJECT

public:
    explicit JobManagerWindow(QWidget* parent = nullptr);

    bool addJob(const gmkv::JobInfo& jobInfo);
    void applyLocalization();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void refresh();
    void saveJobs();
    void loadJobs();
    void removeSelectedJobs();
    void showJobsContextMenu(const QPoint& position);
    void changeSelectedJobsToReady();
    void runAllJobs();
    void handleJobStarted(int jobIndex, const QString& filename);
    void handleJobCompleted(int jobIndex);
    void handleRunFinished(bool aborted, const QStringList& errors);
    void setRunningControlsEnabled(bool running);
    void updateJob(int row, const std::function<void(gmkv::JobInfo&)>& updater);

    gmkv::JobQueue m_queue;
    gmkv::Settings m_settings;
    ExtractionController* m_extractionController = nullptr;
    QTableWidget* m_jobsTable = nullptr;
    QProgressBar* m_currentProgressBar = nullptr;
    QProgressBar* m_totalProgressBar = nullptr;
    QLabel* m_currentTrackLabel = nullptr;
    QCheckBox* m_popupCheckBox = nullptr;
    QPushButton* m_runAllButton = nullptr;
    QPushButton* m_removeButton = nullptr;
    QPushButton* m_loadButton = nullptr;
    QPushButton* m_saveButton = nullptr;
    QPushButton* m_abortButton = nullptr;
    QPushButton* m_abortAllButton = nullptr;
    QList<int> m_runningRows;
};

}
