#pragma once

#include <QWizard>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProcess>

// ── Page IDs ──────────────────────────────────────────────────────────────────
enum WizardPageId {
    Page_Welcome  = 0,
    Page_Homebrew = 1,
    Page_Install  = 2,
    Page_Connect  = 3,
    Page_Done     = 4,
};

// ── WelcomePage ───────────────────────────────────────────────────────────────
class WelcomePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit WelcomePage(QWidget *parent = nullptr);
    int nextId() const override;
};

// ── HomebrewPage ──────────────────────────────────────────────────────────────
class HomebrewPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit HomebrewPage(QWidget *parent = nullptr);
    void initializePage() override;
    bool isComplete() const override;
    int  nextId() const override;

private:
    QLabel      *m_statusLabel;
    QPushButton *m_openWebBtn;
    bool         m_brewFound = false;
};

// ── InstallPage ───────────────────────────────────────────────────────────────
class InstallPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit InstallPage(QWidget *parent = nullptr);
    void initializePage() override;
    bool isComplete() const override;
    int  nextId() const override;

private slots:
    void startInstall();
    void onProcessOutput();
    void onProcessFinished(int code, QProcess::ExitStatus status);

private:
    QPlainTextEdit *m_log;
    QPushButton    *m_installBtn;
    QProcess       *m_process = nullptr;
    bool            m_done    = false;
    bool            m_success = false;
};

// ── ConnectPage ───────────────────────────────────────────────────────────────
class ConnectPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit ConnectPage(QWidget *parent = nullptr);
    void initializePage() override;
    bool isComplete() const override;
    int  nextId() const override;

private slots:
    void detectDrives();

private:
    QLabel         *m_statusLabel;
    QPlainTextEdit *m_log;
    QPushButton    *m_detectBtn;
    bool            m_detected = false;
};

// ── DonePage ──────────────────────────────────────────────────────────────────
class DonePage : public QWizardPage
{
    Q_OBJECT
public:
    explicit DonePage(QWidget *parent = nullptr);
    int nextId() const override { return -1; }
};

// ── SetupWizard ───────────────────────────────────────────────────────────────
class SetupWizard : public QWizard
{
    Q_OBJECT
public:
    explicit SetupWizard(QWidget *parent = nullptr);
};
