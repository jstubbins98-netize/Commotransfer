#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

/**
 * TransferWorker
 *
 * Runs in a dedicated QThread. Emits signals back to the main thread so the
 * UI can update without blocking.
 *
 * Usage:
 *   TransferWorker *worker = new TransferWorker();
 *   worker->moveToThread(&thread);
 *   connect(&thread, &QThread::started, worker, &TransferWorker::run);
 *   worker->setWriteParams(localPath, drive, cbmName, verify);
 *   thread.start();
 */
class TransferWorker : public QObject
{
    Q_OBJECT

public:
    enum class Mode { Write, Verify, Reset, Detect, Format };

    explicit TransferWorker(QObject *parent = nullptr);
    ~TransferWorker() override;

    void setWriteParams(const QString &localPath, int drive,
                        const QString &cbmName, bool verify);
    void setVerifyParams(const QString &localPath, int drive,
                         const QString &cbmName);
    void setFormatParams(const QString &diskName, const QString &diskId, int drive);
    void setMode(Mode m);

    /** Call from the main thread to request a graceful stop. */
    void requestCancel();

public slots:
    void run();

signals:
    /** Emitted for each line of output from the sub-process. */
    void logLine(const QString &line);

    /**
     * Emitted when the overall operation finishes.
     * @param success  true if the operation completed without errors.
     * @param message  Human-readable summary.
     */
    void finished(bool success, const QString &message);

    /** 0–100 progress; -1 means indeterminate. */
    void progress(int percent);

private:
    void doWrite();
    void doVerify();
    void doReset();
    void doDetect();
    void doFormat();

    QString   m_localPath;
    QString   m_cbmName;
    QString   m_diskName;
    QString   m_diskId;
    int       m_drive   = 8;
    bool      m_verify  = false;
    Mode      m_mode    = Mode::Write;
    bool      m_cancel  = false;
    QProcess *m_process = nullptr; // owned by this object during a run
};
