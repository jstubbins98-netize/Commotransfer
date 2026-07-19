#include "TransferWorker.h"
#include "OpenCBMWrapper.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>

TransferWorker::TransferWorker(QObject *parent)
    : QObject(parent)
{}

TransferWorker::~TransferWorker()
{
    requestCancel();
}

void TransferWorker::setWriteParams(const QString &localPath, int drive,
                                    const QString &cbmName, bool verify)
{
    m_localPath = localPath;
    m_drive     = drive;
    m_cbmName   = cbmName.isEmpty() ? OpenCBMWrapper::deriveCBMName(localPath) : cbmName;
    m_verify    = verify;
    m_mode      = Mode::Write;
}

void TransferWorker::setVerifyParams(const QString &localPath, int drive,
                                     const QString &cbmName)
{
    m_localPath = localPath;
    m_drive     = drive;
    m_cbmName   = cbmName.isEmpty() ? OpenCBMWrapper::deriveCBMName(localPath) : cbmName;
    m_mode      = Mode::Verify;
}

void TransferWorker::setFormatParams(const QString &diskName,
                                     const QString &diskId,
                                     int drive)
{
    m_diskName = diskName;
    m_diskId   = diskId;
    m_drive    = drive;
    m_mode     = Mode::Format;
}

void TransferWorker::setMode(Mode m) { m_mode = m; }

void TransferWorker::requestCancel()
{
    m_cancel = true;
    if (m_process && m_process->state() != QProcess::NotRunning)
        m_process->kill();
}

// ── Entry point ───────────────────────────────────────────────────────────────

void TransferWorker::run()
{
    m_cancel = false;
    switch (m_mode) {
        case Mode::Write:  doWrite();  break;
        case Mode::Verify: doVerify(); break;
        case Mode::Reset:  doReset();  break;
        case Mode::Detect: doDetect(); break;
        case Mode::Format: doFormat(); break;
    }
}

// ── Write ─────────────────────────────────────────────────────────────────────

void TransferWorker::doWrite()
{
    emit logLine("──────────────────────────────────────────");
    emit logLine(QString("Writing  : %1").arg(QFileInfo(m_localPath).fileName()));
    emit logLine(QString("CBM name : %1").arg(m_cbmName));
    emit logLine(QString("Drive    : %1").arg(m_drive));
    emit logLine("──────────────────────────────────────────");
    emit progress(-1);

    QStringList lines;
    QProcess proc;
    m_process = &proc;

    // Connect process output streaming
    connect(&proc, &QProcess::readyReadStandardOutput, this, [&]() {
        while (proc.canReadLine()) {
            QString l = QString::fromLocal8Bit(proc.readLine()).trimmed();
            if (!l.isEmpty()) emit logLine(l);
        }
    });
    connect(&proc, &QProcess::readyReadStandardError, this, [&]() {
        while (proc.canReadLine()) {
            QString l = QString::fromLocal8Bit(proc.readLine()).trimmed();
            if (!l.isEmpty()) emit logLine("  " + l);
        }
    });

    bool ok = OpenCBMWrapper::writeFile(m_localPath, m_drive, m_cbmName, &lines, &proc);
    for (const QString &l : lines) emit logLine(l);
    m_process = nullptr;

    if (m_cancel) {
        emit finished(false, "Transfer cancelled.");
        return;
    }

    if (!ok) {
        emit logLine("✗ Write failed.");
        emit finished(false, "Write failed. Check the log for details.");
        return;
    }

    emit logLine("✓ Write complete.");

    if (m_verify && !m_cancel) {
        emit logLine("");
        doVerify();
    } else {
        emit progress(100);
        emit finished(true, "File written successfully.");
    }
}

// ── Verify ────────────────────────────────────────────────────────────────────

