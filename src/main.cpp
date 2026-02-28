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

#include "app/MainWindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    // QApplication owns global UI state (theme, fonts, event dispatch, clipboard, etc.).
    QApplication app(argc, argv);

    // Load one icon from embedded resources. This works from any working directory.
    const QIcon appIcon(QStringLiteral(":/app-icon.svg"));
    app.setWindowIcon(appIcon);

    // MainWindow wires together domain + services + models + widgets.
    MainWindow window;

    // Setting the icon on both app and window improves consistency across platforms/shells.
    window.setWindowIcon(appIcon);
    window.show();

    // Enter Qt's event loop. Program exits when the last window closes (default behavior).
    return app.exec();
}
