/**
  * Universidad de Costa Rica
  * Escuela de Ciencias de la Computación e Informática
  * Arquitectura de Computadores
  * Proyecto Programado Parte 1 - Simulacion procesador MIPS
  * @author Fabian Rodriguez
  * @author Jose Pablo Ureña
  * I Semestre 2015
  */

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

    /**
     * Este va a ser el proceso que se va a realizar con los hilos. Hace la simulacion completa
     * del procesador MIPS.
     * @brief procesador
     * @param PC que indica la posicion de la primera instruccion del programa que le corresponde a este hilo.
     * @return void* ya que asi lo ocupa los pthreads.
     */
    void* procesador(void *PC);

private:

    // Funciones privadas de la clase.

    bool lw(int regX, int regY, int n, int* vecRegs);
    bool sw(int regX, int regY, int n);

    void fin();

    // Miembros de la clase
    int* vecInstrucciones;  /*!< Es el vector que va a tener las instrucciones de todos los programas.*/
    int* vecPCs;            /*!< Vector con los indices donde inicia cada programa en el vector.*/


    int memoryCPU1[5][4];   /* Estructuras de datos para cada procesador */
    int cacheCPU1[6][4];
    int directCPU1[4][4];

};

#endif // PRINCIPALTHREAD_H
