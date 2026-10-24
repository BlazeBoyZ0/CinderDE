#include <QApplication>
#include "cinderde/main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("CinderDE");
    app.setOrganizationName("CinderDE");
    app.setApplicationVersion("1.0.0");

    CinderDE::MainWindow window;
    window.show();

    return app.exec();
}
