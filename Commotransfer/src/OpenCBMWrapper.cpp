#include "OpenCBMWrapper.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>

// ── Search paths ──────────────────────────────────────────────────────────────

QStringList OpenCBMWrapper::searchPaths()
{
    return {
        "/opt/homebrew/bin", // Homebrew Apple Silicon
        "/usr/local/bin",    // Homebrew Intel
        "/usr/bin",
        "/bin"
    };
}

QString OpenCBMWrapper::findBinary(const QString &name)
{
    for (const QString &dir : searchPaths()) {
        QString full = dir + "/" + name;
        if (QFile::exists(full))
            return full;
    }
    return QStandardPaths::findExecutable(name);
}

// ── File-type classification ──────────────────────────────────────────────────

OpenCBMWrapper::DiskFileType OpenCBMWrapper::fileTypeOf(const QString &path)
{
    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "prg") return DiskFileType::PRG;
    if (ext == "d64") return DiskFileType::D64;
    if (ext == "g64") return DiskFileType::G64;
    if (ext == "nib") return DiskFileType::NIB;
    return DiskFileType::Unknown;
}

QString OpenCBMWrapper::fileTypeDescription(DiskFileType t)
{
    switch (t) {
        case DiskFileType::PRG: return "Commodore PRG program file";
        case DiskFileType::D64: return "D64 1541 disk image";
        case DiskFileType::G64: return "G64 GCR disk image";
        case DiskFileType::NIB: return "NIB raw nibble image";
        default:                return "Unknown file type";
    }
}

// ── Installation checks ───────────────────────────────────────────────────────

bool OpenCBMWrapper::isInstalled()
{
    return !findBinary("cbmcopy").isEmpty()
        && !findBinary("cbmctrl").isEmpty()
        && !findBinary("d64copy").isEmpty();
}

bool OpenCBMWrapper::isNibtoolsInstalled()
{
    return !findBinary("nibwrite").isEmpty();
}

bool OpenCBMWrapper::isHomebrewInstalled()
{
    return !findBinary("brew").isEmpty();
}

// ── Drive helpers ─────────────────────────────────────────────────────────────

QString OpenCBMWrapper::detectDrives(QString *errorOutput)
{
    QString cbmctrl = findBinary("cbmctrl");
    if (cbmctrl.isEmpty()) {
        if (errorOutput) *errorOutput = "cbmctrl not found";
        return {};
    }

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(cbmctrl, {"detect"});
    proc.waitForFinished(8000);

    QString out = QString::fromLocal8Bit(proc.readAll());
    if (proc.exitCode() != 0 && errorOutput)
        *errorOutput = out;
    return out;
}

bool OpenCBMWrapper::resetBus(QString *errorOutput)
{
    QString cbmctrl = findBinary("cbmctrl");
    if (cbmctrl.isEmpty()) {
        if (errorOutput) *errorOutput = "cbmctrl not found";
        return false;
    }
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(cbmctrl, {"reset"});
    proc.waitForFinished(8000);
    if (proc.exitCode() != 0 && errorOutput)
        *errorOutput = QString::fromLocal8Bit(proc.readAll());
    return proc.exitCode() == 0;
}

// ── Transfer helpers ──────────────────────────────────────────────────────────

bool OpenCBMWrapper::runProcess(const QString &program,
                                const QStringList &args,
                                QStringList *outputLines,
                                QProcess    *process)
{
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(program, args);

    if (!process->waitForStarted(5000))
        return false;

    while (process->state() != QProcess::NotRunning) {
        process->waitForReadyRead(100);
        while (process->canReadLine()) {
            QString line = QString::fromLocal8Bit(process->readLine()).trimmed();
            if (!line.isEmpty() && outputLines)
                outputLines->append(line);
        }
        QCoreApplication::processEvents();
    }

    // Drain any remaining output
    while (process->canReadLine()) {
        QString line = QString::fromLocal8Bit(process->readLine()).trimmed();
        if (!line.isEmpty() && outputLines)
            outputLines->append(line);
    }
    QString tail = QString::fromLocal8Bit(process->readAll()).trimmed();
    if (!tail.isEmpty() && outputLines)
        outputLines->append(tail);

    return process->exitCode() == 0;
}

