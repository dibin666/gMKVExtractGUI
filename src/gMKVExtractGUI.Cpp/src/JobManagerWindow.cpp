#include "gmkvextractgui/JobManagerWindow.h"

#include "gmkvextractgui/ExtractionController.h"
#include "gmkvextractgui/UiLocalization.h"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace gmkv::gui {

JobManagerWindow::JobManagerWindow(QWidget* parent)
    : QDialog(parent)
    , m_settings(QCoreApplication::applicationDirPath())
{
    m_settings.reload();

    setWindowTitle(QStringLiteral("Job Manager"));
    setWindowTitleKey(this, QStringLiteral("UI.JobManager.Title"));
    resize(820, 460);

    auto* layout = new QVBoxLayout(this);
    m_jobsTable = new QTableWidget(0, 4, this);
    m_jobsTable->setHorizontalHeaderLabels({
        QStringLiteral("State"),
        QStringLiteral("Job"),
        QStringLiteral("Start"),
        QStringLiteral("End"),
    });
    m_jobsTable->horizontalHeader()->setStretchLastSection(true);
    m_jobsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_jobsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_jobsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_jobsTable, 1);

    auto* progressLayout = new QVBoxLayout();
    auto* currentProgressLayout = new QHBoxLayout();
    auto* totalProgressLayout = new QHBoxLayout();
    m_currentProgressBar = new QProgressBar(this);
    m_currentProgressBar->setRange(0, 100);
    m_totalProgressBar = new QProgressBar(this);
    m_totalProgressBar->setRange(0, 100);
    m_currentTrackLabel = new QLabel(QStringLiteral("Ready"), this);
    auto* currentProgressLabel = new QLabel(QStringLiteral("Current Progress:"), this);
    setTextKey(currentProgressLabel, QStringLiteral("UI.JobManager.Progress.CurrentProgress"));
    currentProgressLayout->addWidget(currentProgressLabel);
    currentProgressLayout->addWidget(m_currentProgressBar, 1);
    auto* totalProgressLabel = new QLabel(QStringLiteral("Total Progress:"), this);
    setTextKey(totalProgressLabel, QStringLiteral("UI.JobManager.Progress.TotalProgress"));
    totalProgressLayout->addWidget(totalProgressLabel);
    totalProgressLayout->addWidget(m_totalProgressBar, 1);
    totalProgressLayout->addWidget(m_currentTrackLabel, 1);
    progressLayout->addLayout(currentProgressLayout);
    progressLayout->addLayout(totalProgressLayout);
    layout->addLayout(progressLayout);

    auto* buttons = new QHBoxLayout();
    m_runAllButton = new QPushButton(QStringLiteral("Run All"), this);
    setTextKey(m_runAllButton, QStringLiteral("UI.JobManager.Actions.RunJobs"));
    m_removeButton = new QPushButton(QStringLiteral("Remove"), this);
    setTextKey(m_removeButton, QStringLiteral("UI.JobManager.Actions.Remove"));
    m_loadButton = new QPushButton(QStringLiteral("Load..."), this);
    setTextKey(m_loadButton, QStringLiteral("UI.JobManager.Actions.LoadJobs"));
    m_saveButton = new QPushButton(QStringLiteral("Save..."), this);
    setTextKey(m_saveButton, QStringLiteral("UI.JobManager.Actions.SaveJobs"));
    m_abortButton = new QPushButton(QStringLiteral("Abort"), this);
    setTextKey(m_abortButton, QStringLiteral("UI.JobManager.Actions.Abort"));
    m_abortAllButton = new QPushButton(QStringLiteral("Abort All"), this);
    setTextKey(m_abortAllButton, QStringLiteral("UI.JobManager.Actions.AbortAll"));
    m_popupCheckBox = new QCheckBox(QStringLiteral("Popup"), this);
    setTextKey(m_popupCheckBox, QStringLiteral("UI.JobManager.Actions.Popup"));
    m_popupCheckBox->setChecked(m_settings.showPopupInJobManager);
    auto* closeButton = new QPushButton(QStringLiteral("Close"), this);
    buttons->addWidget(m_runAllButton);
    buttons->addWidget(m_removeButton);
    buttons->addWidget(m_loadButton);
    buttons->addWidget(m_saveButton);
    buttons->addWidget(m_abortButton);
    buttons->addWidget(m_abortAllButton);
    buttons->addWidget(m_popupCheckBox);
    buttons->addStretch(1);
    buttons->addWidget(closeButton);
    layout->addLayout(buttons);

    m_extractionController = new ExtractionController(this);
    connect(m_extractionController, &ExtractionController::started, this, [this](int) {
        setRunningControlsEnabled(true);
        m_currentProgressBar->setValue(0);
        m_totalProgressBar->setValue(0);
        m_currentTrackLabel->setText(QStringLiteral("Starting extraction"));
    });
    connect(m_extractionController, &ExtractionController::jobStarted, this, &JobManagerWindow::handleJobStarted);
    connect(m_extractionController, &ExtractionController::jobCompleted, this, &JobManagerWindow::handleJobCompleted);
    connect(m_extractionController, &ExtractionController::progressUpdated, this, [this](int currentProgress, int totalProgress) {
        m_currentProgressBar->setValue(currentProgress);
        m_totalProgressBar->setValue(totalProgress);
    });
    connect(m_extractionController, &ExtractionController::trackUpdated, this, [this](const QString& filename, const QString& trackName) {
        m_currentTrackLabel->setText(QStringLiteral("%1 from %2").arg(trackName, QFileInfo(filename).fileName()));
    });
    connect(m_extractionController, &ExtractionController::finished, this, &JobManagerWindow::handleRunFinished);

    connect(m_removeButton, &QPushButton::clicked, this, &JobManagerWindow::removeSelectedJobs);
    connect(m_jobsTable, &QTableWidget::customContextMenuRequested, this, &JobManagerWindow::showJobsContextMenu);
    connect(m_loadButton, &QPushButton::clicked, this, &JobManagerWindow::loadJobs);
    connect(m_saveButton, &QPushButton::clicked, this, &JobManagerWindow::saveJobs);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(m_runAllButton, &QPushButton::clicked, this, &JobManagerWindow::runAllJobs);
    connect(m_abortButton, &QPushButton::clicked, m_extractionController, &ExtractionController::abortCurrent);
    connect(m_abortAllButton, &QPushButton::clicked, m_extractionController, &ExtractionController::abortAll);
    connect(m_popupCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        m_settings.showPopupInJobManager = checked;
        m_settings.save();
    });

    setRunningControlsEnabled(false);
    refresh();
    applyLocalization();
}

