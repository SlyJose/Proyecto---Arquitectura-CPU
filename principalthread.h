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
#include <semaphore.h>
#include <errno.h>

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

struct sMemory{
    int memory[5][8];       // Memoria CPU
};
struct sCach{
    int cache[6][4];        // Cache CPU
};
struct sDirectory{
    int directory[8][5];    // Directory CPU
};


class principalThread{

#ifdef __APPLE__
    // Para Mac OSX no esta implementado pthread_barrier
    // esta implementacion se encontro en:
    //      http://stackoverflow.com/questions/3640853/performance-test-sem-t-v-s-dispatch-semaphore-t-and-pthread-once-t-v-s-dispat
public:

    typedef int pthread_barrierattr_t;
    typedef struct
    {
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        int count;
        int tripCount;
    } pthread_barrier_t;


    int pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count)
    {
        if(count == 0)
        {
            errno = EINVAL;
            return -1;
        }
        if(pthread_mutex_init(&barrier->mutex, 0) < 0)
        {
            return -1;
        }
        if(pthread_cond_init(&barrier->cond, 0) < 0)
        {
            pthread_mutex_destroy(&barrier->mutex);
            return -1;
        }
        barrier->tripCount = count;
        barrier->count = 0;

        return 0;
    }

    int pthread_barrier_destroy(pthread_barrier_t *barrier)
    {
        pthread_cond_destroy(&barrier->cond);
        pthread_mutex_destroy(&barrier->mutex);
        return 0;
    }

    int pthread_barrier_wait(pthread_barrier_t *barrier)
    {
        pthread_mutex_lock(&barrier->mutex);
        ++(barrier->count);
        if(barrier->count >= barrier->tripCount)
        {
            barrier->count = 0;
            pthread_cond_broadcast(&barrier->cond);
            pthread_mutex_unlock(&barrier->mutex);
            return 1;
        }
        else
        {
            pthread_cond_wait(&barrier->cond, &(barrier->mutex));
            pthread_mutex_unlock(&barrier->mutex);
            return 0;
        }
    }

#endif
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

    void cambiaCiclo();
    void esperaCambioCiclo();

    bool lw(int regX, int regY, int n, int* vecRegs, sMemory *pTm, sCach *pTc, sDirectory *pTd, sMemory *pTmX, sCach *pTcX, sDirectory *pTdX, sMemory *pTmY, sCach *pTcY, sDirectory *pTdY, int idCPU);
    bool sw(int regX, int regY, int n, int *vecRegs, sMemory *pTm, sCach *pTc, sDirectory *pTd, sMemory *pTmX, sCach *pTcX, sDirectory *pTdX, sMemory *pTmY, sCach *pTcY, sDirectory *pTdY, int idCPU);

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
    void copiarAcache(sMemory* memoria, sCach* cache, int idBloque);
    void copiarAcache(sCach *pointerC, int bloqueCache, int numBloque, sMemory *pointerM, sMemory *pointerMX, sMemory *pointerMY);

    void fin(int idThread, int* registros);

    int getCurrentPC();


    //-------------------------
    //| Miembros de la clase. |
    //-------------------------
    int numThreads;
    int m_indexPC;
    int* vecPCs;

};

#endif // PRINCIPALTHREAD_H
