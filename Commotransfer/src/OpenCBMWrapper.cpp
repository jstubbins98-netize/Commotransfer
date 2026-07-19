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

// ── Search paths ─────────────────────────────────────────────────────────────

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
    // Fall back to PATH search via QStandardPaths
    QString found = QStandardPaths::findExecutable(name);
    return found;
}

bool OpenCBMWrapper::isInstalled()
{
    return !findBinary("cbmcopy").isEmpty() && !findBinary("cbmctrl").isEmpty();
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

    // Stream output line-by-line
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

    // If caller supplied an explicit CBM name, rename on the fly:
    //   cbmcopy -w -d 8 "localfile.prg,cbmname,p"
    if (!cbmName.isEmpty()) {
        QString cbmArg = QFileInfo(localPath).fileName() + "," + cbmName + ",p";
        // cbmcopy accepts "localfilename,cbmname,type" in the filename arg
        // We still need the actual file path, so run from the file's directory
        process->setWorkingDirectory(QFileInfo(localPath).absolutePath());
        args << cbmArg;
    } else {
        process->setWorkingDirectory(QFileInfo(localPath).absolutePath());
        args << QFileInfo(localPath).fileName();
    }

    return runProcess(cbmcopy, args, outputLines, process);
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

// ── Utility ───────────────────────────────────────────────────────────────────

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

    // cbmformat -d <drive> "<name>,<id>"
    QString nameArg = diskName + "," + diskId;
    QStringList args = {"-d", QString::number(drive), nameArg};
    return runProcess(cbmformat, args, outputLines, process);
}

QString OpenCBMWrapper::deriveCBMName(const QString &localPath)
{
    QFileInfo fi(localPath);
    QString base = fi.baseName();             // strip path + extension
    base = base.toUpper();                    // CBM names are uppercase
    base = base.left(16);                     // max 16 characters
    // Replace characters illegal on CBM with underscores
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
