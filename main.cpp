#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Manchis");
    QCoreApplication::setApplicationName("Qt-SmallTextEditor");


    MainWindow w;
    w.show();

    return a.exec();
}
