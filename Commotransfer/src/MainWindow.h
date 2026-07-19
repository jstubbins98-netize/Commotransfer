#pragma once

#include <QMainWindow>
#include <QThread>
#include <QString>

class DropZone;
class TransferWorker;
class QSpinBox;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QTextEdit;
class QProgressBar;
class QLabel;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onFileSelected(const QString &path);
    void onWriteClicked();
    void onVerifyClicked();
    void onResetClicked();
    void onDetectClicked();
    void onFormatClicked();
    void onClearLog();
    void onOpenFile();
    void onRunSetup();

    // Worker signals
    void onLogLine(const QString &line);
    void onProgress(int percent);
    void onFinished(bool success, const QString &message);

private:
    void buildUI();
    void buildMenuBar();
    void buildToolBar();
    void updateButtons();
    void setTransferActive(bool active);
    void startWorker(class TransferWorker *worker);
    void appendLog(const QString &line, bool error = false);

    // ── Widgets ────────────────────────────────────────────────────────────
    DropZone      *m_dropZone      = nullptr;
    QSpinBox      *m_driveSpin     = nullptr;
    QLineEdit     *m_cbmNameEdit   = nullptr;
    QCheckBox     *m_verifyCheck   = nullptr;

    QPushButton   *m_writeBtn      = nullptr;
    QPushButton   *m_verifyBtn     = nullptr;
    QPushButton   *m_formatBtn     = nullptr;
    QPushButton   *m_cancelBtn     = nullptr;

    QTextEdit     *m_log           = nullptr;
    QProgressBar  *m_progressBar   = nullptr;
    QLabel        *m_statusLabel   = nullptr;

    // ── Actions ────────────────────────────────────────────────────────────
    QAction *m_openAction   = nullptr;
    QAction *m_detectAction = nullptr;
    QAction *m_resetAction  = nullptr;
    QAction *m_setupAction  = nullptr;
    QAction *m_clearAction  = nullptr;

    // ── Worker / thread ────────────────────────────────────────────────────
    QThread         *m_thread = nullptr;
    TransferWorker  *m_worker = nullptr;
    bool             m_busy   = false;
};
