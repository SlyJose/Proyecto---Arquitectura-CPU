/**
  * Universidad de Costa Rica
  * Escuela de Ciencias de la Computación e Informática
  * Arquitectura de Computadores
  * Proyecto Programado Parte 1 - Simulacion procesador MIPS
  * @author Fabian Rodriguez
  * @author Jose Pablo Ureña
  * I Semestre 2015
  */


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <string>
#include <iostream>
#include <QDebug>
#include <QMessageBox>
#include "principalthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
