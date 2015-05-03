#ifndef PRINCIPALTHREAD_H
#define PRINCIPALTHREAD_H

#include <QString>
#include <QDebug>
#include <iterator>
#include <pthread.h>
#include "methods.h"

//Los tipos de instrucciones que puede simular el procesador
#define DADDI   8
#define DADD    32
#define DSUB    34
#define LW      35
#define SW      43
#define BEQZ    4
#define BNEZ    5
#define FIN     63

class principalThread
{
public:
    principalThread(QString programa, int numProgramas);
    ~principalThread();

    void procesador();

private:
    int* vecInstrucciones;  /* Es el vector que va a tener las instrucciones de todos los programas.*/
    int* vecPCs;            /* Vector con los indices donde inicia cada programa en el vector.*/
};

#endif // PRINCIPALTHREAD_H
