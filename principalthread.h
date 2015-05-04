#ifndef PRINCIPALTHREAD_H
#define PRINCIPALTHREAD_H

#include <QString>
#include <QDebug>
#include <iterator>
#include <pthread.h>

//Los tipos de instrucciones que puede simular el procesador
#define DADDI   8
#define DADD    32
#define DSUB    34
#define LW      35
#define SW      43
#define BEQZ    4
#define BNEZ    5
#define FIN     63

//Los tipos de estado del directorio

#define U   0
#define C   1
#define M   2

class principalThread
{
public:
    principalThread(QString programa, int numProgramas);
    ~principalThread();

    int daddi(int regX, int regY, int n);
    int dadd(int regX, int regY, int regZ);
    int dsub(int regX, int regY, int regZ);

    bool lw(int regX, int regY, int n);
    bool sw(int regX, int regY, int n);

    void beqz(int regX, int etiq);
    void bnez(int regX, int etiq);

    void fin();

    void* procesador();

private:
    int* vecInstrucciones;  /* Es el vector que va a tener las instrucciones de todos los programas.*/
    int* vecPCs;            /* Vector con los indices donde inicia cada programa en el vector.*/


    int memoryCPU1[5][4];   /* Estructuras de datos para cada procesador */
    int cacheCPU1[6][4];
    int directCPU1[4][4];

};

#endif // PRINCIPALTHREAD_H
