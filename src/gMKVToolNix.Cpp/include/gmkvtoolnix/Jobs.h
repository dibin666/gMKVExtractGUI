#pragma once

#include "gmkvtoolnix/MkvExtract.h"

#include <QDateTime>
#include <QList>
#include <QString>

namespace gmkv {

enum class FormMkvExtractionMode
{
    Tracks,
    CueSheet,
    Tags,
    Timecodes,
    TracksAndTimecodes,
    Cues,
    TracksAndCues,
    TracksAndCuesAndTimecodes,
};

enum class JobState
{
    Ready,
    Pending,
    Running,
    Completed,
    Failed,
};

struct Job
{
    FormMkvExtractionMode extractionMode = FormMkvExtractionMode::Tracks;
    QString mkvToolNixPath;
    ExtractSegmentsParameters parameters;

    QString toString() const;
};

struct JobInfo
{
    Job job;
    QDateTime startTime;
    QDateTime endTime;
    JobState state = JobState::Ready;

    void reset();
};

QString toString(FormMkvExtractionMode extractionMode);
FormMkvExtractionMode extractionModeFromString(const QString& value, FormMkvExtractionMode fallback = FormMkvExtractionMode::Tracks);
QString toString(JobState state);
JobState jobStateFromString(const QString& value, JobState fallback = JobState::Ready);

bool jobsEqual(const Job& left, const Job& right);

class JobQueue
{
public:
    bool addJob(const JobInfo& jobInfo);
    QList<JobInfo> jobs() const;
    void setJobs(QList<JobInfo> jobs);
    void clear();

private:
    QList<JobInfo> m_jobs;
};

class JobXmlService
{
public:
    static void save(const QList<JobInfo>& jobs, const QString& filename);
    static QList<JobInfo> load(const QString& filename);
};

}