void TransferWorker::doVerify()
{
    emit logLine("──────────────────────────────────────────");
    emit logLine(QString("Verifying: %1").arg(m_cbmName));
    emit logLine("──────────────────────────────────────────");
    emit progress(-1);

    // Compute checksum of the original file
    QString originalHash = OpenCBMWrapper::sha256OfFile(m_localPath);
    if (originalHash.isEmpty()) {
        emit logLine("✗ Cannot read original file for verification.");
        emit finished(false, "Verification failed: cannot read source file.");
        return;
    }
    emit logLine(QString("Source SHA-256 : %1").arg(originalHash));

    // Read the file back from the disk into a temp directory
    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        emit logLine("✗ Cannot create temporary directory.");
        emit finished(false, "Verification failed: temp dir error.");
        return;
    }

    QStringList lines;
    QProcess proc;
    m_process = &proc;

    connect(&proc, &QProcess::readyReadStandardOutput, this, [&]() {
        while (proc.canReadLine()) {
            QString l = QString::fromLocal8Bit(proc.readLine()).trimmed();
            if (!l.isEmpty()) emit logLine(l);
        }
    });
    connect(&proc, &QProcess::readyReadStandardError, this, [&]() {
        while (proc.canReadLine()) {
            QString l = QString::fromLocal8Bit(proc.readLine()).trimmed();
            if (!l.isEmpty()) emit logLine("  " + l);
        }
    });

    bool ok = OpenCBMWrapper::readFile(m_cbmName, m_drive, tmp.path(), &lines, &proc);
    for (const QString &l : lines) emit logLine(l);
    m_process = nullptr;

    if (m_cancel) {
        emit finished(false, "Verification cancelled.");
        return;
    }

    if (!ok) {
        emit logLine("✗ Read-back from drive failed.");
        emit finished(false, "Verification failed: could not read file from disk.");
        return;
    }

    // cbmcopy writes the file as the CBM name (no extension)
    // Try several common filename patterns
    QStringList candidates = {
        tmp.path() + "/" + m_cbmName,
        tmp.path() + "/" + m_cbmName + ".prg",
        tmp.path() + "/" + m_cbmName.toLower(),
        tmp.path() + "/" + m_cbmName.toLower() + ".prg",
    };

    QString readBackPath;
    for (const QString &c : candidates) {
        if (QFile::exists(c)) { readBackPath = c; break; }
    }

    if (readBackPath.isEmpty()) {
        // Search the tmp dir for any file
        QDir dir(tmp.path());
        QStringList entries = dir.entryList(QDir::Files);
        if (!entries.isEmpty())
            readBackPath = tmp.path() + "/" + entries.first();
    }

    if (readBackPath.isEmpty()) {
        emit logLine("✗ Read-back file not found in temp directory.");
        emit finished(false, "Verification failed: read-back file missing.");
        return;
    }

    QString readBackHash = OpenCBMWrapper::sha256OfFile(readBackPath);
    emit logLine(QString("Disk SHA-256   : %1").arg(readBackHash));

    if (readBackHash == originalHash) {
        emit logLine("✓ Verification passed — disk contents match source file.");
        emit progress(100);
        emit finished(true, "Write verified successfully.");
    } else {
        emit logLine("✗ Verification FAILED — checksums do not match!");
        emit finished(false, "Verification failed: data mismatch detected.");
    }
}

// ── Format ────────────────────────────────────────────────────────────────────

void TransferWorker::doFormat()
{
    emit logLine("──────────────────────────────────────────");
    emit logLine(QString("Formatting disk on drive %1").arg(m_drive));
    emit logLine(QString("Disk name : %1").arg(m_diskName));
    emit logLine(QString("Disk ID   : %2").arg(m_diskId));
    emit logLine("⚠ This will erase all data on the disk.");
    emit logLine("──────────────────────────────────────────");
    emit progress(-1);

    QStringList lines;
    QProcess proc;
    m_process = &proc;

    connect(&proc, &QProcess::readyReadStandardOutput, this, [&]() {
        while (proc.canReadLine()) {
            QString l = QString::fromLocal8Bit(proc.readLine()).trimmed();
            if (!l.isEmpty()) emit logLine(l);
        }
    });
    connect(&proc, &QProcess::readyReadStandardError, this, [&]() {
        while (proc.canReadLine()) {
            QString l = QString::fromLocal8Bit(proc.readLine()).trimmed();
            if (!l.isEmpty()) emit logLine("  " + l);
        }
    });

    bool ok = OpenCBMWrapper::formatDisk(m_diskName, m_diskId, m_drive, &lines, &proc);
    for (const QString &l : lines) emit logLine(l);
    m_process = nullptr;

    if (m_cancel) {
        emit finished(false, "Format cancelled.");
        return;
    }

    emit progress(100);

    if (ok) {
        emit logLine("✓ Disk formatted successfully.");
        emit finished(true, QString("Disk \"%1\" formatted.").arg(m_diskName));
    } else {
        emit logLine("✗ Format failed. Check the log for details.");
        emit finished(false, "Format failed.");
    }
}

// ── Reset ─────────────────────────────────────────────────────────────────────

void TransferWorker::doReset()
{
    emit logLine("Resetting IEC bus…");
    emit progress(-1);
    QString err;
    bool ok = OpenCBMWrapper::resetBus(&err);
    if (ok)
        emit logLine("✓ Bus reset complete.");
    else
        emit logLine("✗ Bus reset failed: " + err);
    emit progress(100);
    emit finished(ok, ok ? "Bus reset." : "Bus reset failed.");
}

// ── Detect ────────────────────────────────────────────────────────────────────

void TransferWorker::doDetect()
{
    emit logLine("Scanning IEC bus for drives…");
    emit progress(-1);
    QString err;
    QString out = OpenCBMWrapper::detectDrives(&err);
    if (out.isEmpty()) {
        emit logLine("✗ No response from bus. Is the ZoomFloppy connected and the drive powered on?");
        if (!err.isEmpty()) emit logLine("  " + err);
        emit finished(false, "No drives detected.");
    } else {
        for (const QString &l : out.split('\n', Qt::SkipEmptyParts))
            emit logLine(l);
        emit logLine("✓ Scan complete.");
        emit finished(true, "Scan complete.");
    }
    emit progress(100);
}
