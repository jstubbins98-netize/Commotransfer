#include "MainWindow.h"
#include "DropZone.h"
#include "OpenCBMWrapper.h"
#include "SetupWizard.h"
#include "TransferWorker.h"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Commotransfer");
    setMinimumSize(740, 560);
    resize(860, 640);

    buildMenuBar();
    buildToolBar();
    buildUI();
    updateButtons();
}

MainWindow::~MainWindow()
{
    if (m_worker) m_worker->requestCancel();
    if (m_thread) { m_thread->quit(); m_thread->wait(3000); }
}

// ── UI construction ───────────────────────────────────────────────────────────

void MainWindow::buildUI()
{
    // ── Central splitter ────────────────────────────────────────────────────
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(6);
    setCentralWidget(splitter);

    // ── Left panel ──────────────────────────────────────────────────────────
    auto *leftWidget = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(16, 16, 8, 16);
    leftLayout->setSpacing(14);

    // Title
    auto *titleLabel = new QLabel("Commotransfer");
    {
        QFont f = titleLabel->font();
        f.setPointSize(18);
        f.setBold(true);
        titleLabel->setFont(f);
    }
    auto *subLabel = new QLabel("Write .prg files to real Commodore floppy disks");
    {
        QFont f = subLabel->font();
        f.setPointSize(11);
        subLabel->setFont(f);
        subLabel->setStyleSheet("color: gray;");
    }
    leftLayout->addWidget(titleLabel);
    leftLayout->addWidget(subLabel);
    leftLayout->addSpacing(4);

    // Drop zone
    m_dropZone = new DropZone;
    connect(m_dropZone, &DropZone::fileSelected, this, &MainWindow::onFileSelected);
    leftLayout->addWidget(m_dropZone);

    // ── Settings group ──────────────────────────────────────────────────────
    auto *settingsGroup = new QGroupBox("Transfer Settings");
    auto *formLayout    = new QFormLayout(settingsGroup);
    formLayout->setLabelAlignment(Qt::AlignRight);
    formLayout->setSpacing(10);
    formLayout->setContentsMargins(12, 12, 12, 12);

    m_driveSpin = new QSpinBox;
    m_driveSpin->setRange(8, 11);
    m_driveSpin->setValue(8);
    m_driveSpin->setFixedWidth(80);
    m_driveSpin->setToolTip("CBM device number (1541/1571 drives are usually 8)");
    formLayout->addRow("Drive number:", m_driveSpin);

    m_cbmNameEdit = new QLineEdit;
    m_cbmNameEdit->setMaxLength(16);
    m_cbmNameEdit->setPlaceholderText("Auto-derived from filename");
    m_cbmNameEdit->setToolTip(
        "Name used on the CBM disk (max 16 chars, uppercase).\n"
        "Leave blank to derive from the local filename."
    );
    formLayout->addRow("CBM filename:", m_cbmNameEdit);

    m_verifyCheck = new QCheckBox("Verify after write");
    m_verifyCheck->setChecked(true);
    m_verifyCheck->setToolTip(
        "After writing, read the file back from the disk and compare\n"
        "checksums to confirm the transfer was successful."
    );
    formLayout->addRow("", m_verifyCheck);

    leftLayout->addWidget(settingsGroup);

    // ── Action buttons ──────────────────────────────────────────────────────
    m_writeBtn = new QPushButton("Write to Disk");
    m_writeBtn->setFixedHeight(40);
    m_writeBtn->setToolTip("Write the selected .prg file to the CBM drive");
    m_writeBtn->setEnabled(false);
    {
        QFont f = m_writeBtn->font();
        f.setBold(true);
        m_writeBtn->setFont(f);
    }
    connect(m_writeBtn, &QPushButton::clicked, this, &MainWindow::onWriteClicked);

    m_verifyBtn = new QPushButton("Verify Only");
    m_verifyBtn->setFixedHeight(34);
    m_verifyBtn->setToolTip("Read the file back from disk and compare checksums");
    m_verifyBtn->setEnabled(false);
    connect(m_verifyBtn, &QPushButton::clicked, this, &MainWindow::onVerifyClicked);

    m_formatBtn = new QPushButton("Format Disk…");
    m_formatBtn->setFixedHeight(34);
    m_formatBtn->setToolTip("Erase and format the disk in the selected drive");
    connect(m_formatBtn, &QPushButton::clicked, this, &MainWindow::onFormatClicked);

    m_cancelBtn = new QPushButton("Cancel");
    m_cancelBtn->setFixedHeight(34);
    m_cancelBtn->setVisible(false);
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        if (m_worker) m_worker->requestCancel();
        m_cancelBtn->setEnabled(false);
    });

    auto *btnLayout = new QVBoxLayout;
    btnLayout->setSpacing(8);
    btnLayout->addWidget(m_writeBtn);

    auto *secondRowLayout = new QHBoxLayout;
    secondRowLayout->addWidget(m_verifyBtn);
    secondRowLayout->addWidget(m_formatBtn);
    btnLayout->addLayout(secondRowLayout);

    auto *cancelLayout = new QHBoxLayout;
    cancelLayout->addWidget(m_cancelBtn);
    btnLayout->addLayout(cancelLayout);

    leftLayout->addLayout(btnLayout);

    // Progress bar
    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setTextVisible(false);
    leftLayout->addWidget(m_progressBar);

    leftLayout->addStretch();
    splitter->addWidget(leftWidget);

    // ── Right panel — log ────────────────────────────────────────────────────
    auto *rightWidget = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(8, 16, 16, 16);
    rightLayout->setSpacing(6);

    auto *logHeader = new QHBoxLayout;
    auto *logLabel  = new QLabel("Activity Log");
    {
        QFont f = logLabel->font();
        f.setBold(true);
        logLabel->setFont(f);
    }
    auto *clearBtn = new QPushButton("Clear");
    clearBtn->setFixedSize(60, 24);
    clearBtn->setFlat(true);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearLog);

    logHeader->addWidget(logLabel);
    logHeader->addStretch();
    logHeader->addWidget(clearBtn);
    rightLayout->addLayout(logHeader);

    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    QFont mono("Menlo");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(11);
    m_log->setFont(mono);
    m_log->setLineWrapMode(QTextEdit::NoWrap);
    rightLayout->addWidget(m_log);

    splitter->addWidget(rightWidget);
    splitter->setSizes({340, 500});

    // ── Status bar ───────────────────────────────────────────────────────────
    m_statusLabel = new QLabel(
        OpenCBMWrapper::isInstalled() ? "OpenCBM ready" : "OpenCBM not installed — run Setup"
    );
    statusBar()->addWidget(m_statusLabel);

    // Initial log greeting
    appendLog("Commotransfer ready.");
    appendLog(OpenCBMWrapper::isInstalled()
        ? "OpenCBM tools found: " + OpenCBMWrapper::findBinary("cbmcopy")
        : "⚠ OpenCBM not found. Use Help → Run Setup Wizard to install it."
    );
}