bool JobManagerWindow::addJob(const gmkv::JobInfo& jobInfo)
{
    const bool added = m_queue.addJob(jobInfo);
    refresh();
    return added;
}

void JobManagerWindow::closeEvent(QCloseEvent* event)
{
    if (m_extractionController->isRunning()) {
        const QMessageBox::StandardButton result = QMessageBox::question(
            this,
            QStringLiteral("Job Manager"),
            QStringLiteral("Jobs are still running. Abort all jobs?"));
        if (result == QMessageBox::Yes) {
            m_extractionController->abortAll();
        }
        event->ignore();
        return;
    }

    QDialog::closeEvent(event);
}

void JobManagerWindow::refresh()
{
    const QList<gmkv::JobInfo> jobs = m_queue.jobs();
    applyLocalization();
    m_jobsTable->setRowCount(jobs.size());

    for (qsizetype row = 0; row < jobs.size(); ++row) {
        const gmkv::JobInfo& job = jobs[row];
        m_jobsTable->setItem(row, 0, new QTableWidgetItem(gmkv::toString(job.state)));
        m_jobsTable->setItem(row, 1, new QTableWidgetItem(job.job.toString()));
        m_jobsTable->setItem(row, 2, new QTableWidgetItem(job.startTime.isValid() ? job.startTime.toString(Qt::ISODate) : QString()));
        m_jobsTable->setItem(row, 3, new QTableWidgetItem(job.endTime.isValid() ? job.endTime.toString(Qt::ISODate) : QString()));
    }

    m_jobsTable->resizeColumnsToContents();
}

