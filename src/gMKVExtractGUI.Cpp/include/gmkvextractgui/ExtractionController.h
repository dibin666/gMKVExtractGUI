#pragma once

#include "gmkvtoolnix/Jobs.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include <atomic>
#include <mutex>

class QThread;

namespace gmkv {
class MkvExtractRunner;
}

namespace gmkv::gui {

class ExtractionController final : public QObject
{
    Q_OBJECT

public:
    explicit ExtractionController(QObject* parent = nullptr);

    bool isRunning() const;
    void start(const QList<gmkv::Job>& jobs);
    void abortCurrent();
    void abortAll();

signals:
    void started(int totalJobs);
    void jobStarted(int jobIndex, const QString& filename);
    void jobCompleted(int jobIndex);
    void progressUpdated(int currentProgress, int totalProgress);
    void trackUpdated(const QString& filename, const QString& trackName);
    void finished(bool aborted, const QStringList& errors);

private:
    gmkv::MkvExtractRunResult runJobInWorker(const gmkv::Job& job, int jobIndex, int totalJobs);
    void setActiveRunner(gmkv::MkvExtractRunner* runner);

    QThread* m_thread = nullptr;
    gmkv::MkvExtractRunner* m_activeRunner = nullptr;
    mutable std::mutex m_activeRunnerMutex;
    std::atomic_bool m_abortAllRequested = false;
};

}
