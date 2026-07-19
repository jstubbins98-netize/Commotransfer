# Commotransfer — User Manual

**Version 1.0.0**

---

## Table of Contents

1. Overview
2. How It Works
3. System Requirements
4. Installation
   - 4.1 Install Homebrew
   - 4.2 Install Qt and CMake
   - 4.3 Build Commotransfer
   - 4.4 First-Run Setup Wizard
5. Hardware Setup
   - 5.1 What You Need
   - 5.2 Connecting the ZoomFloppy
   - 5.3 Drive Device Numbers
6. The Main Window
   - 6.1 File Drop Zone
   - 6.2 Transfer Settings
   - 6.3 Action Buttons
   - 6.4 Activity Log
   - 6.5 Status Bar
   - 6.6 Menu Bar
7. Writing a File to Disk
8. Verifying a Transfer
9. Drive Utilities
   - 9.1 Detect Drives
   - 9.2 Reset IEC Bus
10. CBM Filenames Explained]
11. Under the Hood
    - 11.1 OpenCBM
    - 11.2 Writing
    - 11.3 Verification
    - 11.4 The ZoomFloppy
    - 11.5 Application Architecture
12. Troubleshooting
13. Glossary

---

## 1. Overview

**Commotransfer** is a native macOS application that lets you write Commodore 64 `.prg` program files from your Mac directly to real 5¼" floppy disks using a **ZoomFloppy** USB adapter. After writing, it can optionally read the file back from the disk and verify the transfer was byte-perfect using a SHA-256 checksum comparison.

Commotransfer is a graphical front-end for the open-source **OpenCBM** toolkit. It handles all the command-line details for you — you just drag a file, pick a drive number, and click Write.

**What Commotransfer can do:**

- Write any `.prg` file to a Commodore floppy disk (1541, 1571, and compatible drives)
- Optionally verify the written file by reading it back and comparing checksums
- Detect drives connected via the ZoomFloppy adapter
- Reset the IEC serial bus if a drive becomes unresponsive
- Install and configure OpenCBM automatically on first launch

**What Commotransfer does not do:**

- Format disks (use `cbmformat` in Terminal for that)
- Read full disk images (use `d64copy` for `.d64` image transfers)
- Work with Commodore 128, Plus/4, or VIC-20 specific formats (though basic PRG transfer may still work)
- Operate without a ZoomFloppy or compatible OpenCBM-supported USB adapter

---

## 2. How It Works

```
┌──────────────┐   USB (HID)   ┌─────────────┐   IEC Serial   ┌──────────────┐
│     Mac      │ ────────────▶ │ ZoomFloppy  │ ─────────────▶ │ 1541 / 1571  │
│ Commotransfer│               │  Adapter    │                 │    Drive     │
└──────────────┘               └─────────────┘                 └──────────────┘
       │                             │
       │  cbmcopy / cbmctrl          │  Commodore IEC bus
       │  (via OpenCBM)              │  (clock, data, ATN lines)
       ▼                             ▼
  Local .prg file              5¼" floppy disk
```

1. You select a `.prg` file in Commotransfer.
2. Commotransfer calls `cbmcopy` (part of OpenCBM) with the file path and target drive number.
3. `cbmcopy` communicates with the ZoomFloppy over USB using OpenCBM's USB driver.
4. The ZoomFloppy translates USB signals into the Commodore **IEC serial bus** protocol.
5. The drive writes the file to the floppy disk in Commodore PRG format.
6. Optionally, `cbmcopy` reads the file back and Commotransfer compares checksums.

---

## 3. System Requirements

