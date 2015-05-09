/**
  * Universidad de Costa Rica
  * Escuela de Ciencias de la Computación e Informática
  * Arquitectura de Computadores
  * Proyecto Programado Parte 1 - Simulacion procesador MIPS
  * @author Fabian Rodriguez
  * @author Jose Pablo Ureña
  * I Semestre 2015
  */


#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);  /* GIt HUB PRUEBA */

    QCoreApplication::setOrganizationName("Manchis");
    QCoreApplication::setApplicationName("Qt-SmallTextEditor");


    MainWindow w;
    w.show();

    return a.exec();
}