bool OpenCBMWrapper::writeFile(const QString &localPath,
                               int drive,
                               const QString &cbmName,
                               QStringList *outputLines,
                               QProcess    *process)
{
    QString cbmcopy = findBinary("cbmcopy");
    if (cbmcopy.isEmpty()) {
        if (outputLines) outputLines->append("ERROR: cbmcopy not found");
        return false;
    }

    QStringList args = {"-w", "-d", QString::number(drive)};

    process->setWorkingDirectory(QFileInfo(localPath).absolutePath());

    if (!cbmName.isEmpty()) {
        // cbmcopy -w -d 8 "local.prg,cbmname,p"
        QString cbmArg = QFileInfo(localPath).fileName() + "," + cbmName + ",p";
        args << cbmArg;
    } else {
        args << QFileInfo(localPath).fileName();
    }

    return runProcess(cbmcopy, args, outputLines, process);
}

bool OpenCBMWrapper::writeDiskImage(const QString &localPath,
                                    int drive,
                                    QStringList *outputLines,
                                    QProcess    *process)
{
    DiskFileType type = fileTypeOf(localPath);

    if (type == DiskFileType::D64) {
        // d64copy -d <drive> <image.d64> :
        // The trailing ":" tells d64copy to use the IEC bus drive.
        QString d64copy = findBinary("d64copy");
        if (d64copy.isEmpty()) {
            if (outputLines) outputLines->append("ERROR: d64copy not found. Is OpenCBM installed?");
            return false;
        }
        QStringList args = {"-d", QString::number(drive), localPath, ":"};
        return runProcess(d64copy, args, outputLines, process);
    }

    if (type == DiskFileType::G64 || type == DiskFileType::NIB) {
        // nibwrite -d <drive> <imagefile>
        QString nibwrite = findBinary("nibwrite");
        if (nibwrite.isEmpty()) {
            if (outputLines) outputLines->append(
                "ERROR: nibwrite not found. Install nibtools: brew install nibtools");
            return false;
        }
        QStringList args = {"-d", QString::number(drive), localPath};
        return runProcess(nibwrite, args, outputLines, process);
    }

    if (outputLines) outputLines->append("ERROR: Unsupported image format");
    return false;
}

bool OpenCBMWrapper::readFile(const QString &cbmName,
                              int drive,
                              const QString &destDir,
                              QStringList *outputLines,
                              QProcess    *process)
{
    QString cbmcopy = findBinary("cbmcopy");
    if (cbmcopy.isEmpty()) {
        if (outputLines) outputLines->append("ERROR: cbmcopy not found");
        return false;
    }

    process->setWorkingDirectory(destDir);
    QStringList args = {"-r", "-d", QString::number(drive), cbmName};
    return runProcess(cbmcopy, args, outputLines, process);
}

// ── Formatting ────────────────────────────────────────────────────────────────

bool OpenCBMWrapper::formatDisk(const QString &diskName,
                                const QString &diskId,
                                int drive,
                                QStringList *outputLines,
                                QProcess    *process)
{
    QString cbmformat = findBinary("cbmformat");
    if (cbmformat.isEmpty()) {
        if (outputLines) outputLines->append("ERROR: cbmformat not found");
        return false;
    }

    QString nameArg = diskName + "," + diskId;
    QStringList args = {"-d", QString::number(drive), nameArg};
    return runProcess(cbmformat, args, outputLines, process);
}

// ── Utility ───────────────────────────────────────────────────────────────────

QString OpenCBMWrapper::deriveCBMName(const QString &localPath)
{
    QFileInfo fi(localPath);
    QString base = fi.baseName();
    base = base.toUpper();
    base = base.left(16);
    base.replace(QRegularExpression("[^A-Z0-9 _\\-]"), "_");
    return base;
}

QString OpenCBMWrapper::sha256OfFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&f))
        return {};
    return hash.result().toHex();
}
