#include "gmkvextractgui/ExtractionController.h"

#include "gmkvtoolnix/MkvExtract.h"
#include "gmkvtoolnix/MkvToolNix.h"

#include <QFileInfo>
#include <QMetaObject>
#include <QScopeGuard>
#include <QThread>

#include <exception>

namespace gmkv::gui {

namespace {

QList<gmkv::PlannedExtractCommand> planCommandsForJob(const gmkv::Job& job, const gmkv::Version& version)
{
    switch (job.extractionMode) {
    case gmkv::FormMkvExtractionMode::CueSheet:
        return { gmkv::MkvExtractPlanner::planCueSheet(job.parameters, version) };
    case gmkv::FormMkvExtractionMode::Tags:
        return { gmkv::MkvExtractPlanner::planTags(job.parameters, version) };
    case gmkv::FormMkvExtractionMode::Tracks:
    case gmkv::FormMkvExtractionMode::Timecodes:
    case gmkv::FormMkvExtractionMode::TracksAndTimecodes:
    case gmkv::FormMkvExtractionMode::Cues:
    case gmkv::FormMkvExtractionMode::TracksAndCues:
    case gmkv::FormMkvExtractionMode::TracksAndCuesAndTimecodes:
        return gmkv::MkvExtractPlanner::planSegments(job.parameters, version);
    }
    return {};
}

QStringList resultErrors(const gmkv::MkvExtractRunResult& result)
{
    QStringList errors = result.errors;
    if (!result.errorString.trimmed().isEmpty() && !errors.contains(result.errorString)) {
        errors.prepend(result.errorString);
    }
    return errors;
}

}

ExtractionController::ExtractionController(QObject* parent)
    : QObject(parent)
{
}

bool ExtractionController::isRunning() const
{
    return m_thread != nullptr;
}

void ExtractionController::start(const QList<gmkv::Job>& jobs)
{
    if (jobs.isEmpty() || m_thread != nullptr) {
        return;
    }

    m_abortAllRequested.store(false);
    emit started(jobs.size());

    m_thread = QThread::create([this, jobs]() {
        QStringList errors;
        bool aborted = false;

        for (qsizetype jobIndex = 0; jobIndex < jobs.size(); ++jobIndex) {
            if (m_abortAllRequested.load()) {
                aborted = true;
                errors.append(QStringLiteral("User aborted all the processes!"));
                break;
            }

            QMetaObject::invokeMethod(this, [this, jobIndex, filename = jobs[jobIndex].parameters.mkvFile]() {
                emit jobStarted(static_cast<int>(jobIndex), filename);
            }, Qt::QueuedConnection);

            const gmkv::MkvExtractRunResult result = runJobInWorker(jobs[jobIndex], static_cast<int>(jobIndex), jobs.size());
            if (!result.succeeded()) {
                aborted = result.aborted;
                errors.append(resultErrors(result));
                break;
            }

            QMetaObject::invokeMethod(this, [this, jobIndex, totalJobs = jobs.size()]() {
                emit progressUpdated(100, static_cast<int>(((jobIndex + 1) * 100) / totalJobs));
                emit jobCompleted(static_cast<int>(jobIndex));
            }, Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(this, [this, errors, aborted]() {
            m_thread = nullptr;
            setActiveRunner(nullptr);
            emit finished(aborted, errors);
        }, Qt::QueuedConnection);
    });

    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);
    m_thread->start();
}

void ExtractionController::abortCurrent()
{
    std::lock_guard<std::mutex> lock(m_activeRunnerMutex);
    if (m_activeRunner != nullptr) {
        m_activeRunner->abort();
    }
}

void ExtractionController::abortAll()
{
    m_abortAllRequested.store(true);
    std::lock_guard<std::mutex> lock(m_activeRunnerMutex);
    if (m_activeRunner != nullptr) {
        m_activeRunner->abortAll();
    }
}

gmkv::MkvExtractRunResult ExtractionController::runJobInWorker(const gmkv::Job& job, int jobIndex, int totalJobs)
{
    gmkv::MkvExtractRunResult result;
    try {
        const gmkv::Version version = gmkv::MkvToolVersionService::readVersion(job.mkvToolNixPath, gmkv::MkvTool::Extract);
        const QList<gmkv::PlannedExtractCommand> commands = planCommandsForJob(job, version);
        if (commands.isEmpty()) {
            result.errorString = QStringLiteral("No extractable commands were generated for %1.").arg(QFileInfo(job.parameters.mkvFile).fileName());
            result.errors.append(result.errorString);
            return result;
        }

        gmkv::MkvExtractRunner runner(gmkv::MkvToolNix::executablePath(job.mkvToolNixPath, gmkv::MkvTool::Extract));
        setActiveRunner(&runner);
        const auto clearActiveRunner = qScopeGuard([this]() {
            setActiveRunner(nullptr);
        });

        connect(&runner, &gmkv::MkvExtractRunner::progressUpdated, this, [this, jobIndex, totalJobs](int progress) {
            emit progressUpdated(progress, static_cast<int>(((jobIndex * 100) + progress) / totalJobs));
        }, Qt::QueuedConnection);
        connect(&runner, &gmkv::MkvExtractRunner::trackUpdated, this, [this](const QString& filename, const QString& trackName) {
            emit trackUpdated(filename, trackName);
        }, Qt::QueuedConnection);

        result = runner.run(commands);
    } catch (const std::exception& ex) {
        result.errorString = QString::fromUtf8(ex.what());
        result.errors.append(result.errorString);
    }

    return result;
}

void ExtractionController::setActiveRunner(gmkv::MkvExtractRunner* runner)
{
    std::lock_guard<std::mutex> lock(m_activeRunnerMutex);
    m_activeRunner = runner;
}

}
