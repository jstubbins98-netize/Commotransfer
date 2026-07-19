#include "SetupWizard.h"
#include "OpenCBMWrapper.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QLabel *makeHeading(const QString &text, int ptSize = 16)
{
    auto *lbl = new QLabel(text);
    QFont f = lbl->font();
    f.setPointSize(ptSize);
    f.setBold(true);
    lbl->setFont(f);
    lbl->setWordWrap(true);
    return lbl;
}

static QLabel *makeBody(const QString &text)
{
    auto *lbl = new QLabel(text);
    lbl->setWordWrap(true);
    lbl->setTextFormat(Qt::RichText);
    return lbl;
}

// ── WelcomePage ───────────────────────────────────────────────────────────────

WelcomePage::WelcomePage(QWidget *parent) : QWizardPage(parent)
{
    setTitle("Welcome to Commotransfer");

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(16);

    layout->addWidget(makeHeading("One-time setup", 15));
    layout->addWidget(makeBody(
        "This wizard will install and configure <b>OpenCBM</b> so Commotransfer "
        "can write .prg files to real Commodore 5¼\" floppy disks via your "
        "<b>ZoomFloppy USB adapter</b>.<br><br>"
        "The setup will:<ul>"
        "<li>Check that <b>Homebrew</b> is installed</li>"
        "<li>Install <b>opencbm</b> via Homebrew (no password required)</li>"
        "<li>Detect your ZoomFloppy adapter and connected drives</li>"
        "</ul>"
    ));
    layout->addStretch();
}

int WelcomePage::nextId() const
{
    return OpenCBMWrapper::isInstalled() ? Page_Connect : Page_Homebrew;
}

// ── HomebrewPage ──────────────────────────────────────────────────────────────

HomebrewPage::HomebrewPage(QWidget *parent) : QWizardPage(parent)
{
    setTitle("Homebrew");
    setSubTitle("Homebrew is required to install OpenCBM.");

    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);

    m_openWebBtn = new QPushButton("Open Homebrew Website…");
    m_openWebBtn->setVisible(false);
    connect(m_openWebBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl("https://brew.sh"));
    });

    auto *note = makeBody(
        "After installing Homebrew, <b>quit and reopen</b> this wizard — "
        "it will detect Homebrew automatically."
    );
    note->setVisible(false);
    connect(m_openWebBtn, &QPushButton::clicked, note, &QLabel::show);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_statusLabel);
    layout->addWidget(m_openWebBtn);
    layout->addWidget(note);
    layout->addStretch();
}

void HomebrewPage::initializePage()
{
    m_brewFound = OpenCBMWrapper::isHomebrewInstalled();
    if (m_brewFound) {
        m_statusLabel->setText("✅  Homebrew is installed. Click <b>Next</b> to continue.");
        m_openWebBtn->setVisible(false);
    } else {
        m_statusLabel->setText(
            "❌  Homebrew was not found on this system.<br><br>"
            "Please install Homebrew first, then return to this wizard.<br><br>"
            "You can install it by running this in Terminal:<br>"
            "<tt>/bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/"
            "Homebrew/install/HEAD/install.sh)\"</tt>"
        );
        m_openWebBtn->setVisible(true);
    }
    emit completeChanged();
}

bool HomebrewPage::isComplete() const { return m_brewFound; }
int  HomebrewPage::nextId()     const { return Page_Install; }

// ── InstallPage ───────────────────────────────────────────────────────────────

InstallPage::InstallPage(QWidget *parent) : QWizardPage(parent)
{
    setTitle("Install OpenCBM");
    setSubTitle("Click the button below to install opencbm via Homebrew.");

    m_installBtn = new QPushButton("Install OpenCBM");
    m_installBtn->setFixedHeight(36);
    connect(m_installBtn, &QPushButton::clicked, this, &InstallPage::startInstall);

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMinimumHeight(180);
    QFont mono("Menlo");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(11);
    m_log->setFont(mono);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_installBtn);
    layout->addWidget(m_log);
}

void InstallPage::initializePage()
{
    if (OpenCBMWrapper::isInstalled()) {
        m_log->appendPlainText("OpenCBM is already installed — nothing to do.");
        m_done    = true;
        m_success = true;
        m_installBtn->setEnabled(false);
        emit completeChanged();
    }
}

void InstallPage::startInstall()
{
    m_installBtn->setEnabled(false);
    m_log->clear();
    m_log->appendPlainText("Running: brew install opencbm");
    m_log->appendPlainText("──────────────────────────────────────────");

    QString brewBin = OpenCBMWrapper::findBinary("brew");
    if (brewBin.isEmpty()) brewBin = "/opt/homebrew/bin/brew";

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    // Pass a clean environment so brew output is not buffered
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("HOMEBREW_NO_AUTO_UPDATE", "1"); // skip auto-update for speed
    m_process->setProcessEnvironment(env);

    connect(m_process, &QProcess::readyRead,
            this, &InstallPage::onProcessOutput);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &InstallPage::onProcessFinished);

    m_process->start(brewBin, {"install", "opencbm"});
}