void MainWindow::buildMenuBar()
{
    // File
    auto *fileMenu = menuBar()->addMenu("File");
    m_openAction = fileMenu->addAction("Open .prg File…");
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction("Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    // Drive
    auto *driveMenu = menuBar()->addMenu("Drive");
    m_detectAction = driveMenu->addAction("Detect Drives", this, &MainWindow::onDetectClicked);
    m_resetAction  = driveMenu->addAction("Reset IEC Bus", this, &MainWindow::onResetClicked);

    // Log
    auto *logMenu = menuBar()->addMenu("Log");
    m_clearAction = logMenu->addAction("Clear Log", this, &MainWindow::onClearLog);

    // Help
    auto *helpMenu = menuBar()->addMenu("Help");
    m_setupAction = helpMenu->addAction("Run Setup Wizard…", this, &MainWindow::onRunSetup);
}

void MainWindow::buildToolBar()
{
    auto *tb = addToolBar("Main");
    tb->setMovable(false);
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    tb->addAction(QIcon::fromTheme("document-open"), "Open File", this, &MainWindow::onOpenFile);
    tb->addSeparator();
    tb->addAction(QIcon::fromTheme("media-playback-start"), "Detect Drives", this, &MainWindow::onDetectClicked);
    tb->addAction(QIcon::fromTheme("view-refresh"), "Reset Bus", this, &MainWindow::onResetClicked);
}

// ── Slot implementations ──────────────────────────────────────────────────────

void MainWindow::onFileSelected(const QString &path)
{
    // Auto-fill CBM name if the field is empty
    if (m_cbmNameEdit->text().isEmpty()) {
        m_cbmNameEdit->setPlaceholderText(OpenCBMWrapper::deriveCBMName(path));
    }
    appendLog("File selected: " + path);
    updateButtons();
}

void MainWindow::onWriteClicked()
{
    if (m_dropZone->currentFile().isEmpty()) return;

    QString cbmName = m_cbmNameEdit->text().trimmed().toUpper();
    if (cbmName.isEmpty()) cbmName = OpenCBMWrapper::deriveCBMName(m_dropZone->currentFile());

    auto *worker = new TransferWorker;
    worker->setWriteParams(
        m_dropZone->currentFile(),
        m_driveSpin->value(),
        cbmName,
        m_verifyCheck->isChecked()
    );
    startWorker(worker);
}

void MainWindow::onVerifyClicked()
{
    if (m_dropZone->currentFile().isEmpty()) return;

    QString cbmName = m_cbmNameEdit->text().trimmed().toUpper();
    if (cbmName.isEmpty()) cbmName = OpenCBMWrapper::deriveCBMName(m_dropZone->currentFile());

    auto *worker = new TransferWorker;
    worker->setVerifyParams(
        m_dropZone->currentFile(),
        m_driveSpin->value(),
        cbmName
    );
    startWorker(worker);
}

void MainWindow::onFormatClicked()
{
    // ── Dialog 1: disk name ───────────────────────────────────────────────────
    bool ok = false;
    QString diskName = QInputDialog::getText(
        this,
        "Format Disk — Step 1 of 2",
        "Disk name (up to 16 characters):",
        QLineEdit::Normal,
        QString(),
        &ok
    );
    if (!ok) return;

    diskName = diskName.left(16);          // enforce maximum length

    // ── Dialog 2: disk ID ─────────────────────────────────────────────────────
    QString diskId;
    while (true) {
        diskId = QInputDialog::getText(
            this,
            "Format Disk — Step 2 of 2",
            "Disk ID (exactly 2 digits, e.g. 01):",
            QLineEdit::Normal,
            diskId,
            &ok
        );
        if (!ok) return;

        // Validate: must be exactly 2 decimal digits
        static const QRegularExpression re("^\\d{2}$");
        if (re.match(diskId).hasMatch()) break;

        QMessageBox::warning(
            this,
            "Invalid Disk ID",
            "The disk ID must be exactly two digits (e.g. 01, 42, 99).\nPlease try again."
        );
    }

    // ── Confirmation ──────────────────────────────────────────────────────────
    auto confirm = QMessageBox::warning(
        this,
        "Format Disk",
        QString(
            "This will <b>permanently erase</b> all data on the disk in drive %1.<br><br>"
            "Disk name: <b>%2</b><br>"
            "Disk ID: <b>%3</b><br><br>"
            "Are you sure?"
        ).arg(m_driveSpin->value()).arg(diskName.isEmpty() ? "(none)" : diskName).arg(diskId),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel
    );
    if (confirm != QMessageBox::Yes) return;

    // ── Start worker ──────────────────────────────────────────────────────────
    auto *worker = new TransferWorker;
    worker->setFormatParams(diskName, diskId, m_driveSpin->value());
    startWorker(worker);
}

void MainWindow::onResetClicked()
{
    auto *worker = new TransferWorker;
    worker->setMode(TransferWorker::Mode::Reset);
    startWorker(worker);
}

void MainWindow::onDetectClicked()
{
    auto *worker = new TransferWorker;
    worker->setMode(TransferWorker::Mode::Detect);
    startWorker(worker);
}

void MainWindow::onClearLog()
{
    m_log->clear();
}

void MainWindow::onOpenFile()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        "Select a .prg file",
        QDir::homePath(),
        "Commodore PRG files (*.prg);;All files (*)"
    );
    if (!path.isEmpty()) m_dropZone->setFile(path);
}