void JobManagerWindow::saveJobs()
{
    if (m_extractionController->isRunning()) {
        QMessageBox::warning(this, QStringLiteral("Job Manager"), QStringLiteral("Jobs cannot be saved while extraction is running."));
        return;
    }

    const QString filename = QFileDialog::getSaveFileName(this, loc(QStringLiteral("UI.JobManager.Dialogs.SelectJobsFileTitle")), QString(), QStringLiteral("XML Files (*.xml);;All Files (*)"));
    if (!filename.isEmpty()) {
        gmkv::JobXmlService::save(m_queue.jobs(), filename);
    }
}

void JobManagerWindow::loadJobs()
{
    if (m_extractionController->isRunning()) {
        QMessageBox::warning(this, QStringLiteral("Job Manager"), QStringLiteral("Jobs cannot be loaded while extraction is running."));
        return;
    }

    const QString filename = QFileDialog::getOpenFileName(this, loc(QStringLiteral("UI.JobManager.Dialogs.SelectJobFileTitle")), QString(), QStringLiteral("XML Files (*.xml);;All Files (*)"));
    if (!filename.isEmpty()) {
        m_queue.setJobs(gmkv::JobXmlService::load(filename));
        refresh();
    }
}

void JobManagerWindow::removeSelectedJobs()
{
    if (m_extractionController->isRunning()) {
        QMessageBox::warning(this, QStringLiteral("Job Manager"), QStringLiteral("Jobs cannot be removed while extraction is running."));
        return;
    }

    QList<int> selectedRows;
    for (const QModelIndex& index : m_jobsTable->selectionModel()->selectedRows()) {
        selectedRows.append(index.row());
    }
    std::sort(selectedRows.begin(), selectedRows.end(), std::greater<int>());

    QList<gmkv::JobInfo> jobs = m_queue.jobs();
    for (int row : selectedRows) {
        if (row >= 0 && row < jobs.size()) {
            jobs.removeAt(row);
        }
    }
    m_queue.setJobs(jobs);
    refresh();
}

void JobManagerWindow::showJobsContextMenu(const QPoint& position)
{
    QMenu menu(this);
    QAction* selectAllAction = menu.addAction(loc(QStringLiteral("UI.JobManager.Jobs.SelectAll")), m_jobsTable, &QTableWidget::selectAll);
    QAction* deselectAllAction = menu.addAction(loc(QStringLiteral("UI.JobManager.Jobs.DeselectAll")), m_jobsTable, &QTableWidget::clearSelection);
    menu.addSeparator();
    QAction* readyAction = menu.addAction(loc(QStringLiteral("UI.JobManager.Jobs.ChangeToReadyStatus")), this, &JobManagerWindow::changeSelectedJobsToReady);

    const bool hasJobs = m_jobsTable->rowCount() > 0;
    const bool hasSelection = m_jobsTable->selectionModel() != nullptr
        && !m_jobsTable->selectionModel()->selectedRows().isEmpty();
    selectAllAction->setEnabled(hasJobs);
    deselectAllAction->setEnabled(hasSelection);
    readyAction->setEnabled(hasSelection && !m_extractionController->isRunning());

    menu.exec(m_jobsTable->viewport()->mapToGlobal(position));
}

void JobManagerWindow::applyLocalization()
{
    gmkv::gui::applyLocalization(this);
    m_jobsTable->setHorizontalHeaderLabels({
        loc(QStringLiteral("UI.JobManager.Columns.State")),
        loc(QStringLiteral("UI.JobManager.Columns.Job")),
        loc(QStringLiteral("UI.JobManager.Columns.StartTime")),
        loc(QStringLiteral("UI.JobManager.Columns.EndTime")),
    });
}

void JobManagerWindow::changeSelectedJobsToReady()
{
    if (m_extractionController->isRunning()) {
        return;
    }

    QList<gmkv::JobInfo> jobs = m_queue.jobs();
    for (const QModelIndex& index : m_jobsTable->selectionModel()->selectedRows()) {
        const int row = index.row();
        if (row < 0 || row >= jobs.size()) {
            continue;
        }

        jobs[row].state = gmkv::JobState::Ready;
        jobs[row].startTime = {};
        jobs[row].endTime = {};
    }
    m_queue.setJobs(jobs);
    refresh();
}

