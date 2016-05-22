#include <QSurfaceFormat>
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName("Dark Cropper");
    QCoreApplication::setOrganizationName("cmdrkotori");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