void MainWindow::onRunSetup()
{
    SetupWizard wizard(this);
    wizard.exec();
    m_statusLabel->setText(
        OpenCBMWrapper::isInstalled() ? "OpenCBM ready" : "OpenCBM not installed"
    );
    updateButtons();
}

// ── Worker management ─────────────────────────────────────────────────────────

void MainWindow::startWorker(TransferWorker *worker)
{
    if (m_busy) return;

    m_worker = worker;
    m_thread = new QThread(this);
    worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started,  worker, &TransferWorker::run);
    connect(worker,   &TransferWorker::logLine,  this, &MainWindow::onLogLine);
    connect(worker,   &TransferWorker::progress, this, &MainWindow::onProgress);
    connect(worker,   &TransferWorker::finished, this, &MainWindow::onFinished);
    connect(worker,   &TransferWorker::finished, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, worker, &TransferWorker::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);

    setTransferActive(true);
    m_thread->start();
}

void MainWindow::onLogLine(const QString &line)
{
    appendLog(line);
}

void MainWindow::onProgress(int percent)
{
    if (percent < 0) {
        m_progressBar->setRange(0, 0); // indeterminate
    } else {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(percent);
    }
}

void MainWindow::onFinished(bool success, const QString &message)
{
    m_thread = nullptr;
    m_worker = nullptr;
    setTransferActive(false);

    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(success ? 100 : 0);
    m_statusLabel->setText(message);

    appendLog("");
    appendLog(success ? "✓ " + message : "✗ " + message, !success);
    appendLog("");
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void MainWindow::setTransferActive(bool active)
{
    m_busy = active;
    m_progressBar->setVisible(active);
    m_cancelBtn->setVisible(active);
    m_cancelBtn->setEnabled(active);
    m_writeBtn->setEnabled(!active && !m_dropZone->currentFile().isEmpty());
    m_verifyBtn->setEnabled(!active && !m_dropZone->currentFile().isEmpty());
    m_formatBtn->setEnabled(!active);
    m_detectAction->setEnabled(!active);
    m_resetAction->setEnabled(!active);
}

void MainWindow::updateButtons()
{
    bool hasFile = !m_dropZone->currentFile().isEmpty();
    m_writeBtn->setEnabled(!m_busy && hasFile);
    m_verifyBtn->setEnabled(!m_busy && hasFile);
}

void MainWindow::appendLog(const QString &line, bool error)
{
    if (error) {
        m_log->setTextColor(QColor(220, 60, 60));
    } else if (line.startsWith("✓")) {
        m_log->setTextColor(QColor(50, 180, 80));
    } else if (line.startsWith("✗") || line.startsWith("⚠")) {
        m_log->setTextColor(QColor(220, 140, 0));
    } else {
        // Use palette foreground for normal lines
        m_log->setTextColor(m_log->palette().text().color());
    }

    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_log->append("[" + ts + "] " + line);

    // Auto-scroll
    QScrollBar *sb = m_log->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_busy) {
        auto reply = QMessageBox::question(
            this, "Transfer in progress",
            "A transfer is currently running. Cancel it and quit?",
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply != QMessageBox::Yes) { event->ignore(); return; }
        if (m_worker) m_worker->requestCancel();
        if (m_thread) { m_thread->quit(); m_thread->wait(3000); }
    }
    event->accept();
}