void JobManagerWindow::runAllJobs()
{
    if (m_extractionController->isRunning()) {
        QMessageBox::warning(this, QStringLiteral("Job Manager"), QStringLiteral("Jobs are already running."));
        return;
    }

    QList<gmkv::JobInfo> jobs = m_queue.jobs();
    QList<gmkv::Job> jobsToRun;
    m_runningRows.clear();

    for (qsizetype row = 0; row < jobs.size(); ++row) {
        if (jobs[row].state != gmkv::JobState::Ready) {
            continue;
        }

        jobs[row].state = gmkv::JobState::Pending;
        jobs[row].startTime = {};
        jobs[row].endTime = {};
        jobsToRun.append(jobs[row].job);
        m_runningRows.append(static_cast<int>(row));
    }

    if (jobsToRun.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Job Manager"), QStringLiteral("There are no ready jobs to run."));
        return;
    }

    m_queue.setJobs(jobs);
    refresh();
    m_extractionController->start(jobsToRun);
}

void JobManagerWindow::handleJobStarted(int jobIndex, const QString& filename)
{
    if (jobIndex < 0 || jobIndex >= m_runningRows.size()) {
        return;
    }

    const int row = m_runningRows[jobIndex];
    updateJob(row, [](gmkv::JobInfo& jobInfo) {
        jobInfo.state = gmkv::JobState::Running;
        jobInfo.startTime = QDateTime::currentDateTime();
    });

    m_currentTrackLabel->setText(QStringLiteral("Extracting %1").arg(QFileInfo(filename).fileName()));
    m_jobsTable->selectRow(row);
}

void JobManagerWindow::handleJobCompleted(int jobIndex)
{
    if (jobIndex < 0 || jobIndex >= m_runningRows.size()) {
        return;
    }

    updateJob(m_runningRows[jobIndex], [](gmkv::JobInfo& jobInfo) {
        jobInfo.state = gmkv::JobState::Completed;
        jobInfo.endTime = QDateTime::currentDateTime();
    });
}

void JobManagerWindow::handleRunFinished(bool aborted, const QStringList& errors)
{
    QList<gmkv::JobInfo> jobs = m_queue.jobs();
    for (int row : m_runningRows) {
        if (row < 0 || row >= jobs.size()) {
            continue;
        }

        if (jobs[row].state == gmkv::JobState::Running) {
            jobs[row].state = aborted ? gmkv::JobState::Ready : gmkv::JobState::Failed;
            jobs[row].endTime = QDateTime::currentDateTime();
        } else if (jobs[row].state == gmkv::JobState::Pending) {
            jobs[row].state = gmkv::JobState::Ready;
        }
    }

    m_queue.setJobs(jobs);
    m_runningRows.clear();
    setRunningControlsEnabled(false);

    if (aborted) {
        m_currentTrackLabel->setText(QStringLiteral("Extraction aborted"));
    } else if (!errors.isEmpty()) {
        m_currentTrackLabel->setText(QStringLiteral("Extraction failed"));
        QMessageBox::critical(this, QStringLiteral("Job Manager"), errors.join(QLatin1Char('\n')));
    } else {
        m_currentTrackLabel->setText(QStringLiteral("Extraction completed"));
        if (m_popupCheckBox->isChecked()) {
            QMessageBox::information(this, loc(QStringLiteral("UI.JobManager.Title")), loc(QStringLiteral("UI.JobManager.Success.JobsCompleted")));
        }
    }

    refresh();
}

void JobManagerWindow::setRunningControlsEnabled(bool running)
{
    m_runAllButton->setEnabled(!running);
    m_removeButton->setEnabled(!running);
    m_loadButton->setEnabled(!running);
    m_saveButton->setEnabled(!running);
    m_abortButton->setEnabled(running);
    m_abortAllButton->setEnabled(running);
}

void JobManagerWindow::updateJob(int row, const std::function<void(gmkv::JobInfo&)>& updater)
{
    QList<gmkv::JobInfo> jobs = m_queue.jobs();
    if (row < 0 || row >= jobs.size()) {
        return;
    }

    updater(jobs[row]);
    m_queue.setJobs(jobs);
    refresh();
}

}
