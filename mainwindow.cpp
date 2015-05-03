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
     ui->textResult->setText(hileraArchivo);
     principalThread programaPrincipal(hileraArchivo, numProgramas);
}
















