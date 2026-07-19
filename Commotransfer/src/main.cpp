#include <QApplication>
#include <QSettings>

#include "MainWindow.h"
#include "OpenCBMWrapper.h"
#include "SetupWizard.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("Commotransfer");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Commotransfer");
    app.setOrganizationDomain("commotransfer.app");

#ifdef Q_OS_MACOS
    // Native macOS menu bar, no icons in menus
    app.setAttribute(Qt::AA_DontShowIconsInMenus, true);
    // Respect macOS dark / light mode — Qt + AppKit do this automatically
    // when the app is built with a modern SDK, but we also honour it via
    // the style sheet below when the user hasn't chosen a specific theme.
#endif

    // ── First-run setup ───────────────────────────────────────────────────────
    QSettings settings;
    bool setupDone = settings.value("setup/completed", false).toBool();

    // Show the setup wizard if:
    //  • This is the first launch, OR
    //  • OpenCBM is not installed (e.g. after an OS upgrade)
    if (!setupDone || !OpenCBMWrapper::isInstalled()) {
        SetupWizard wizard;
        int result = wizard.exec();

        if (result == QDialog::Accepted) {
            settings.setValue("setup/completed", true);
        } else {
            // User cancelled — still allow launch so they can try again later
            // (the main window will show a warning about missing OpenCBM)
        }
    }

    // ── Main window ───────────────────────────────────────────────────────────
    MainWindow window;
    window.show();

    return app.exec();
}
