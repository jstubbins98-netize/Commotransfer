# Commotransfer

Write `.prg` files from your Mac to real Commodore 5¼" floppy disks via a [ZoomFloppy](http://www.go4retro.com/products/zoomfloppy/) USB adapter and OpenCBM.

---

## Features

- **Drag & drop** — drop any `.prg` file onto the window, or use File → Open
- **One-click write** — writes the file to a connected 1541/1571 drive
- **Verify after write** — reads the file back and compares SHA-256 checksums
- **IEC bus detect / reset** — scan for attached drives, reset the bus
- **First-run setup wizard** — installs and configures OpenCBM via MacPorts automatically
- **Dark mode** — follows macOS appearance automatically

---

## Requirements

| Requirement | Notes |
|---|---|
| macOS 12 Monterey or later | Intel or Apple Silicon |
| [Homebrew](https://brew.sh) | For installing OpenCBM |
| Qt 6 (or Qt 5.15) | Install via Homebrew |
| ZoomFloppy USB adapter | [go4retro.com](http://www.go4retro.com/products/zoomfloppy/) |
| Commodore 1541 / 1571 / compatible drive | Set to device number 8–11 |

---

## Build

### 1 — Install dependencies

**Option A: MacPorts**

```bash
sudo port install qt6 cmake
```

**Option B: Homebrew**

```bash
brew install qt cmake
```

### 2 — Install OpenCBM

The setup wizard inside the app handles this automatically. Or run manually:

```bash
sudo port install opencbm
```

### 3 — Configure & build

```bash
cd Commotransfer
mkdir build && cd build

# MacPorts Qt6
cmake .. -DCMAKE_PREFIX_PATH=/opt/local/libexec/qt6

# Homebrew Qt6 (Apple Silicon)
# cmake .. -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt

cmake --build . -j$(sysctl -n hw.logicalcpu)
```

### 4 — Run

```bash
open Commotransfer.app
# or
./Commotransfer
```

On the first launch the setup wizard will run automatically.

---

## Usage

1. **Connect** — plug in ZoomFloppy, connect to your drive via IEC cable, power the drive on
2. **Drop file** — drag a `.prg` file onto the window, or use File → Open
3. **Set drive number** — typically `8` for the first drive
4. **CBM filename** — leave blank to auto-derive from the local filename (uppercase, max 16 chars)
5. **Verify after write** — leave checked (recommended)
6. **Click Write to Disk**

The activity log on the right shows all `cbmcopy` / `cbmctrl` output in real time.

### Verify only

Click **Verify Only** to read the file back from the disk and compare checksums without writing again.

### Bus reset

Use **Drive → Reset IEC Bus** (`cbmctrl reset`) if the drive seems stuck or unresponsive.

### Detect drives

Use **Drive → Detect Drives** (`cbmctrl detect`) to confirm the ZoomFloppy can see your drives.

---

## How it works

Commotransfer is a Qt C++ GUI frontend for the [OpenCBM](https://opencbm.trikaliotis.net/) command-line tools:

| OpenCBM command | What Commotransfer does |
|---|---|
| `cbmcopy -w -d 8 myfile.prg` | Writes `myfile.prg` to drive 8 |
| `cbmcopy -r -d 8 MYFILE` | Reads `MYFILE` back from drive 8 |
| `cbmctrl detect` | Scans the IEC bus for drives |
| `cbmctrl reset` | Resets the IEC bus |

Verification works by computing the SHA-256 digest of the original `.prg` file, reading the written file back from the disk into a temp directory, computing its digest, and comparing the two.

---

## Troubleshooting

**"OpenCBM not found"**  
Run the setup wizard: Help → Run Setup Wizard, or install manually:
```bash
sudo port install opencbm
```

**"No drives detected"**  
- Confirm the ZoomFloppy is plugged in to a USB port
- Confirm the IEC cable is connected
- Confirm the drive is powered on
- Confirm the drive's device number matches the spinner in the app (default 8)

**Drive is stuck / making noise**  
Use Drive → Reset IEC Bus to send a reset signal.

**Build fails — Qt not found**  
Pass `-DCMAKE_PREFIX_PATH` pointing to your Qt installation:
```bash
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt)   # Homebrew
cmake .. -DCMAKE_PREFIX_PATH=/opt/local/libexec/qt6 # MacPorts
```

---

## Project structure

```
Commotransfer/
├── CMakeLists.txt
├── README.md
├── scripts/
│   └── setup_opencbm.sh     # Manual setup helper
└── src/
    ├── main.cpp              # Entry point, first-run wizard logic
    ├── MainWindow.h/cpp      # Main application window
    ├── SetupWizard.h/cpp     # First-run setup wizard (MacPorts + OpenCBM)
    ├── OpenCBMWrapper.h/cpp  # Wrapper around cbmcopy / cbmctrl
    ├── TransferWorker.h/cpp  # Background QThread for write/verify/detect
    └── DropZone.h/cpp        # Drag-and-drop file selection widget
```

---

## License

MIT
