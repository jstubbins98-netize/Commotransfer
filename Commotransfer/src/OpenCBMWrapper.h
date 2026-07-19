#pragma once

#include <QString>
#include <QStringList>
#include <QProcess>

/**
 * OpenCBMWrapper
 *
 * Thin C++ wrapper around the OpenCBM command-line tools (cbmcopy, cbmctrl).
 * All heavy operations (write, read) are meant to be called from a worker
 * thread; the static helpers are safe to call from any thread.
 */
class OpenCBMWrapper
{
public:
    // ── Installation checks ────────────────────────────────────────────────

    /** Returns the full path to a tool binary, or empty string if not found. */
    static QString findBinary(const QString &name);

    /** True if cbmcopy and cbmctrl are both found on PATH. */
    static bool isInstalled();

    /** True if Homebrew brew(1) command is available. */
    static bool isHomebrewInstalled();

    // ── Drive / device helpers ─────────────────────────────────────────────

    /**
     * Run `cbmctrl detect` and return raw output.
     * @param errorOutput  Populated with stderr on failure.
     */
    static QString detectDrives(QString *errorOutput = nullptr);

    /**
     * Send a bus reset: `cbmctrl reset`
     * Returns true on success.
     */
    static bool resetBus(QString *errorOutput = nullptr);

    // ── Disk formatting ────────────────────────────────────────────────────

    /**
     * Format a disk using cbmformat.
     *
     * Equivalent to: cbmformat -d <drive> "<diskName>,<diskId>"
     *
     * @param diskName    Up to 16 characters, any content.
     * @param diskId      Exactly 2 characters (e.g. "01").
     * @param drive       CBM device number.
     * @param outputLines Receives stdout/stderr lines as they arrive.
     * @param process     Caller-supplied QProcess so the caller can kill it.
     * @returns true if the process exited with code 0.
     */
    static bool formatDisk(const QString &diskName,
                           const QString &diskId,
                           int drive,
                           QStringList *outputLines,
                           QProcess    *process);

    // ── Transfer operations ────────────────────────────────────────────────

    /**
     * Write a single local .prg file to the CBM drive.
     *
     * Equivalent to: cbmcopy -w -d <drive> <localPath>
     *
     * @param localPath    Absolute path to the local .prg file.
     * @param drive        CBM device number (usually 8–11).
     * @param cbmName      Overrides the on-disk filename (≤16 chars, uppercase).
     *                     If empty, cbmcopy derives it from the local filename.
     * @param outputLines  Receives stdout/stderr lines as they arrive.
     *                     Pass nullptr to discard.
     * @param process      Caller-supplied QProcess so the caller can kill it.
     * @returns true if the process exited with code 0.
     */
    static bool writeFile(const QString &localPath,
                          int drive,
                          const QString &cbmName,
                          QStringList *outputLines,
                          QProcess    *process);

    /**
     * Read a file from the CBM drive back to a local path.
     *
     * Equivalent to: cbmcopy -r -d <drive> <cbmName>
     * The file lands in <destDir>/<cbmName> (cbmcopy creates the file there).
     *
     * @param cbmName      CBM filename to read (as reported by the directory).
     * @param drive        CBM device number.
     * @param destDir      Directory where the read-back file will be placed.
     * @param outputLines  Receives stdout/stderr lines as they arrive.
     * @param process      Caller-supplied QProcess.
     * @returns true on success.
     */
    static bool readFile(const QString &cbmName,
                         int drive,
                         const QString &destDir,
                         QStringList *outputLines,
                         QProcess    *process);

    // ── Utility ────────────────────────────────────────────────────────────

    /**
     * Derive the CBM filename from a local path using the same rules cbmcopy
     * applies: uppercase, max 16 characters, strip leading path components.
     */
    static QString deriveCBMName(const QString &localPath);

    /**
     * Compute the SHA-256 hex digest of a file's contents.
     * Returns empty string on error.
     */
    static QString sha256OfFile(const QString &path);

private:
    /** Shared helper: run a process, stream lines to outputLines. */
    static bool runProcess(const QString &program,
                           const QStringList &args,
                           QStringList *outputLines,
                           QProcess    *process);

    /** Search directories for a named binary. */
    static QStringList searchPaths();
};
