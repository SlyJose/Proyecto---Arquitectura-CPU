#include <QtGui>

#include "Window.h"

Window::Window(QWidget *parent): QDialog(parent){


    showProgress->show();

    //acceptButton = new QPushButton("Aceptar", this);
    //acceptButton->setGeometry(QRect(QPoint(200, 200), QSize(100, 25)));
    //connect(acceptButton, SIGNAL (released()), this, SLOT (handleButton()));

    QGridLayout *mainLayout = new QGridLayout;

    setLayout(mainLayout);
    setWindowTitle(tr("Hilos Procesados"));
    resize(500, 200);


}

void Window::closeWindow(){


}