void InstallPage::onProcessOutput()
{
    while (m_process->canReadLine()) {
        m_log->appendPlainText(
            QString::fromLocal8Bit(m_process->readLine()).trimmed()
        );
    }
}

void InstallPage::onProcessFinished(int code, QProcess::ExitStatus)
{
    m_log->appendPlainText(
        QString::fromLocal8Bit(m_process->readAll()).trimmed()
    );
    m_log->appendPlainText("──────────────────────────────────────────");

    m_done    = true;
    m_success = (code == 0) && OpenCBMWrapper::isInstalled();

    if (m_success) {
        m_log->appendPlainText("✅  OpenCBM installed successfully.");
    } else {
        m_log->appendPlainText(
            "❌  Installation failed or OpenCBM tools not found after install.\n"
            "    Try running in Terminal:\n"
            "        brew install opencbm\n"
            "    Then restart Commotransfer."
        );
    }
    emit completeChanged();
    m_process->deleteLater();
    m_process = nullptr;
}

bool InstallPage::isComplete() const { return m_done && m_success; }
int  InstallPage::nextId()     const { return Page_Connect; }

// ── ConnectPage ───────────────────────────────────────────────────────────────

ConnectPage::ConnectPage(QWidget *parent) : QWizardPage(parent)
{
    setTitle("Connect ZoomFloppy");
    setSubTitle("Connect your ZoomFloppy adapter and power on your drive.");

    m_statusLabel = new QLabel(
        "Plug in your ZoomFloppy USB adapter, connect it to your Commodore drive "
        "(e.g. 1541), power the drive on, then click Detect."
    );
    m_statusLabel->setWordWrap(true);

    m_detectBtn = new QPushButton("Detect Drives");
    m_detectBtn->setFixedHeight(36);
    connect(m_detectBtn, &QPushButton::clicked, this, &ConnectPage::detectDrives);

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMinimumHeight(120);
    QFont mono("Menlo");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(11);
    m_log->setFont(mono);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_statusLabel);
    layout->addSpacing(8);
    layout->addWidget(m_detectBtn);
    layout->addWidget(m_log);
}

void ConnectPage::initializePage()
{
    m_detected = false;
    emit completeChanged();
}

void ConnectPage::detectDrives()
{
    m_detectBtn->setEnabled(false);
    m_log->clear();
    m_log->appendPlainText("Scanning IEC bus…");

    QString err;
    QString out = OpenCBMWrapper::detectDrives(&err);

    if (!out.isEmpty()) {
        m_log->appendPlainText(out);
        m_log->appendPlainText("✅  Drive(s) detected. Click Finish to continue.");
        m_detected = true;
    } else {
        m_log->appendPlainText(
            "No drives found. Check:\n"
            "  • ZoomFloppy is connected (USB)\n"
            "  • Drive is powered on and cable connected\n"
            "  • Drive is set to device number 8–11\n\n"
            + (err.isEmpty() ? "" : "Error: " + err + "\n\n") +
            "You can still continue and detect later from the main window."
        );
        m_detected = true; // allow skipping
    }

    emit completeChanged();
    m_detectBtn->setEnabled(true);
}

bool ConnectPage::isComplete() const { return m_detected; }
int  ConnectPage::nextId()     const { return Page_Done; }

// ── DonePage ──────────────────────────────────────────────────────────────────

DonePage::DonePage(QWidget *parent) : QWizardPage(parent)
{
    setTitle("Setup Complete");

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(makeHeading("You're ready to go!", 15));
    layout->addWidget(makeBody(
        "Commotransfer is configured. You can now:<br><br>"
        "<b>1.</b> Drop a .prg file onto the main window<br>"
        "<b>2.</b> Select the drive number (usually 8)<br>"
        "<b>3.</b> Click <b>Write to Disk</b><br><br>"
        "Enable <i>Verify after write</i> to have Commotransfer read the "
        "file back and confirm the data matches."
    ));
    layout->addStretch();
}

// ── SetupWizard ───────────────────────────────────────────────────────────────

SetupWizard::SetupWizard(QWidget *parent) : QWizard(parent)
{
    setWindowTitle("Commotransfer Setup");
    setWizardStyle(QWizard::ModernStyle);
    setFixedSize(600, 440);

    setPage(Page_Welcome,  new WelcomePage(this));
    setPage(Page_Homebrew, new HomebrewPage(this));
    setPage(Page_Install,  new InstallPage(this));
    setPage(Page_Connect,  new ConnectPage(this));
    setPage(Page_Done,     new DonePage(this));

    setStartId(Page_Welcome);

    setButtonText(QWizard::FinishButton, "Open Commotransfer");
    setButtonText(QWizard::CancelButton, "Quit");
    setOption(QWizard::NoCancelButton, false);
}
