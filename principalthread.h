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
#include <QObject>
#include <iterator>
#include <pthread.h>

//---------------------------------------------------------------
//| Los tipos de instrucciones que puede simular el procesador. |
//---------------------------------------------------------------

#define DADDI   8
#define DADD    32
#define DSUB    34
#define LW      35
#define SW      43
#define BEQZ    4
#define BNEZ    5
#define FIN     63

//---------------------------------------------------------
//| Los tipos de estado del bloque en directorio y cache. |
//---------------------------------------------------------

#define U   0
#define C   1
#define M   2
#define I   3

//--------------------------------------
//| Identificadores de cada procesador |
//--------------------------------------

#define CPU0    0
#define CPU1    1
#define CPU2    2

struct threadData{      /*!< Para pasar parametros a los threads. */
    void* ptr;
    int idThread;
    int numPC;
    int idCPU;
};

class principalThread{

private:

    /**
     * Este va a ser el proceso que se va a realizar con los hilos. Hace la simulacion completa
     * del procesador MIPS.
     * @brief procesador
     * @param PC que indica la posicion de la primera instruccion del programa que le corresponde a este hilo.
     * @return void* ya que asi lo ocupa los pthreads.
     */
    void* procesador(int id, int pc);

    static void* procesadorHelper(void* threadStruct);

public:

    principalThread(QString programa, int numHilos); //constructor
    ~principalThread(); //destructor

    /**
     * Este va a ser el proceso grande que va a controlar la distribucion de los hilos entre los procesadores.
     * Tambien va a llevar cuenta del reloj de la CPU y controlar la barrera ciclica.
     * @brief controlador
     */
    QString controlador();


private:

    //-----------------------------------
    //| Funciones privadas de la clase. |
    //-----------------------------------

    bool lw(int regX, int regY, int n, int* vecRegs);
    bool sw(int regX, int regY, int n, int *vecRegs);

    void fin(int idThread, int* registros);

    //-------------------------
    //| Miembros de la clase. |
    //-------------------------
    int numThreads;
    int* vecPCs;

};

#endif // PRINCIPALTHREAD_H
