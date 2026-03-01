/*
    File: main.cpp
    Purpose:
      - Entry point for the INIX desktop application.
      - Creates QApplication, applies the shared window icon, and starts the event loop.

    How it fits in the codebase:
      - Bootstraps the UI layer by constructing app::MainWindow.
      - Uses the Qt resource path (:/app-icon.svg) prepared in src/assets/resources.qrc.
      - Hands control to Qt's event loop via app.exec(), where all signals/slots are processed.
*/

#define _WIN32_WINNT 0x0603  // Windows 8.1 (required for SetProcessDpiAwareness)

#include "app/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QLocale>
#include <QTranslator>
#include <ShellScalingApi.h>

#pragma comment(lib, "Shcore.lib")


int main(int argc, char* argv[]) {
    

    // Enable high DPI scaling on Windows and Linux. On macOS, this is enabled by default and has no effect.
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    
    // QApplication owns global UI state (theme, fonts, event dispatch, clipboard, etc.).
    QApplication app(argc, argv);

    // Load one icon from embedded resources. This works from any working directory.
    const QIcon appIcon(QStringLiteral(":/app-icon.svg"));
    app.setWindowIcon(appIcon);

    // Load application translations from "<exe-dir>/translations".
    // Prefer the OS UI language (for example en-US), not numeric/date culture.
    QTranslator appTranslator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    const QLocale translationLocale =
        uiLanguages.isEmpty() ? QLocale::system() : QLocale(uiLanguages.at(0));
    if (appTranslator.load(translationLocale, QStringLiteral("inix"), QStringLiteral("_"),
                           QCoreApplication::applicationDirPath() + QStringLiteral("/translations"))) {
        app.installTranslator(&appTranslator);
    }

    // MainWindow wires together domain + services + models + widgets.
    MainWindow window;

    // Setting the icon on both app and window improves consistency across platforms/shells.
    window.setWindowIcon(appIcon);
    window.show();

    // Enter Qt's event loop. Program exits when the last window closes (default behavior).
    return app.exec();
}
