/**
  * Universidad de Costa Rica
  * Escuela de Ciencias de la Computación e Informática
  * Arquitectura de Computadores
  * Proyecto Programado Parte 1 - Simulacion procesador MIPS
  * @author Fabian Rodriguez
  * @author Jose Pablo Ureña
  * I Semestre 2015
  */


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QListWidget>
#include <QListWidgetItem>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()                        /* Boton que permite abrir ventana de busqueda de archivos */
{
    ui->textResult->setText("");
    ui->hideLine->setVisible(false);
    QString rutaArchivo = QFileDialog::getOpenFileName (this);      /* Se obtiene la ruta de los archivos seleccionados */
    QFile newFile(rutaArchivo);
    QString hileraArchivo;
    QString nuevaHileraArchivo;
    int numProgramas = 0;

    if (newFile.open (QIODevice::ReadOnly | QIODevice::Text)) {
        hileraArchivo = newFile.readAll();                     /* hileraArchivo almacena todo el archivo */
        ++numProgramas;
    }

    bool continuar = false;

    do{
        QMessageBox cargarNuevo;
        cargarNuevo.setText("¿Desea cargar un nuevo archivo para el CPU?");
        cargarNuevo.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        cargarNuevo.setDefaultButton(QMessageBox::No);
        int result = cargarNuevo.exec();
        if(result == 65536){                                   /* Se verifica si el usuario desea cargar mas archivos */
            continuar = false;

        }else{                                                 /* El usuario si desea cargar nuevos archivos */
            continuar = true;
            QString nuevaRutaArchivo = QFileDialog::getOpenFileName (this);

            QFile file(nuevaRutaArchivo);

            if (file.open (QIODevice::ReadOnly | QIODevice::Text)) {
                nuevaHileraArchivo = file.readAll();
            }
            hileraArchivo = hileraArchivo + "@" + nuevaHileraArchivo;
            ++numProgramas;
        }
    }while(continuar);

    principalThread programaPrincipal(hileraArchivo, numProgramas);
    QString estadisticas = programaPrincipal.controlador();    
    ui->textResult->setText(estadisticas);    

}


void MainWindow::on_aboutButton_clicked()
{
    vInfo = new Info(this);
    vInfo->show();
}

void MainWindow::on_pushButton_2_clicked()
{
    this->close();
}