| Component | Minimum |
|---|---|
| **macOS** | 12 Monterey or later (Intel or Apple Silicon) |
| **Homebrew** | Any recent version — [brew.sh](https://brew.sh) |
| **Qt** | Qt 6 (Qt 5.15 also supported as fallback) |
| **CMake** | 3.16 or later |
| **ZoomFloppy** | USB adapter from [go4retro.com](http://www.go4retro.com/products/zoomfloppy/) |
| **Drive** | Commodore 1541, 1571, or compatible 5¼" IEC drive |
| **Disk** | Formatted Commodore 1541 double-sided single-density (DSSD) diskette |

> **Apple Silicon note:** Commotransfer and OpenCBM run natively on Apple Silicon Macs via Homebrew's `/opt/homebrew` prefix. No Rosetta required.

---

## 4. Installation

### 4.1 Install Homebrew

If Homebrew is not already installed, open **Terminal** and run:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

Follow the on-screen instructions. On Apple Silicon, Homebrew installs to `/opt/homebrew`. On Intel Macs it uses `/usr/local`.

After installation, follow any instructions to add Homebrew to your PATH (Homebrew will print these if needed).

### 4.2 Install Qt and CMake

```bash
brew install qt cmake
```

This installs Qt 6 and CMake. The build script detects these automatically.

### 4.3 Build Commotransfer

Navigate to the `Commotransfer` project directory and run the build script:

```bash
cd Commotransfer
bash build.sh
```

The script will:
1. Locate `cmake` and Qt on your system
2. Configure the build with CMake
3. Compile the application using all available CPU cores
4. Report the path to the finished `Commotransfer.app`

**Additional build options:**

```bash
bash build.sh --clean   # Delete the build/ folder and rebuild from scratch
bash build.sh --run     # Build and immediately launch the app
```

Once built, you can copy `build/Commotransfer.app` to your `/Applications` folder like any other macOS app.

### 4.4 First-Run Setup Wizard

The first time Commotransfer launches, the **Setup Wizard** opens automatically. It has four steps:

**Step 1 — Welcome**
Describes what the wizard will do. If OpenCBM is already installed on your system, the wizard skips straight to the connection test.

**Step 2 — Homebrew check**
Confirms that Homebrew is present. If not found, it shows a button to open [brew.sh](https://brew.sh) and waits. Quit and reopen the wizard after installing Homebrew — it detects it automatically.

**Step 3 — Install OpenCBM**
Click **Install OpenCBM**. The wizard runs:

```bash
brew install opencbm
```

Live output from Homebrew scrolls in the log area. This step requires an internet connection and may take a few minutes. Unlike MacPorts, Homebrew does **not** require your administrator password for package installation.

**Step 4 — Connect ZoomFloppy**
Plug in the ZoomFloppy, connect it to your drive, power the drive on, then click **Detect Drives**. The wizard runs `cbmctrl detect` and shows any drives it finds. If no drives are found you can still continue — you can detect drives later from the main window.

**Step 5 — Done**
The wizard confirms everything is ready. Click **Open Commotransfer** to launch the main application. The wizard will not appear again on subsequent launches.

> **Re-running the wizard:** You can open the wizard at any time from **Help → Run Setup Wizard** in the menu bar — useful after a macOS upgrade or if OpenCBM gets uninstalled.

---

## 5. Hardware Setup

### 5.1 What You Need

- A **ZoomFloppy** USB adapter
- A Commodore **IEC serial cable** (the round 6-pin DIN connector used by C64 peripherals)
- A **Commodore 1541** or 1571 floppy drive (or compatible), with its own power supply
- A formatted **5¼" double-sided single-density (DSSD)** diskette

### 5.2 Connecting the ZoomFloppy

1. Connect the IEC serial cable from the **ZoomFloppy's IEC port** to the **drive's IEC port**.
2. Plug the ZoomFloppy into any available **USB port** on your Mac.
3. Connect the drive's **power supply** and **switch it on**.
4. Wait a few seconds for the drive's motor to settle (the red LED will stop flashing).

The drive's activity light may briefly flash when the IEC bus is initialised — this is normal.

> **Multiple drives:** You can chain multiple drives on the IEC bus. Each drive needs a unique device number (see below). Only one ZoomFloppy is needed for the whole chain.

### 5.3 Drive Device Numbers

Commodore IEC drives are addressed by a device number set by jumpers or a switch on the drive:

| Device number | Typical use |
|---|---|
| 8 | First (or only) drive — the default |
| 9 | Second drive |
| 10 | Third drive |
| 11 | Fourth drive |

Most 1541 drives ship configured as device 8. Check your drive's manual or the jumper markings on the circuit board if you need to change it.

---

## 6. The Main Window

```
┌─────────────────────────────────────────────────────────────────────┐
│  File   Drive   Log   Help                                [menu bar] │
├──────────────────────────────┬──────────────────────────────────────┤
│  Commotransfer               │  Activity Log               [Clear]  │
│  Write .prg files to real…   │                                      │
│                              │  [10:42:01] Commotransfer ready.     │
│  ┌──────────────────────┐    │  [10:42:01] OpenCBM tools found:…   │
│  │                      │    │  [10:42:15] File selected: game.prg  │
│  │   Drop a .prg file   │    │  [10:43:02] ──────────────────────   │
│  │   here or click to   │    │  [10:43:02] Writing  : game.prg      │
│  │       browse         │    │  [10:43:02] CBM name : GAME          │
│  │                      │    │  [10:43:02] Drive    : 8             │
│  └──────────────────────┘    │  [10:43:02] ──────────────────────   │
│                              │  [10:43:45] ✓ Write complete.        │
│  Transfer Settings           │  [10:43:45] Verifying: GAME          │
│  Drive number:  [ 8 ▲▼]     │  [10:44:10] ✓ Verification passed    │
│  CBM filename:  [        ]   │                                      │
│  ☑ Verify after write        │                                      │
│                              │                                      │
│  [ Write to Disk ]           │                                      │
│  [ Verify Only ] [ Cancel ]  │                                      │
│  ▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░ (prog)│                                      │
├──────────────────────────────┴──────────────────────────────────────┤
│  OpenCBM ready                                            [status]   │
└─────────────────────────────────────────────────────────────────────┘
```

### 6.1 File Drop Zone

The large box in the upper-left is the **file drop zone**. You can:

- **Drag and drop** a `.prg` file directly from Finder onto it
- **Click** anywhere in the box to open a file picker dialog

When a file is loaded, the box shows the filename and its folder path. The **CBM filename** field (see below) auto-fills with the derived on-disk name.

Only `.prg` files are accepted. Dragging any other file type will be rejected.

### 6.2 Transfer Settings

**Drive number**
The Commodore device number of the target drive (8–11). Use the arrows or type directly. Default is `8`.

**CBM filename**
The name the file will have on the Commodore disk. This field is optional — leave it blank and Commotransfer derives the name automatically from the local filename (see [CBM Filenames Explained](#10-cbm-filenames-explained)).

If you type a name here it will be converted to uppercase automatically (CBM filenames are always uppercase). Maximum 16 characters.

**Verify after write**
When checked (default), Commotransfer reads the file back from the disk immediately after writing and compares its SHA-256 checksum against the original. A mismatch is reported as a failure. This is strongly recommended — floppy media can be unreliable and this is the only way to be certain the transfer was successful.

### 6.3 Action Buttons

**Write to Disk**
Starts the write operation (and verify, if enabled). Enabled only when a file is loaded and no operation is already running.

**Verify Only**
Reads the file back from the disk and checks its checksum without writing. Useful if you want to confirm an already-written disk is still good.

**Format Disk…**
Opens the two-step format dialogs (see [Formatting a Disk](#formatting-a-disk)). Available whenever no other operation is running. Disabled during active transfers.

**Cancel**
Appears during an active operation. Sends a termination signal to the running process and aborts the operation. The disk may be left in a partial or inconsistent state after cancellation.

### 6.4 Activity Log

The right panel shows a timestamped log of everything Commotransfer does. Each entry is prefixed with the current time (`[HH:MM:SS]`).

- **Normal lines** — standard output from `cbmcopy` / `cbmctrl`
- **✓ green lines** — successful completion of an operation
- **✗ orange lines** — errors or warnings
- **Red lines** — critical failures

Click **Clear** in the log header to wipe the log. You can also select and copy text from the log for troubleshooting purposes.

### 6.5 Status Bar

The thin bar at the bottom of the window shows the result of the most recent operation (e.g. "File written successfully." or "Verification failed: data mismatch detected."). When no operation is running it shows the OpenCBM status ("OpenCBM ready" or "OpenCBM not installed").

### 6.6 Menu Bar

**File**
- **Open .prg File…** (`⌘O`) — Opens the file picker (same as clicking the drop zone)
- **Quit** (`⌘Q`)

**Drive**
- **Detect Drives** — Runs `cbmctrl detect` and logs results
- **Reset IEC Bus** — Runs `cbmctrl reset`

**Log**
- **Clear Log** — Clears the activity log

**Help**
- **Run Setup Wizard…** — Re-opens the first-run setup wizard

---

## 7. Writing a File to Disk

**Before you begin:**
- Drive is powered on, IEC cable connected to ZoomFloppy
- ZoomFloppy is plugged into your Mac
- A formatted disk is inserted in the drive
- The drive's activity light is off (not busy)

**Steps:**

1. Load a `.prg` file — drag it onto the drop zone or use **File → Open .prg File…**
2. Confirm the **Drive number** matches your drive (usually `8`)
3. Optionally set a custom **CBM filename**, or leave blank for automatic naming
4. Ensure **Verify after write** is checked (recommended)
5. Click **Write to Disk**

Commotransfer will:
1. Log the operation details (filename, CBM name, drive)
2. Launch `cbmcopy` in the background
3. Stream all output from `cbmcopy` to the log in real time
4. If verify is enabled: read the file back and compare checksums
5. Report success or failure in the log and status bar

**Typical write time:** A 200-block PRG file (~50 KB) takes approximately 30–90 seconds depending on the drive speed and USB timing. The 1541 is slow by design — this is normal.

> **Do not remove the disk or power off the drive during a transfer.** This may corrupt the disk directory or leave a partial file on disk. Use Cancel if you need to abort.

---

## 8. Formatting a Disk

> ⚠ **Formatting permanently erases all data on the disk.** There is no undo.

Click **Format Disk…** to open the two-step format flow.

**Step 1 — Disk name**
A dialog prompts for the disk name. This is the label stored in the disk's directory header and shown when you type `LOAD "$",8` on a C64. It can contain any characters and be up to 16 characters long. Leave it blank if you don't need a name.

**Step 2 — Disk ID**
A second dialog prompts for the disk ID. This must be **exactly two digits** (e.g. `01`, `42`, `99`). If you enter anything other than two digits the dialog will show a warning and ask again — it will not proceed until a valid ID is entered.

**Confirmation**
Before formatting begins, a confirmation dialog summarises the drive number, disk name, and disk ID and asks you to confirm. Click **Yes** to proceed or **Cancel** to abort.

**What happens during format**
Commotransfer runs:

```bash
cbmformat -d <drive> "<disk name>,<disk id>"
```

This performs a full low-level format of the disk: writing sector headers, gap bytes, and an empty directory on every track. On a 1541 drive this takes approximately **60–90 seconds** — the drive head will step across all 35 tracks. The activity light will be on throughout; this is normal.

Progress is shown in the activity log in real time. When complete, the log reports success or failure and the status bar updates.

**After formatting**
The disk is ready to use. Its directory is empty and its full capacity is available (~664 blocks free on a 1541 disk). You can now write `.prg` files to it using the Write to Disk button.

---

## 9. Verifying a Transfer

Verification works by:

1. Computing the **SHA-256** cryptographic hash of the original local `.prg` file
2. Running `cbmcopy -r` to read the file back from the Commodore disk into a temporary directory
3. Computing the SHA-256 hash of the read-back file
4. Comparing the two hashes

Both hashes are printed in the log so you can inspect them manually if needed:

```
[10:44:05] Source SHA-256 : a3f82c1d...
[10:44:09] Disk SHA-256   : a3f82c1d...
[10:44:09] ✓ Verification passed — disk contents match source file.
```

If the hashes differ:

```
[10:44:09] ✗ Verification FAILED — checksums do not match!
```

A failed verification means the data on disk does not exactly match your source file. Possible causes:

- The disk has a bad sector at the location where the file was written
- The disk is worn, dirty, or misaligned
- An IEC bus error occurred during the write
- The disk was not formatted correctly for the drive

**What to do on a failed verify:**
1. Try a different disk
2. Try the write again on the same disk (in case of a transient bus error)
3. If failures persist, clean the drive head and/or check the IEC cable connections

---

## 9. Drive Utilities

### 9.1 Detect Drives

**Drive → Detect Drives** runs `cbmctrl detect`, which queries every possible device number (8–15) on the IEC bus and logs any drives that respond.

Example output:
```
 8: 1541-II or 1571 loading from 8
```

This is useful for:
- Confirming the ZoomFloppy can see your drive before attempting a transfer
- Checking the device number a drive is configured to use
- Diagnosing "drive not found" errors

### 9.2 Reset IEC Bus

**Drive → Reset IEC Bus** sends an ATN (Attention) reset signal across the IEC bus via `cbmctrl reset`. This:

- Clears any stuck state in the drive's communication buffer
- Cancels any in-progress serial transaction
- Is equivalent to turning the drive off and on, but without losing the disk state

Use this when:
- A previous transfer failed and the drive seems unresponsive
- `cbmcopy` or `cbmctrl detect` hangs or returns an error immediately
- The drive's activity light is stuck on after an aborted operation

A reset does **not** damage the disk or erase any data.

---

## 10. CBM Filenames Explained

Commodore disk filenames have different rules from macOS filenames:

| Rule | Detail |
|---|---|
| **Case** | Always uppercase. The CBM DOS is case-insensitive but stores uppercase. |
| **Max length** | 16 characters |
| **Allowed characters** | Letters, digits, spaces, hyphens, underscores. Some special characters are valid on disk but cause issues with `cbmcopy`. |
| **File type** | `.prg` files are stored as type `PRG` (program). |

**Automatic derivation**

When you leave the CBM filename field blank, Commotransfer applies these rules to your local filename:

1. Strip the path — use only the base filename
2. Remove the `.prg` extension
3. Convert to uppercase
4. Truncate to 16 characters
5. Replace any illegal characters with underscores

Examples:

| Local filename | Derived CBM name |
|---|---|
| `mygame.prg` | `MYGAME` |
| `my-cool-demo.prg` | `MY-COOL-DEMO` |
| `super_long_filename_here.prg` | `SUPER_LONG_FILEN` |
| `game v2 (fixed).prg` | `GAME V2 _FIXED_` |

**When to set a custom name**

- If the auto-derived name conflicts with a file already on the disk
- If you want the file to load with a specific `LOAD "NAME",8,1` command on the C64
- If the auto-derived name contains characters that cause issues

---

## 11. Under the Hood

### 11.1 OpenCBM

[OpenCBM](https://opencbm.trikaliotis.net/) is an open-source library and set of command-line tools for communicating with Commodore IEC bus peripherals from modern computers. It supports a range of USB-to-IEC adapters including the ZoomFloppy.

Commotransfer uses two OpenCBM tools:

| Tool | Purpose |
|---|---|
| `cbmcopy` | Read and write files between the local filesystem and a CBM drive |
| `cbmctrl` | Low-level IEC bus control (detect, reset, etc.) |

Both tools are installed by Homebrew to `/opt/homebrew/bin/` (Apple Silicon) or `/usr/local/bin/` (Intel).

### 11.2 Writing

A write operation issues this command:

```bash
cbmcopy -w -d <drive> <filename>
```

For example, writing `game.prg` to drive 8:

```bash
cbmcopy -w -d 8 game.prg
```

`cbmcopy` runs from the file's containing directory. The file is written to the disk with a PRG file type marker. The on-disk name is derived from the local filename unless you supply a custom CBM name, in which case the argument becomes:

```bash
cbmcopy -w -d 8 "game.prg,GAME,p"
```

The `,p` suffix tells cbmcopy to use PRG type explicitly.

### 11.3 Verification

Verification is a two-phase operation:

**Phase 1 — Read back**
```bash
cbmcopy -r -d <drive> <CBM_NAME>
```
This reads the named file from the disk into a temporary directory created by the OS. The temporary directory is deleted automatically when verification completes.

**Phase 2 — Checksum comparison**
Commotransfer computes the SHA-256 hash of both the original file and the read-back file using Qt's `QCryptographicHash` class. If they match, the transfer is confirmed good.

### 11.4 The ZoomFloppy

The ZoomFloppy is a USB HID (Human Interface Device) adapter designed by [go4retro](http://www.go4retro.com/products/zoomfloppy/). It presents itself to macOS as a standard USB device — no custom kernel driver needed. OpenCBM's user-space library communicates with it via the IOKit HID framework.

The ZoomFloppy translates between USB packets and the three-wire Commodore IEC serial bus: **Clock**, **Data**, and **ATN** (Attention). It handles the timing-critical serial protocol that the C64 uses to talk to its drives.

### 11.5 Application Architecture

Commotransfer is built in **C++17** with **Qt 6** (Widgets). It follows a straightforward model:

```
main.cpp
  └─ SetupWizard        First-run wizard (QWizard)
  └─ MainWindow         Main application window (QMainWindow)
        ├─ DropZone     Custom drag-and-drop widget (QLabel subclass)
        ├─ TransferWorker  Background thread (QThread + QObject)
        │     └─ OpenCBMWrapper  Calls cbmcopy / cbmctrl via QProcess
        └─ Activity log   (QTextEdit, read-only)
```

**TransferWorker** runs in its own `QThread` so the UI remains responsive during long transfers. It communicates back to the main thread exclusively through Qt signals:
- `logLine(QString)` — a new output line to append to the log
- `progress(int)` — progress percentage (-1 for indeterminate)
- `finished(bool, QString)` — operation complete, with success flag and summary

**OpenCBMWrapper** is a pure static utility class. It locates the OpenCBM binaries by searching known Homebrew paths, constructs the correct arguments, and runs `cbmcopy` or `cbmctrl` via `QProcess`. The caller supplies the `QProcess` instance so it can be cancelled from outside.

**DropZone** is a custom `QLabel` subclass that paints its own dashed-border drop target using `QPainter` and accepts `QDropEvent` for `.prg` files. Clicking it opens a `QFileDialog`.

---

## 12. Troubleshooting

**"OpenCBM not installed — run Setup"**

OpenCBM was not found. Open **Help → Run Setup Wizard** or run in Terminal:
```bash
brew install opencbm
```
Then restart Commotransfer.

---

**Write button is greyed out**

Either no file is loaded (drop a `.prg` file first) or another operation is already running. Wait for the current operation to finish or click Cancel.

---

**`cbmctrl detect` shows nothing / "No drives detected"**

Check the following in order:
1. ZoomFloppy is plugged into a USB port (try a different port or hub)
2. IEC cable is firmly connected at both ends
3. Drive is powered on (power supply connected and switched on)
4. Drive device number matches the spinner in Commotransfer (default 8)
5. Try **Drive → Reset IEC Bus**, then **Detect Drives** again

---

**Transfer fails with "ERROR: cbmcopy not found"**

OpenCBM's tools are not on the search path. Commotransfer looks in `/opt/homebrew/bin` and `/usr/local/bin`. If you installed OpenCBM another way, ensure `cbmcopy` is in one of those locations or open a bug report.

---

**Write completes but Verify fails**

The data on disk does not match the source file:
- Try writing again — a single bit error on the bus can cause this
- Try a different disk — the media may have a bad sector
- If it fails consistently, clean the drive head (use a proper 5¼" head cleaning disk)
- Check the IEC cable for loose connectors or damage

---

**Drive activity light stays on / drive is stuck**

Use **Drive → Reset IEC Bus**. If that does not help, power-cycle the drive (switch it off and on). Do not remove the disk while the activity light is on.

---

**Build fails: "Qt not found"**

Make sure Qt is installed:
```bash
brew install qt
```
If you installed Qt another way, pass the path manually:
```bash
cd build
cmake .. -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
```

---

**App crashes on launch on Apple Silicon**

Ensure you built with the Homebrew Qt at `/opt/homebrew/opt/qt` (not an Intel Qt). Run `build.sh --clean` to wipe and rebuild.

---

**The disk fills up**

A standard 1541-formatted disk holds 664 blocks free (approximately 170 KB). Each block is 254 bytes. If `cbmcopy` reports a "disk full" error, use a fresh disk or delete existing files from the disk first using a Commodore 64 or a disk management tool.

---

## 13. Glossary

**ATN (Attention)**
One of the three signal lines on the Commodore IEC bus. The computer asserts ATN to get the attention of all devices before issuing a command.

**cbmcopy**
An OpenCBM command-line tool for transferring files between a modern computer and a Commodore IEC drive.

**cbmctrl**
An OpenCBM command-line tool for low-level IEC bus control: detecting devices, sending resets, reading the drive status, etc.

**CBM**
Commodore Business Machines — the company that made the Commodore 64, the 1541 drive, and the IEC bus standard used by Commotransfer.

**Device number**
The address a Commodore peripheral uses on the IEC bus. Drives typically use numbers 8–11; the printer was device 4.

**D64**
A disk image format representing an entire 1541 floppy as a single file. Commotransfer does not work with D64 images — it transfers individual `.prg` files.

**IEC bus**
The serial bus used by Commodore 8-bit computers to communicate with peripherals. Uses three active signal lines (ATN, Clock, Data) over a 6-pin DIN connector.

**OpenCBM**
An open-source library and toolset for communicating with Commodore IEC peripherals from modern operating systems via USB adapters.

**PRG**
The native Commodore 64 program file format. A PRG file begins with a two-byte load address, followed by raw 6502 machine code or BASIC tokens. The `.prg` extension is used on modern systems to identify these files.

**SHA-256**
A cryptographic hash function that produces a 256-bit (64 hex character) digest of a file's contents. Two identical files always produce the same digest; even a single changed byte produces a completely different one. Used by Commotransfer to verify that the written data matches the source exactly.

**ZoomFloppy**
A USB adapter by go4retro that connects a modern computer's USB port to the Commodore IEC serial bus, allowing OpenCBM to communicate with real Commodore drives.

**1541**
The most common Commodore floppy disk drive. Single-sided, double-density, 5¼" disks, approximately 170 KB capacity. Operates as an IEC device, typically at device number 8.

**1571**
An enhanced Commodore floppy drive capable of reading both sides of a disk (double-sided), with roughly double the capacity of a 1541. Fully compatible with Commotransfer.
