/**
  * Universidad de Costa Rica
  * Escuela de Ciencias de la Computación e Informática
  * Arquitectura de Computadores
  * Proyecto Programado Parte 2 - Simulacion procesadores MIPS en paralelo
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

//------------------------------------------
//| Constantes que hay en todo el programa |
//------------------------------------------

#define numBloquesMem   24

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
    void* procesador(int id, int pc, int idCPU);

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

    bool lw(int regX, int regY, int n, int* vecRegs, int *pTm, int *pTc, int *pTd, int *pTmX, int *pTcX, int *pTdX, int *pTmY, int *pTcY, int *pTdY);
    bool sw(int regX, int regY, int n, int *vecRegs, int *pTm, int *pTc, int *pTd, int *pTmX, int *pTcX, int *pTdX, int *pTmY, int *pTcY, int *pTdY);

    /**
     * Este método se encarga de poner "uncached" el bloque identificado por el parámetro
     * \var bloqueInvalidar en el directorio que se le envía (el puntero).
     * @brief uncachPage
     * @param Puntero al directorio al que pertenece el bloque
     * @param id del bloque a invalidar
     */
    void uncachPage(sDirectory *directorio, int bloqueInvalidar);
    /**
     * Copia de una memoria que ya se identificó cual es a una caché que ya se sabe cual es
     * el bloque identificado por \var bloqueReemplazar
     * @brief copiarAmemoria
     * @param Puntero a una memoria que se sabe que es la que corresponde
     * @param Puntero a una cache que se sabe que es la que corresponde
     * @param Identificador del bloque que se quiere reemplazar
     */
    void copiarAmemoria(sMemory* memoria, sCach *cache, int bloqueReemplazar);
    void copiarAmemoria(sCach *pointerC, int bloqueCache, sMemory *pointerM, sMemory *pointerMX, sMemory *pointerMY);
    /**
     * Va a copiar de una memoria ya identificada a una cache ya identificada el bloque identificado
     * por la variable \var idBloque.
     * @brief copiarAcache
     * @param memoria
     * @param cache
     * @param bloque
     */
    void copiarAcache(sMemory* memoria, sCache* cache, int idBloque);
    void copiarAcache(sCach *pointerC, int bloqueCache, int numBloque, sMemory *pointerM, sMemory *pointerMX, sMemory *pointerMY);

    void fin(int idThread, int* registros);


    //-------------------------
    //| Miembros de la clase. |
    //-------------------------
    int numThreads;
    int* vecPCs;

};

#endif // PRINCIPALTHREAD_H
