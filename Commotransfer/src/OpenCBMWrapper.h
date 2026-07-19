#pragma once

#include <QString>
#include <QStringList>
#include <QProcess>

/**
 * OpenCBMWrapper
 *
 * Thin C++ wrapper around the OpenCBM command-line tools (cbmcopy, cbmctrl,
 * d64copy) and nibtools (nibwrite).  All heavy operations are meant to be
 * called from a worker thread; the static helpers are safe from any thread.
 */
class OpenCBMWrapper
{
public:
    // ── File-type classification ────────────────────────────────────────────

    /** Disk file types understood by Commotransfer. */
    enum class DiskFileType {
        PRG,     ///< Single Commodore program file — written with cbmcopy
        D64,     ///< 1541 disk image (35 tracks) — written with d64copy
        G64,     ///< GCR disk image — written with nibtools nibwrite
        NIB,     ///< Raw nibble image — written with nibtools nibwrite
        Unknown
    };

    /** Classify a file by its extension (case-insensitive). */
    static DiskFileType fileTypeOf(const QString &path);

    /** Human-readable description of a DiskFileType. */
    static QString fileTypeDescription(DiskFileType t);

    // ── Installation checks ────────────────────────────────────────────────

    /** Returns the full path to a tool binary, or empty string if not found. */
    static QString findBinary(const QString &name);

    /** True if cbmcopy, cbmctrl, and d64copy are all found. */
    static bool isInstalled();

    /** True if nibtools nibwrite is found (needed for G64/NIB). */
    static bool isNibtoolsInstalled();

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
     * @param process      Caller-supplied QProcess so the caller can kill it.
     * @returns true if the process exited with code 0.
     */
    static bool writeFile(const QString &localPath,
                          int drive,
                          const QString &cbmName,
                          QStringList *outputLines,
                          QProcess    *process);

    /**
     * Write a disk image (.d64, .g64, or .nib) to the CBM drive.
     *
     * .d64  → d64copy -d <drive> <file> :
     * .g64  → nibwrite -d <drive> <file>
     * .nib  → nibwrite -d <drive> <file>
     *
     * @param localPath    Absolute path to the image file.
     * @param drive        CBM device number.
     * @param outputLines  Receives stdout/stderr lines as they arrive.
     * @param process      Caller-supplied QProcess so the caller can kill it.
     * @returns true if the process exited with code 0.
     */
    static bool writeDiskImage(const QString &localPath,
                               int drive,
                               QStringList *outputLines,
                               QProcess    *process);

    /**
     * Read a file from the CBM drive back to a local path.
     *
     * Equivalent to: cbmcopy -r -d <drive> <cbmName>
     * The file lands in <destDir>/<cbmName>.
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
