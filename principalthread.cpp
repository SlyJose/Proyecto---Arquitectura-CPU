/**
  * Universidad de Costa Rica
  * Escuela de Ciencias de la Computación e Informática
  * Arquitectura de Computadores
  * Proyecto Programado Parte 2 - Simulacion procesadores MIPS en paralelo
  * @author Fabian Rodriguez
  * @author Jose Pablo Ureña
  * I Semestre 2015
  */


#include "principalthread.h"

/*---- Variables globales (memoria compartida) -----*/


sMemory sMem;               /*!< Memoria del CPU0. */
sCach sCache;               /*!< Cache del CPU0. */
sDirectory sDirect;         /*!< Directorio del CPU0. */

sMemory sMem1;              /*!< Memoria del CPU1. */
sCach sCache1;              /*!< Cache del CPU1. */
sDirectory sDirect1;        /*!< Directorio del CPU1. */

sMemory sMem2;              /*!< Memoria del CPU2. */
sCach sCache2;              /*!< Cache del CPU2. */
sDirectory sDirect2;        /*!< Directorio del CPU2. */

int* vecPrograma;           /*!< Estructura donde se van a almacenar las instrucciones de todos los programas a correr. */
std::queue<int> colaPCs;    /*!< Cola que lleva los indices del vector del programa donde inicia cada programa nuevo. */
int numThreads;             /*!< Numero total de programas que hay que ejecutar. */
int idThread;               /*!< Identificador del hilo. */
QString estadisticas;       /*!< Donde se van a guardar los datos para mostrar al final de la ejecucion. */
int reloj;                  /*!< Va a indicar por cual ciclo de reloj se encuentran los procesadores. */

/* Semaforos para los recursos críticos y para lograr sincronizacion entre los procesadores */
pthread_mutex_t mutCache = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutCache1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutCache2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutMem = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutMem1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutMem2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutDir = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutDir1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutDir2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutClock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutCPU0 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutCPU1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutCPU2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutEstadisticas = PTHREAD_MUTEX_INITIALIZER;
sem_t semReloj;
/*------------------------------------------------------------------------------------------*/

principalThread::principalThread(QString programa, int numHilos)
{
    idThread = 0;
    sem_init(&semReloj, 0, 3);  //inicializo el semaforo en 3
    reloj = 0;
    numThreads = numHilos;
    QString::iterator it;
    QString strTemp;
    int tamVec = 1;
    colaPCs.push(0);
    for(it = programa.begin(); it!= programa.end(); ++it){
        if(*it == ' ' || *it == '\n'){
            ++it;
            strTemp.append('|');
            ++tamVec;
        }
        if( *it == '@'){
            strTemp.append('@');
            ++it;
        }
        strTemp.append(*it);
    }

    vecPrograma = new int[tamVec];
    int j=0;
    bool unaCifra = true;
    int numCifras = 1;
    bool signo = false;
    QString numTemp;
    bool ok;

    for(it = strTemp.begin(); it!=strTemp.end(); ++it){
        if(*it == '@'){
            colaPCs.push(j);
        }else{
            if( *it == '|'){
                vecPrograma[j] =numTemp.toInt(&ok, 10);
                ++j;
                unaCifra = true;
                numCifras = 1;
                signo = 1;
                numTemp.clear();
            }else{
                numTemp.append(*it);
            }
        }
    }

    //Inicializa los valores de las memorias, las caches y los directorios.

    int contador = 0;
    for(int i=0; i<5; ++i){                     /* Se inicializa la memoria*/
        for(int j=0; j<8; ++j){
            if(i == 4){
                sMem.memory[i][j] = contador;           // Etiquetas de los bloques
                sMem1.memory[i][j] = contador + 8;
                sMem2.memory[i][j] = contador + 16;
            }else{
                sMem.memory[i][j] = 0;
                sMem1.memory[i][j] = 0;
                sMem2.memory[i][j] = 0;
            }
            ++contador;
        }
    }

    for(int i=0; i<6; ++i){                     /* Se inicializa cada cache de CPU. */
        for(int j=0; j<4; ++j){
            if(i==4){
                sCache.cache[i][j] = -1;
                sCache1.cache[i][j] = -1;
                sCache2.cache[i][j] = -1;
            }else{
                if(i==5){
                    sCache.cache[i][j] = I;
                    sCache1.cache[i][j] = I;
                    sCache2.cache[i][j] = I;
                }else{
                    sCache.cache[i][j] = 0;
                    sCache1.cache[i][j] = 0;
                    sCache2.cache[i][j] = 0;
                }
            }
        }
    }

    for(int i=0; i<8; ++i){                                 /* Se inicializa cada directorio de CPU */
        for(int j=0; j<5; ++j){

            if( j==0 ){                                      // Llenado de columna cero y su respectivo numero de bloke
                sDirect.directory[i][j] = i;
                sDirect1.directory[i][j] = i+8;
                sDirect2.directory[i][j] = i+16;
            }else{
                if( j==1 ){                                  // LLenado de columna uno y su respectiva etiqueta de bloke
                    sDirect.directory[i][j] = U;
                    sDirect1.directory[i][j] = U;
                    sDirect2.directory[i][j] = U;
                }else{
                    sDirect.directory[i][j] = 0;
                    sDirect1.directory[i][j] = 0;
                    sDirect2.directory[i][j] = 0;
                }
            }
        }
    }
}


principalThread::~principalThread()
{

}

bool principalThread::lw(int regX, int regY, int n, int *vecRegs, sMemory *pTm, sCach *pTc, sDirectory *pTd, sMemory *pTmX, sCach *pTcX, sDirectory *pTdX, sMemory *pTmY, sCach *pTcY, sDirectory *pTdY, int idCPU)
{    
    int dirPrev = n + vecRegs[regY];
    int numBloque = dirPrev/16;
    int bloqueCache = numBloque%4;  /* Numero del bloque a buscar en el cache*/
    int filaCache = (dirPrev%16)/4;
    
    int resultLockCache;
    switch(idCPU){  //intento bloquear la cache que corresponde a la local dependiendo de cual CPU soy
    case CPU0:
        resultLockCache = pthread_mutex_trylock(&mutCache);
        break;
    case CPU1:
        resultLockCache = pthread_mutex_trylock(&mutCache1);
        break;
    case CPU2:
        resultLockCache = pthread_mutex_trylock(&mutCache2);
        break;
    }
    if(resultLockCache == 0){
        if(pTc->cache[4][bloqueCache] == numBloque && pTc->cache[5][bloqueCache] != I){     // Hay un hit :)
            vecRegs[regX] = pTc->cache[filaCache][bloqueCache]; //lo pongo en el registro
            switch(idCPU){      //libera la cache que corresponde a la local dependiendo de cual CPU soy
            case CPU0:
                pthread_mutex_unlock(&mutCache);
                break;
            case CPU1:
                pthread_mutex_unlock(&mutCache1);
                break;
            case CPU2:
                pthread_mutex_unlock(&mutCache2);
                break;
            }
            return true;
        }else{      // Hay un fallo de caché :(

            // ----------------------------------------------------------------------------------
            // |        Quito el bloque modificado , que le voy a caer encima, de la cache      |
            // ----------------------------------------------------------------------------------

            // Saco el bloque de mi cache y lo guardo en la memoria que corresponda
            // pero tengo que ir a ponerlo como "uncached" en el directorio que corresponda.
            if(pTc->cache[5][bloqueCache] == M){    //donde va a poner el bloque esta modificado?

                int bloqueReemplazar = pTc->cache[4][bloqueCache];

                if(pTd->directory[0][0]<= bloqueReemplazar  && bloqueReemplazar <= pTd->directory[7][0]){  //esta en mi memoria?
                    int resultLockDir;
                    switch(idCPU){      //intento bloquear mi directorio
                    case CPU0:
                        resultLockDir = pthread_mutex_trylock(&mutDir);
                        break;
                    case CPU1:
                        resultLockDir = pthread_mutex_trylock(&mutDir1);
                        break;
                    case CPU2:
                        resultLockDir = pthread_mutex_trylock(&mutDir2);
                        break;
                    }
                    if(resultLockDir == 0){
                        esperaCiclos(2, idCPU); //Dura 2 ciclos en obtener el directorio
                        uncachPage(pTd, bloqueReemplazar);  //pongo uncached el bloque en mi directorio
                        esperaCiclos(16, idCPU);    //Dura 16 ciclos en copiar de cache local a memoria local
                        copiarAmemoria(pTm, pTc, bloqueReemplazar);     //lo muevo de cache a memoria
                        esperaCiclos(1, idCPU);     //Dura 1 ciclo en invalidar la cache
                        pTc->cache[5][bloqueCache] = I;     //invalido el bloque en cache
                        switch(idCPU){
                        case CPU0:
                            pthread_mutex_unlock(&mutDir);
                            break;
                        case CPU1:
                            pthread_mutex_unlock(&mutDir1);
                            break;
                        case CPU2:
                            pthread_mutex_unlock(&mutDir2);
                            break;
                        }
                    }else{
                        switch(idCPU){
                        case CPU0:
                            pthread_mutex_unlock(&mutCache);
                            break;
                        case CPU1:
                            pthread_mutex_unlock(&mutCache1);
                            break;
                        case CPU2:
                            pthread_mutex_unlock(&mutCache2);
                            break;
                        }
                        return false;
                    }
                }
                if(bloqueReemplazar >= pTdX->directory[0][0] && bloqueReemplazar <= pTdX->directory[7][0]){ //esta en memoria remota 1?
                    int resultLockDir;
                    switch(idCPU){      //intento bloquear el directorio remoto 1
                    case CPU0:
                        resultLockDir = pthread_mutex_trylock(&mutDir1);
                        break;
                    case CPU1:
                        resultLockDir = pthread_mutex_trylock(&mutDir);
                        break;
                    case CPU2:
                        resultLockDir = pthread_mutex_trylock(&mutDir);
                        break;
                    }
                    if(resultLockDir == 0){
                        esperaCiclos(4, idCPU); //Dura 4 ciclos en obtener el bloqueo de directorio remoto
                        uncachPage(pTdX, bloqueReemplazar); //lo pongo uncached
                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque en cache a una memoria remota
                        copiarAmemoria(pTmX, pTc, bloqueReemplazar);    //lo muevo de cache a memoria
                        esperaCiclos(1, idCPU);     //Dura 1 ciclo en invalidar una cache
                        pTc->cache[5][bloqueCache] = I;     //invalido el bloque en cache
                        switch(idCPU){
                        case CPU0:
                            pthread_mutex_unlock(&mutDir1);
                            break;
                        case CPU1:
                            pthread_mutex_unlock(&mutDir);
                            break;
                        case CPU2:
                            pthread_mutex_unlock(&mutDir);
                            break;
                        }
                    }else{
                        switch(idCPU){
                        case CPU0:
                            pthread_mutex_unlock(&mutDir1);
                            break;
                        case CPU1:
                            pthread_mutex_unlock(&mutDir);
                            break;
                        case CPU2:
                            pthread_mutex_unlock(&mutDir);
                            break;
                        }
                        return false;
                    }
                }
                if(bloqueReemplazar >= pTdY->directory[0][0] && bloqueReemplazar <= pTdY->directory[7][0]){ //esta en memoria remota 2?
                    int resultLockDir;
                    switch(idCPU){      //intento bloquear el directorio remoto 2
                    case CPU0:
                        resultLockDir = pthread_mutex_trylock(&mutDir2);
                        break;
                    case CPU1:
                        resultLockDir = pthread_mutex_trylock(&mutDir1);
                        break;
                    case CPU2:
                        resultLockDir = pthread_mutex_trylock(&mutDir1);
                        break;
                    }
                    if(resultLockDir == 0){
                        esperaCiclos(4, idCPU);     //Dura 4 ciclos en obtener un directorio remoto
                        uncachPage(pTdY, bloqueReemplazar);     //pone uncached el bloque
                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en copiar un bloque de cache a una memoria remota
                        copiarAmemoria(pTmY, pTc, bloqueReemplazar);    //lo muevo a memoria
                        esperaCiclos(1, idCPU);     //Dura 1 ciclo en invaludar un bloque en cache
                        pTc->cache[5][bloqueCache] = I;     //invalido el bloque en cache
                        switch(idCPU){
                        case CPU0:
                            pthread_mutex_unlock(&mutDir2);
                            break;
                        case CPU1:
                            pthread_mutex_unlock(&mutDir1);
                            break;
                        case CPU2:
                            pthread_mutex_unlock(&mutDir1);
                            break;
                        }
                    }else{
                        switch(idCPU){
                        case CPU0:
                            pthread_mutex_unlock(&mutDir2);
                            break;
                        case CPU1:
                            pthread_mutex_unlock(&mutDir1);
                            break;
                        case CPU2:
                            pthread_mutex_unlock(&mutDir1);
                            break;
                        }
                        return false;
                    }
                }
            }

            // -----------------------------------------------------------------
            // |        Subo el bloque de memoria a cache (Write allocate)     |
            // -----------------------------------------------------------------

            // Tengo que ver si alguien lo tiene modificado en su directorio primero
            // y ademas se que no lo tengo en mi cache local, pero puede que el
            // bloque si me pertenezca -> lo tengo en mi directorio
            if(pTd->directory[0][0] <= numBloque && numBloque <= pTd->directory[7][0]){ //el bloque que tengo que subir me pertenece a mi?
                int resultBloqueDirect;
                switch(idCPU){  //trato de bloquear el directorio local
                case CPU0:
                    resultBloqueDirect = pthread_mutex_trylock(&mutDir);
                    break;
                case CPU1:
                    resultBloqueDirect = pthread_mutex_trylock(&mutDir1);
                    break;
                case CPU2:
                    resultBloqueDirect = pthread_mutex_trylock(&mutDir2);
                    break;
                }
                if(resultBloqueDirect == 0){    //obtengo el bloqueo del directorio local
                    esperaCiclos(2, idCPU);     //Dura 2 ciclos en obtener el bloqueo del directorio local
                    int indicePosMemDir = numBloque%8;
                    if(pTd->directory[indicePosMemDir][0] == numBloque){    //lo ubico en mi directorio
                        if(pTd->directory[indicePosMemDir][1] == M){   //esta modificado en algun lado?
                            switch(idCPU){
                            case CPU0:
                                if(pTd->directory[indicePosMemDir][3] == 1){       // CPU1 lo tiene modificado?
                                    if(pthread_mutex_trylock(&mutCache1) == 0){     //intento bloquear mutCache1
                                        // Me dieron la cache entonces la copio a mi memoria, le cambio el estado
                                        // a compartido y lo subo a cache
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria local
                                        copiarAmemoria(pTm, pTcX, numBloque);   //copio el bloque modificado a mi memoria
                                        pTcX->cache[5][bloqueCache] = C;
                                        pTd->directory[indicePosMemDir][2] = 1;     // Actualizo el directorio
                                        pTd->directory[indicePosMemDir][1] = C;
                                        pthread_mutex_unlock(&mutCache1);
                                        for(int i=0; i<4; ++i){     //Subo el bloque de memoria a cache
                                            pTc->cache[i][bloqueCache] =  pTm->memory[i][numBloque%8];
                                        }
                                    }else{
                                        // Libero los recursos que habia adquirido
                                        pthread_mutex_unlock(&mutDir);
                                        pthread_mutex_unlock(&mutCache);
                                        return false;
                                    }
                                }
                                if(pTd->directory[indicePosMemDir][4] == 1){       // CPU2 lo tiene modificado?
                                    if(pthread_mutex_trylock(&mutCache2) == 0){     //intento bloquear mutCache2
                                        // Me dieron la cache entonces la copio a mi memoria, le cambio el estado
                                        // a compartido y lo subo a cache
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria local
                                        copiarAmemoria(pTm, pTcY, numBloque);   //copio el bloque modificado a mi memoria
                                        pTcY->cache[5][bloqueCache] = C;
                                        pTd->directory[indicePosMemDir][2] = 1;     // Actualizo el directorio
                                        pTd->directory[indicePosMemDir][1] = C;
                                        pthread_mutex_unlock(&mutCache2);
                                        for(int i=0; i<4; ++i){     //Subo el bloque de memoria a cache
                                            pTc->cache[i][bloqueCache] =  pTm->memory[i][numBloque%8];
                                        }
                                    }else{
                                        // Libero los recursos que habia adquirido
                                        pthread_mutex_unlock(&mutDir);
                                        pthread_mutex_unlock(&mutCache);
                                        return false;
                                    }
                                }
                                break;
                            case CPU1:
                                if(pTd->directory[indicePosMemDir][2] == 1){
                                    if(pthread_mutex_trylock(&mutCache) == 0){      //intento bloquear mutCache
                                        // Me dieron la cache entonces la copio a mi memoria, le cambio el estado
                                        // a compartido y lo subo a cache
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria local
                                        copiarAmemoria(pTm, pTcX, numBloque);   //copio el bloque modificado a mi memoria
                                        pTcX->cache[5][bloqueCache] = C;
                                        pTd->directory[indicePosMemDir][2] = 1;     // Actualizo el directorio
                                        pTd->directory[indicePosMemDir][1] = C;
                                        pthread_mutex_unlock(&mutCache);
                                        for(int i=0; i<4; ++i){     //Subo el bloque de memoria a cache
                                            pTc->cache[i][bloqueCache] =  pTm->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir1);
                                        pthread_mutex_unlock(&mutCache1);
                                        return false;
                                    }
                                }
                                if(pTd->directory[indicePosMemDir][4] == 1){
                                    if(pthread_mutex_trylock(&mutCache2) == 0){     //intento bloquear mutCache2
                                        // Me dieron la cache entonces la copio a mi memoria, le cambio el estado
                                        // a compartido y lo subo a cache
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria local
                                        copiarAmemoria(pTm, pTcY, numBloque);   //copio el bloque modificado a mi memoria
                                        pTcY->cache[5][bloqueCache] = C;
                                        pTd->directory[indicePosMemDir][2] = 1;     // Actualizo el directorio
                                        pTd->directory[indicePosMemDir][1] = C;
                                        pthread_mutex_unlock(&mutCache2);
                                        for(int i=0; i<4; ++i){     //Subo el bloque de memoria a cache
                                            pTc->cache[i][bloqueCache] =  pTm->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir1);
                                        pthread_mutex_unlock(&mutCache1);
                                        return false;
                                    }
                                }
                                break;
                            case CPU2:
                                if(pTd->directory[indicePosMemDir][2] == 1){
                                    if(pthread_mutex_trylock(&mutCache) == 0){      //intento bloquear mutCache
                                        // Me dieron la cache entonces la copio a mi memoria, le cambio el estado
                                        // a compartido y lo subo a cache
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria local
                                        copiarAmemoria(pTm, pTcX, numBloque);   //copio el bloque modificado a mi memoria
                                        pTcX->cache[5][bloqueCache] = C;
                                        pTd->directory[indicePosMemDir][2] = 1;     // Actualizo el directorio
                                        pTd->directory[indicePosMemDir][1] = C;
                                        pthread_mutex_unlock(&mutCache);
                                        for(int i=0; i<4; ++i){     //Subo el bloque de memoria a cache
                                            pTc->cache[i][bloqueCache] =  pTm->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir2);
                                        pthread_mutex_unlock(&mutCache2);
                                        return false;
                                    }
                                }
                                if(pTd->directory[indicePosMemDir][3] == 1){
                                    if(pthread_mutex_trylock(&mutCache1) == 0){     //intento bloquear mutCache1
                                        // Me dieron la cache entonces la copio a mi memoria, le cambio el estado
                                        // a compartido y lo subo a cache
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria local
                                        copiarAmemoria(pTm, pTcY, numBloque);   //copio el bloque modificado a mi memoria
                                        pTcY->cache[5][bloqueCache] = C;
                                        pTd->directory[indicePosMemDir][2] = 1;     // Actualizo el directorio
                                        pTd->directory[indicePosMemDir][1] = C;
                                        pthread_mutex_unlock(&mutCache1);
                                        for(int i=0; i<4; ++i){     //Subo el bloque de memoria a cache
                                            pTc->cache[i][bloqueCache] =  pTm->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir2);
                                        pthread_mutex_unlock(&mutCache2);
                                        return false;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    switch(idCPU){  //libero el directorio local
                    case CPU0:
                        pthread_mutex_unlock(&mutDir);
                        break;
                    case CPU1:
                        pthread_mutex_unlock(&mutDir1);
                        break;
                    case CPU2:
                        pthread_mutex_unlock(&mutDir2);
                        break;
                    }
                }else{  //no me dieron el bloqueo del directorio local
                    switch(idCPU){
                    case CPU0:
                        pthread_mutex_unlock(&mutCache);
                        break;
                    case CPU1:
                        pthread_mutex_unlock(&mutCache1);
                        break;
                    case CPU2:
                        pthread_mutex_unlock(&mutCache2);
                        break;
                    }
                    return false;
                }
            }
            if(pTdX->directory[0][0] <= numBloque && numBloque <= pTdX->directory[7][0]){   //el bloque que tengo que subir pertenece al directorio remoto 1
                int resultLockDir;
                switch(idCPU){
                case CPU0:
                    resultLockDir = pthread_mutex_trylock(&mutDir1);   //Si soy CPU0 -> dirRemoto 1 = directorio de CPU1
                    break;
                case CPU1:
                    resultLockDir = pthread_mutex_trylock(&mutDir);    //Si soy CPU1 -> dirRemoto 1 = directorio de CPU0
                    break;
                case CPU2:
                    resultLockDir = pthread_mutex_trylock(&mutDir);    //Si soy CPU2 -> dirRemoto 1 = directorio de CPU0
                    break;
                }
                if(resultLockDir == 0){    //Si me dieron el directorio
                    esperaCiclos(4, idCPU); //Dura 4 ciclos en obtener el bloqueo de un directorio remoto
                    int posDirectorio = numBloque%8;
                    if(pTdX->directory[posDirectorio][0] == numBloque){   // Ubico el bloque en el directorio
                        if(pTdX->directory[posDirectorio][1] == M){     //Esta modificado?
                            switch(idCPU){//busco donde esta modificado
                            case CPU0:
                                if(pTdX->directory[posDirectorio][3] == 1){
                                    if(pthread_mutex_trylock(&mutCache1) == 0){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmX, pTcX, numBloque);
                                        pTcX->cache[5][bloqueCache] = C;
                                        pTdX->directory[posDirectorio][1] = C;  //actualizo el directorio remoto 1
                                        pTdX->directory[posDirectorio][2] = 1;
                                        pthread_mutex_unlock(&mutCache1);
                                        for(int i=0; i<4; ++i){      // Sube el bloque de la memoria remota 1 a la mi cache
                                            pTc->cache[i][bloqueCache] =  pTmX->memory[i][numBloque%8];
                                        }
                                    }else{
                                        //libero directorio y cache
                                        pthread_mutex_unlock(&mutDir1);
                                        pthread_mutex_unlock(&mutCache);
                                        return false;
                                    }
                                }
                                if(pTdX->directory[posDirectorio][4] == 1){
                                    if(pthread_mutex_trylock(&mutCache2) == 0){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmX, pTcY, numBloque);
                                        pTcY->cache[5][bloqueCache] = C;
                                        pTdX->directory[posDirectorio][1] = C;  //actualizo el directorio remoto 1
                                        pTdX->directory[posDirectorio][2] = 1;
                                        pthread_mutex_unlock(&mutCache2);
                                        for(int i=0; i<4; ++i){      // Sube el bloque de la memoria remota 1 a la mi cache
                                            pTc->cache[i][bloqueCache] =  pTmX->memory[i][numBloque%8];
                                        }
                                    }else{
                                        //libero directorio y cache
                                        pthread_mutex_unlock(&mutDir1);
                                        pthread_mutex_unlock(&mutCache);
                                        return false;
                                    }
                                }
                                break;
                            case CPU1:
                                if(pTdX->directory[posDirectorio][2] == 1){
                                    if(pthread_mutex_trylock(&mutCache)){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmX, pTcX, numBloque);
                                        pTcX->cache[5][bloqueCache] = C;
                                        pTdX->directory[posDirectorio][1] = C;  //actualizo el directorio remoto 1
                                        pTdX->directory[posDirectorio][3] = 1;
                                        pthread_mutex_unlock(&mutCache);
                                        for(int i=0; i<4; ++i){      // Sube el bloque de la memoria remota 1 a la mi cache
                                            pTc->cache[i][bloqueCache] =  pTmX->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir);
                                        pthread_mutex_unlock(&mutCache1);
                                        return false;
                                    }

                                }
                                if(pTdX->directory[posDirectorio][4] == 1){
                                    if(pthread_mutex_trylock(&mutCache2)){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmX, pTcY, numBloque);
                                        pTcY->cache[5][bloqueCache] = C;
                                        pTdX->directory[posDirectorio][1] = C;
                                        pTdX->directory[posDirectorio][3] = 1;
                                        pthread_mutex_unlock(&mutCache2);
                                        for(int i=0; i<4; ++i){      // Sube el bloque de la memoria remota 1 a la mi cache
                                            pTc->cache[i][bloqueCache] =  pTmX->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir);
                                        pthread_mutex_unlock(&mutCache1);
                                        return false;
                                    }
                                }
                                break;
                            case CPU2:
                                if(pTdX->directory[posDirectorio][2] == 1){
                                    if(pthread_mutex_trylock(&mutCache) == 0){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmX, pTcX, numBloque);
                                        pTcX->cache[5][bloqueCache] = C;
                                        pTdX->directory[posDirectorio][1] = C;
                                        pTdX->directory[posDirectorio][4] = 1;
                                        pthread_mutex_unlock(&mutCache);
                                        for(int i=0; i<4; ++i){      // Sube el bloque de la memoria remota 1 a la mi cache
                                            pTc->cache[i][bloqueCache] =  pTmX->memory[i][numBloque%8];
                                        }

                                    }else{
                                        pthread_mutex_unlock(&mutDir);
                                        pthread_mutex_unlock(&mutCache2);
                                        return false;
                                    }
                                }
                                if(pTdX->directory[posDirectorio][3] == 1){
                                    if(pthread_mutex_trylock(&mutCache1) == 0){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmX, pTcY, numBloque);
                                        pTcY->cache[5][bloqueCache] = C;
                                        pTdX->directory[posDirectorio][1] = C;
                                        pTdX->directory[posDirectorio][4] = 1;
                                        pthread_mutex_unlock(&mutCache1);
                                        for(int i=0; i<4; ++i){      // Sube el bloque de la memoria remota 1 a la mi cache
                                            pTc->cache[i][bloqueCache] =  pTmX->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir);
                                        pthread_mutex_unlock(&mutCache2);
                                        return false;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    // Libera el directorio remoto 1
                    switch(idCPU){
                    case CPU0:
                        pthread_mutex_unlock(&mutDir1);
                        break;
                    case CPU1:
                        pthread_mutex_unlock(&mutDir);
                        break;
                    case CPU2:
                        pthread_mutex_unlock(&mutDir);
                        break;
                    }
                }else{  //no me dieron el bloqueo del directorio remoto 1, libero la cache
                    switch(idCPU){
                    case CPU0:
                        pthread_mutex_unlock(&mutCache);
                        break;
                    case CPU1:
                        pthread_mutex_unlock(&mutCache1);
                        break;
                    case CPU2:
                        pthread_mutex_unlock(&mutCache2);
                        break;
                    }
                    return false;
                }
            }
            if(pTdY->directory[0][0] <= numBloque && numBloque <= pTdY->directory[7][0]){   //el bloque que tengo que subir pertenece al directorio remoto 2
                int resultLockDir;
                switch(idCPU){
                case CPU0:
                    resultLockDir = pthread_mutex_trylock(&mutDir2);
                    break;
                case CPU1:
                    resultLockDir = pthread_mutex_trylock(&mutDir1);
                    break;
                case CPU2:
                    resultLockDir = pthread_mutex_trylock(&mutDir1);
                    break;
                }
                if(resultLockDir == 0){ //obtuve el lock sobre el directorio remoto 2?
                    esperaCiclos(4, idCPU);     //Dura 4 ciclos en obtener el bloqueo del directorio remoto
                    int posDirectorio = numBloque%8;
                    if(pTdY->directory[posDirectorio][0] == numBloque){     // Ubico el bloque en el directorio remoto 2
                        if(pTdY->directory[posDirectorio][1] == M){ // Esta modificado?
                            switch(idCPU){
                            case CPU0:
                                if(pTdY->directory[posDirectorio][3] == 1){ //CPU1 lo tiene modificado?
                                    if(pthread_mutex_trylock(&mutCache1)){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmY, pTcX, numBloque);
                                        pTcX->cache[5][bloqueCache] = C;
                                        pTdY->directory[posDirectorio][1] = C;
                                        pTdY->directory[posDirectorio][2] = 1;
                                        pthread_mutex_unlock(&mutCache1);
                                        for(int i=0; i<4; ++i){                        // Subo el bloque de la memoria remota 2 a mi cache
                                            pTc->cache[i][bloqueCache] =  pTmY->memory[i][numBloque%8];
                                        }
                                    }else{
                                        //libero directorio y cache
                                        pthread_mutex_unlock(&mutDir2);
                                        pthread_mutex_unlock(&mutCache);
                                        return false;
                                    }
                                }
                                if(pTdY->directory[posDirectorio][4] == 1){ //CPU2 lo tiene modificado?
                                    if(pthread_mutex_trylock(&mutCache2)){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmY, pTcY, numBloque);
                                        pTcY->cache[5][bloqueCache] = C;
                                        pTdY->directory[posDirectorio][1] = C;
                                        pTdY->directory[posDirectorio][2] = 1;
                                        pthread_mutex_unlock(&mutCache2);
                                        for(int i=0; i<4; ++i){                        // Subo el bloque de la memoria remota 2 a mi cache
                                            pTc->cache[i][bloqueCache] =  pTmY->memory[i][numBloque%8];
                                        }
                                    }else{
                                        //libero directorio y cache
                                        pthread_mutex_unlock(&mutDir2);
                                        pthread_mutex_unlock(&mutCache);
                                        return false;
                                    }
                                }
                                break;
                            case CPU1:
                                if(pTdY->directory[posDirectorio][2] == 1){ //CPU0 lo tiene modificado?
                                    if(pthread_mutex_trylock(&mutCache)){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmY, pTcX, numBloque);
                                        pTcX->cache[5][bloqueCache] = C;
                                        pTdY->directory[posDirectorio][1] = C;
                                        pTdY->directory[posDirectorio][3] = 1;
                                        pthread_mutex_unlock(&mutCache);
                                        for(int i=0; i<4; ++i){                        // Subo el bloque de la memoria remota 2 a mi cache
                                            pTc->cache[i][bloqueCache] =  pTmY->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir1);
                                        pthread_mutex_unlock(&mutCache1);
                                        return false;
                                    }

                                }
                                if(pTdY->directory[posDirectorio][4] == 1){ //CPU2 lo tiene modificado?
                                    if(pthread_mutex_trylock(&mutCache2)){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmY, pTcY, numBloque);
                                        pTcY->cache[5][bloqueCache] = C;
                                        pTdY->directory[posDirectorio][1] = C;
                                        pTdY->directory[posDirectorio][3] = 1;
                                        pthread_mutex_unlock(&mutCache2);
                                        for(int i=0; i<4; ++i){                        // Subo el bloque de la memoria remota 2 a mi cache
                                            pTc->cache[i][bloqueCache] =  pTmY->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir1);
                                        pthread_mutex_unlock(&mutCache1);
                                        return false;
                                    }
                                }
                                break;
                            case CPU2:
                                if(pTdY->directory[posDirectorio][2] == 1){ //CPU0 lo tiene modificado?
                                    if(pthread_mutex_trylock(&mutCache)){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cache remota a memoria
                                        copiarAmemoria(pTmY, pTcX, numBloque);
                                        pTcX->cache[5][bloqueCache] = C;
                                        pTdY->directory[posDirectorio][1] = C;
                                        pTdY->directory[posDirectorio][4] = 1;
                                        pthread_mutex_unlock(&mutCache);
                                        for(int i=0; i<4; ++i){                        // Subo el bloque de la memoria remota 2 a mi cache
                                            pTc->cache[i][bloqueCache] =  pTmY->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir1);
                                        pthread_mutex_unlock(&mutCache2);
                                        return false;
                                    }
                                }
                                if(pTdY->directory[posDirectorio][3] == 1){ //CPU1 lo tiene modificado?
                                    if(pthread_mutex_trylock(&mutCache1)){
                                        esperaCiclos(32, idCPU);    //Dura 32 ciclos en pasar un bloque de cachce remota a memoria
                                        copiarAmemoria(pTmY, pTcY, numBloque);
                                        pTcY->cache[5][bloqueCache] = C;
                                        pTdY->directory[posDirectorio][1] = C;
                                        pTdY->directory[posDirectorio][4] = 1;
                                        pthread_mutex_unlock(&mutCache1);
                                        for(int i=0; i<4; ++i){                        // Subo el bloque de la memoria remota 2 a mi cache
                                            pTc->cache[i][bloqueCache] =  pTmY->memory[i][numBloque%8];
                                        }
                                    }else{
                                        pthread_mutex_unlock(&mutDir1);
                                        pthread_mutex_unlock(&mutCache2);
                                        return false;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    // Libera el directorio remoto 2
                    switch(idCPU){
                    case CPU0:
                        pthread_mutex_unlock(&mutDir2);
                        break;
                    case CPU1:
                        pthread_mutex_unlock(&mutDir1);
                        break;
                    case CPU2:
                        pthread_mutex_unlock(&mutDir1);
                        break;
                    }
                }else{  //no me dieron el bloqueo del directorio remoto 2, libero la cache
                    switch(idCPU){
                    case CPU0:
                        pthread_mutex_unlock(&mutCache);
                        break;
                    case CPU1:
                        pthread_mutex_unlock(&mutCache1);
                        break;
                    case CPU2:
                        pthread_mutex_unlock(&mutCache2);
                        break;
                    }
                    return false;

                }
            }
        }
        esperaCiclos(1, idCPU); //Tiene que esperar 1 ciclo para hacer la lectura
        vecRegs[regX] = pTc->cache[filaCache][bloqueCache];         // Lo leo (lo pongo en el registro)

        // Libero mi cache, es el ultimo recurso que se libera
        switch(idCPU){
        case CPU0:
            pthread_mutex_unlock(&mutCache);
            break;
        case CPU1:
            pthread_mutex_unlock(&mutCache1);
            break;
        case CPU2:
            pthread_mutex_unlock(&mutCache2);
            break;
        }
    }else{
        // No pude obtener el bloqueo sobre mi cache
        // devuelvo false ya que no puedo hacer nada.
        return false;
    }
    return false;
}

void* principalThread::procesador(int id, int pc, int idCPU, int cicloInicio)
{
    /* Se crea un puntero para cada estructura que permita al metodo a llamar, identificar
    cual memoria-cache-directorio de CPU debe utilizar de forma local-externa */


    sMemory     *pMemory;                       /* Estructuras locales */
    sCach       *pCache;
    sDirectory  *pDirect;

    sMemory     *pMemoryX;                      /* Estructuras externas */
    sCach       *pCacheX;
    sDirectory  *pDirectX;

    sMemory     *pMemoryY;
    sCach       *pCacheY;
    sDirectory  *pDirectY;

    int registros[32];                              /* Los registros de cada procesador.*/
    for(int i=0; i<32; ++i){
        registros[i] = 0;
    }
    int IP = pc;                                     /* IP = Instruction pointer */
    int idHilo = id;

    /* Asignacion de punteros locales y externos */

    switch(idCPU){
    case CPU0:
        pMemory = &sMem;                             /* CPU 0 Local */
        pCache = &sCache;
        pDirect = &sDirect;
        pMemoryX = &sMem1;                           /* CPU's Externos */
        pCacheX = &sCache1;
        pDirectX = &sDirect1;
        pMemoryY = &sMem2;
        pCacheY = &sCache2;
        pDirectY = &sDirect2;
        break;

    case CPU1:
        pMemory = &sMem1;                             /* CPU 1 Local */
        pCache = &sCache1;
        pDirect = &sDirect1;
        pMemoryX = &sMem;                             /* CPU's Externos */
        pCacheX = &sCache;
        pDirectX = &sDirect;
        pMemoryY = &sMem2;
        pCacheY = &sCache2;
        pDirectY = &sDirect2;
        break;

    case CPU2:
        pMemory = &sMem2;                             /* CPU 2 Local */
        pCache = &sCache2;
        pDirect = &sDirect2;
        pMemoryX = &sMem;                             /* CPU's Externos */
        pCacheX = &sCache;
        pDirectX = &sDirect;
        pMemoryY = &sMem1;
        pCacheY = &sCache1;
        pDirectY = &sDirect1;
        break;
    }

    while(vecPrograma[IP] != FIN){                         // Mientras no encuentre una instruccion de finalizacion

        qDebug()<<"Soy el CPU "<<idCPU<<" y estoy corriendo el hilo "<<idHilo;

        int IR[4];  //IR = instruction register
        IR[0] = vecPrograma[IP];       //Codigo de instruccion
        IR[1] = vecPrograma[IP+1];     //Primer parametro
        IR[2] = vecPrograma[IP+2];     //Segundo parametro
        IR[3] = vecPrograma[IP+3];     //Tercer parametro

        IP += 4;    //Salta a la siguiente instruccion.

        switch(IR[0]){
        case DADDI:
            registros[IR[2]] = registros[IR[1]] + IR[3];                //Rx <- Ry + n       |  Las tres instrucciones
            esperaCiclos(1, idCPU);
            break;
        case DADD:
            registros[IR[3]] = registros[IR[1]] + registros[IR[2]];     //Rx <- Ry + Rz      |  tardan un ciclo de reloj
            esperaCambioCiclo(idCPU);
            break;
        case DSUB:
            registros[IR[3]] = registros[IR[1]] - registros[IR[2]];     //Rx <- Ry - Rz      |  cada una.
            esperaCiclos(1, idCPU);
            break;
        case LW:
            while(lw(IR[2], IR[1], IR[3], registros, pMemory, pCache, pDirect, pMemoryX, pCacheX, pDirectX, pMemoryY, pCacheY, pDirectY, idCPU) == false) {
                esperaCiclos(1, idCPU);
            }         //Rx <- M(n + (Ry))
            break;
        case SW:
            while(sw(IR[2], IR[1], IR[3], registros, pMemory, pCache, pDirect, pMemoryX, pCacheX, pDirectX, pMemoryY, pCacheY, pDirectY, idCPU)==false){
                esperaCiclos(1, idCPU);
            };           //M(n + (Ry)) <- Rx
            break;
        case BEQZ:
            if(registros[IR[1]] == 0){                                  //Rx = 0, salta
                IP += (IR[3])*4;
            }
            esperaCiclos(1, idCPU);
            break;
        case BNEZ:
            if(registros[IR[1]] != 0){                                  //Rx != 0, salta
                IP += (IR[3])*4;
            }
            esperaCiclos(1, idCPU);
            break;
        }
    }
    if(vecPrograma[IP] == FIN){
        fin(idHilo, registros, idCPU, cicloInicio);
    }
}

void *principalThread::procesadorHelper(void *threadStruct)
{
    struct threadData *td;
    td = (struct threadData*)threadStruct;
    return static_cast<principalThread*>(td->ptr)->procesador(td->idThread, td->numPC, td->idCPU, td->cicloInicio);
}

QString principalThread::controlador()
{
    struct threadData tD0;
    struct threadData tD1;
    struct threadData tD2;
    pthread_t vecThreads[4];    //vector de threads para los procesadores
    pthread_create(&vecThreads[0], NULL, cambiaCiclo, (void*)idThread);    // Hilo que controla el reloj
    ++idThread;
    if(numThreads >= 3){
        tD0.numPC = getCurrentPC();
        tD0.idThread = idThread;
        tD0.idCPU = CPU0;
        tD0.cicloInicio = reloj;
        pthread_create(&vecThreads[idThread], NULL, procesadorHelper, (void*)&tD0);     // Esto lo
        ++idThread;
        tD1.numPC = getCurrentPC();
        tD1.idThread = idThread;
        tD1.idCPU = CPU1;
        tD1.cicloInicio = reloj;
        pthread_create(&vecThreads[idThread], NULL, procesadorHelper, (void*)&tD1);     // hace la
        ++idThread;
        tD2.numPC = getCurrentPC();
        tD2.idThread = idThread;
        tD2.idCPU = CPU2;
        tD2.cicloInicio = reloj;
        pthread_create(&vecThreads[idThread], NULL, procesadorHelper, (void*)&tD2);     // primera vez.
        ++idThread;

        pthread_join(vecThreads[0], NULL);       // Espera a que
        pthread_join(vecThreads[1], NULL);      // todos los hilos
        pthread_join(vecThreads[2], NULL);      // terminen de correr.
    }


    return estadisticas;
}

void* principalThread::cambiaCiclo(void *idThread)
{
    long threadID = (long)idThread;
    qDebug()<<"Soy el hilo: "<<threadID<<" que controlo los ciclos de reloj";
    while(true){
        sem_wait(&semReloj);
        ++reloj;
        qDebug()<<"Ciclo: "<<reloj;
        pthread_mutex_unlock(&mutCPU0);
        pthread_mutex_unlock(&mutCPU1);
        pthread_mutex_unlock(&mutCPU2);
    }
}

void principalThread::esperaCambioCiclo(int idCPU)
{
    sem_post(&semReloj);
    switch(idCPU){
    case CPU0:
        pthread_mutex_lock(&mutCPU0);
        break;
    case CPU1:
        pthread_mutex_lock(&mutCPU1);
        break;
    case CPU2:
        pthread_mutex_lock(&mutCPU2);
        break;
    }
}

void principalThread::esperaCiclos(int numCiclos, int idCPU)
{
    int relojTemp = reloj + numCiclos;
    while(reloj < relojTemp){
        esperaCambioCiclo(idCPU);
    }
}

void principalThread::uncachPage(sDirectory* directorio, int bloqueInvalidar)
{
    int fila = bloqueInvalidar%8;
    if(directorio->directory[fila][0] == bloqueInvalidar){
        directorio->directory[fila][1] = U; //lo pone uncached
        directorio->directory[fila][2] = 0;
        directorio->directory[fila][3] = 0;
        directorio->directory[fila][4] = 0;
    }
}

void principalThread::copiarAmemoria(sMemory *memoria, sCach *cache, int bloqueReemplazar)
{
    for(int i=0; i<4; ++i){
        memoria->memory[i][bloqueReemplazar%8] = cache->cache[i][bloqueReemplazar%4];
    }
}


bool principalThread::sw(int regX, int regY, int n, int *vecRegs, sMemory *pTm, sCach *pTc, sDirectory *pTd, sMemory *pTmX, sCach *pTcX, sDirectory *pTdX, sMemory *pTmY, sCach *pTcY, sDirectory *pTdY, int idCPU){         /* Funcion que realiza el store */

    int dirPrev = n + vecRegs[regY];
    int numBloque = dirPrev / 16;                                                               /* Se obtiene el numero del bloque a buscar en cache */
    int bloqueCache = numBloque % 4;                                                            /* Posicion en cache que debe tomar el bloque */
    bool vacio = true;
    int contador = 0;

    int resultBlockCache;                                                                     // Banderas de verificacion que indican si el recurso fue bloqueado de forma exitosa
    int resultBlockDirect;
    int resultBlockCacheX;
    int resultBlockDirectX;
    int resultBlockCacheY;
    int resultBlockDirectY;

    int tempReloj;                                                                              // Variable temporal que espera que se cumplan los ciclos de reloj necesarios para usar un recurso


    /* Ciclo de verificacion en cache local */

    switch (idCPU) {                                                                                 // Bloqueo de la cache local
    case CPU0:
        resultBlockCache = pthread_mutex_trylock(&mutCache);
        break;
    case CPU1:
        resultBlockCache = pthread_mutex_trylock(&mutCache1);
        break;
    case CPU2:
        resultBlockCache = pthread_mutex_trylock(&mutCache2);
        break;
    }

    while(vacio && contador < 4){

        /* CASO #1 BLOQUE EN CACHE LOCAL EN ESTADO: M */

        if(resultBlockCache == 0){                                                                       // Recurso bloqueado de forma exitosa
            if(pTc->cache[4][contador] == numBloque && pTc->cache[5][contador] == M){                    // Se verifica si se encuentra en cache local estado M
                vacio = false;
                pTc->cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];                                 /* Se almacena el contenido del registro en la posicion de la cache */
                pTc->cache[5][bloqueCache] = M;

                switch(idCPU){                                                                          // Libera la cache local
                case CPU0:
                    pthread_mutex_unlock(&mutCache);
                    break;
                case CPU1:
                    pthread_mutex_unlock(&mutCache1);
                    break;
                case CPU2:
                    pthread_mutex_unlock(&mutCache2);
                    break;
                }
                return true;
            }
        }else{
            return false;
        }



        /* CASO #2 BLOQUE EN CACHE LOCAL EN ESTADO: C  */


        if(resultBlockCache == 0){
            if(pTc->cache[4][contador] == numBloque && pTc->cache[5][contador] == C){                     /* El bloque esta en cache local compartido */

                /* Se busca el directorio donde se encuentra el bloque a modificar, se cambia su estado */

                bool continuar = true;
                if(numBloque < 8){								                                                            // Bloque a modificar en el directorio local
                    resultBlockDirect = pthread_mutex_trylock(&mutDir);
                    if(resultBlockDirect == 0){
                        tempReloj = reloj + 2;                  // Directorio local tarda 2 ciclos
                        while(reloj < tempReloj){
                            esperaCambioCiclo(idCPU);
                        }
                        for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio local */
                            if(pTd->directory[i][0] == numBloque){
                                bool recorrer = true;
                                switch(idCPU){
                                case CPU0:
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU1:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU2:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    break;
                                }

                                pTd->directory[i][1] = M;
                                switch(idCPU){
                                case CPU0:
                                    pTd->directory[i][2] = 1;
                                    pTd->directory[i][3] = 0;
                                    pTd->directory[i][4] = 0;
                                    break;
                                case CPU1:
                                    pTd->directory[i][2] = 0;
                                    pTd->directory[i][3] = 1;
                                    pTd->directory[i][4] = 0;
                                    break;
                                case CPU2:
                                    pTd->directory[i][2] = 0;
                                    pTd->directory[i][3] = 0;
                                    pTd->directory[i][4] = 1;
                                    break;
                                }
                                continuar = false;
                            }
                        }
                        pthread_mutex_unlock(&mutDir);
                    }else{
                        return false;
                    }
                }

                if(numBloque > 7 && numBloque < 16 ){                            					    // Bloque a reemplazar en el directorio externo X
                    resultBlockDirectX = pthread_mutex_trylock(&mutDir1);
                    if(resultBlockDirectX == 0){
                        tempReloj = reloj + 4;                  // Directorio externo tarda 4 ciclos
                        while(reloj < tempReloj){
                            esperaCambioCiclo(idCPU);
                        }
                        for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio X */
                            if(pTdX->directory[i][0] == numBloque){
                                bool recorrer = true;
                                switch(idCPU){
                                case CPU0:
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU1:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU2:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    break;
                                }
                                continuar = false;

                                pTdX->directory[i][1] = M;
                                switch(idCPU){
                                case CPU0:
                                    pTdX->directory[i][2] = 1;
                                    pTdX->directory[i][3] = 0;
                                    pTdX->directory[i][4] = 0;
                                    break;
                                case CPU1:
                                    pTdX->directory[i][2] = 0;
                                    pTdX->directory[i][3] = 1;
                                    pTdX->directory[i][4] = 0;
                                    break;
                                case CPU2:
                                    pTdX->directory[i][2] = 0;
                                    pTdX->directory[i][3] = 0;
                                    pTdX->directory[i][4] = 1;
                                    break;
                                }
                            }
                        }
                        pthread_mutex_unlock(&mutDir1);
                    }else{
                        return false;
                    }
                }

                if(numBloque > 15){                                                            // Bloque a reemplazar en el directorio externo Y
                    resultBlockDirectY = pthread_mutex_trylock(&mutDir2);
                    if(resultBlockDirectY == 0){
                        tempReloj = reloj + 4;                  // Directorio remoto tarda 4 ciclos
                        while(reloj < tempReloj){
                            esperaCambioCiclo(idCPU);
                        }
                        for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio Y */
                            if(pTdY->directory[i][0] == numBloque){
                                bool recorrer = true;
                                switch(idCPU){
                                case CPU0:
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU1:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU2:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        tempReloj = reloj + 1;                  // Invalidacion de bloque en cache, tarda 1 ciclo
                                        while(reloj < tempReloj){
                                            esperaCambioCiclo(idCPU);
                                        }
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    break;
                                }
                                continuar = false;

                                pTdY->directory[i][1] = M;
                                switch(idCPU){
                                case CPU0:
                                    pTdY->directory[i][2] = 1;
                                    pTdY->directory[i][3] = 0;
                                    pTdY->directory[i][4] = 0;
                                    break;
                                case CPU1:
                                    pTdY->directory[i][2] = 0;
                                    pTdY->directory[i][3] = 1;
                                    pTdY->directory[i][4] = 0;
                                    break;
                                case CPU2:
                                    pTdY->directory[i][2] = 0;
                                    pTdY->directory[i][3] = 0;
                                    pTdY->directory[i][4] = 1;
                                    break;
                                }
                            }
                        }
                        pthread_mutex_unlock(&mutDir2);
                    }else{
                        return false;
                    }
                }

                pTc->cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];                               /* Realizadas las comprobaciones, se almacena el contenido del registro en la posicion de la cache */
                pTc->cache[5][bloqueCache] = M;

                switch(idCPU){                                                                          // Libera la cache local
                case CPU0:
                    pthread_mutex_unlock(&mutCache);
                    break;
                case CPU1:
                    pthread_mutex_unlock(&mutCache1);
                    break;
                case CPU2:
                    pthread_mutex_unlock(&mutCache2);
                    break;
                }
                return true;
            }
        }else{
            return false;                                                                           // No se logra bloquear la cache local
        }

        ++contador;
    }



    /* LLegado a este punto, el bloque no se encuentra en cache local */



    /* El bloque que se encuentra en la posicion donde va a caer el nuevo bloque, dependiendo de su estado, se almacena en memoria o no */

    if(resultBlockCache == 0){                                                                      // Se logra bloquear la cache local
        int numBloqueReemplazar = pTc->cache[4][bloqueCache];
        if(pTc->cache[5][bloqueCache] != I){                                                         // El bloque debe modificarse en los directorios, estado C o M

            bool modificado = false;

            if(pTc->cache[5][bloqueCache] == M){                                                            /* Si esta modificado, debe guardarse en memoria */
                modificado = true;
                pTc->cache[5][bloqueCache] = I;                                              // Invalida en cache local
            }
            if(numBloqueReemplazar < 8){                                                            // Bloque a reemplazar en el primer directorio
                resultBlockDirect = pthread_mutex_trylock(&mutDir);
                bool continuar = true;
                if(resultBlockDirect == 0){
                    esperaCiclos(2, idCPU);                                                 // Directorio local tarda 2 ciclos
                    for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio local */
                        if(pTd->directory[i][0] == numBloqueReemplazar){                                        /* Se coloca el estado de los CPU en 0 */
                            if(pTc->cache[5][bloqueCache] == C && !modificado){                                 /* Si esta compartido, debe invalidarse el bloque en las otras caches */
                                bool recorrer = true;
                                switch(idCPU){
                                case CPU0:
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        esperaCiclos(1, idCPU);                             // Invalidacion bloque en cache, tarda 2 ciclos
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloqueReemplazar && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        esperaCiclos(1, idCPU);                             // Invalidacion bloque en cache, tarda 2 ciclos
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloqueReemplazar && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU1:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        esperaCiclos(1, idCPU);                             // Invalidacion bloque en cache, tarda 2 ciclos
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloqueReemplazar && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        esperaCiclos(1, idCPU);                             // Invalidacion bloque en cache, tarda 2 ciclos
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloqueReemplazar && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU2:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        esperaCiclos(1, idCPU);                             // Invalidacion bloque en cache, tarda 2 ciclos
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloqueReemplazar && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        esperaCiclos(1, idCPU);                             // Invalidacion bloque en cache, tarda 2 ciclos
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloqueReemplazar && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    break;
                                }

                            }
                            pTd->directory[i][1] = U;
                            pTd->directory[i][2] = 0;
                            pTd->directory[i][3] = 0;
                            pTd->directory[i][4] = 0;

                            continuar = false;
                        }
                    }
                    if(modificado){                                                                  // Se guarda el bloque en memoria, el directorio bloqueado permite el uso de la memoria
                        esperaCiclos(16, idCPU);                                                     // Escritura a memoria local
                        copiarAmemoria(pTc, bloqueCache, pTm, pTmX, pTmY);                           // Bloque almacenado en memoria
                    }
                    pthread_mutex_unlock(&mutDir);
                }else{
                    return false;
                }
            }

            if(numBloqueReemplazar > 7 && numBloqueReemplazar < 16 ){                                // Bloque a reemplazar en el directorio externo X
                resultBlockDirectX = pthread_mutex_trylock(&mutDir1);
                bool continuar = true;
                if(resultBlockDirectX == 0){
                    esperaCiclos(4, idCPU);                                                         // Directorio externo tarda 4 ciclos
                    for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio X */
                        if(pTdX->directory[i][0] == numBloqueReemplazar){                                       /* Se coloca el estado de los CPU en 0 */
                            if(pTc->cache[5][bloqueCache] == C && !modificado){                                 /* Si esta compartido, debe invalidarse el bloque en las otras caches */
                                bool recorrer = true;
                                switch(idCPU){
                                case CPU0:
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        esperaCiclos(1, idCPU);                                     // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloqueReemplazar && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        esperaCiclos(1, idCPU);                                     // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloqueReemplazar && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU1:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        esperaCiclos(1, idCPU);                                     // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloqueReemplazar && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        esperaCiclos(1, idCPU);                                     // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloqueReemplazar && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU2:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        esperaCiclos(1, idCPU);                                     // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloqueReemplazar && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        esperaCiclos(1, idCPU);                                     // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloqueReemplazar && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    break;
                                }

                            }
                            pTdX->directory[i][1] = U;
                            pTdX->directory[i][2] = 0;
                            pTdX->directory[i][3] = 0;
                            pTdX->directory[i][4] = 0;

                            continuar = false;
                        }
                    }
                    if(modificado){                                                                  // Se guarda el bloque en memoria, el directorio bloqueado permite el uso de la memoria
                        esperaCiclos(32, idCPU);                                                     // Escritura de bloque a memoria externa tarda 32 ciclos
                        copiarAmemoria(pTc, bloqueCache, pTm, pTmX, pTmY);                           // Bloque almacenado en memoria
                    }
                    pthread_mutex_unlock(&mutDir1);
                }else{
                    return false;
                }
            }

            if(numBloqueReemplazar > 15){                                                            // Bloque a reemplazar en el directorio externo Y
                resultBlockDirectY = pthread_mutex_trylock(&mutDir2);
                bool continuar = true;
                if(resultBlockDirectY == 0){
                    esperaCiclos(4, idCPU);                                                          // Directorio remoto tarda 4 ciclos
                    for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio Y */
                        if(pTdY->directory[i][0] == numBloqueReemplazar){                                       /* Se coloca el estado de los CPU en 0 */

                            if(pTc->cache[5][bloqueCache] == C && !modificado){                                 /* Si esta compartido, debe invalidarse el bloque en las otras caches */
                                bool recorrer = true;
                                switch(idCPU){
                                case CPU0:
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        esperaCiclos(1, idCPU);                                                 // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloqueReemplazar && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        esperaCiclos(1, idCPU);                                                 // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloqueReemplazar && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU1:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        esperaCiclos(1, idCPU);                                                 // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloqueReemplazar && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
                                    if(resultBlockCacheY == 0){
                                        esperaCiclos(1, idCPU);                                                 // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloqueReemplazar && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache2);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 2
                                    }
                                    break;

                                case CPU2:
                                    resultBlockCache = pthread_mutex_trylock(&mutCache);
                                    if(resultBlockCache == 0){
                                        esperaCiclos(1, idCPU);                                                 // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcX->cache[4][j] == numBloqueReemplazar && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcX->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 0
                                    }
                                    resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
                                    if(resultBlockCacheX == 0){
                                        esperaCiclos(1, idCPU);                                                 // Invalidacion de bloque en cache, tarda 1 ciclo
                                        for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                                            if(pTcY->cache[4][j] == numBloqueReemplazar && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                                                pTcY->cache[5][j] = I;
                                                recorrer = false;
                                            }
                                        }

                                        pthread_mutex_unlock(&mutCache1);

                                    }else{
                                        return false;    // No se logra bloquear la cache en CPU 1
                                    }
                                    break;
                                }

                            }

                            continuar = false;

                            pTdY->directory[i][1] = U;
                            pTdY->directory[i][2] = 0;
                            pTdY->directory[i][3] = 0;
                            pTdY->directory[i][4] = 0;
                        }
                    }
                    if(modificado){                                                                  // Se guarda el bloque en memoria, el directorio bloqueado permite el uso de la memoria
                        esperaCiclos(32, idCPU);                                                     // Se copia el bloque a memoria externa, tarda 32 ciclos
                        copiarAmemoria(pTc, bloqueCache, pTm, pTmX, pTmY);                           // Bloque almacenado en memoria
                    }
                    pthread_mutex_unlock(&mutDir2);
                }else{
                    return false;
                }
            }
        }
    }else{
        return false;                                                                               // No se logra bloquear la cache local
    }



    /* Se busca el directorio donde se encuentra el bloque a modificar, se cambia su estado */

    if(numBloque < 8){								                                                            // Bloque a modificar en el directorio local
        resultBlockDirect = pthread_mutex_trylock(&mutDir);
        bool continuar = true;
        if(resultBlockDirect == 0){
            esperaCiclos(2, idCPU);                                                                             // Directorio local tarda 2 ciclos
            for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio local */
                if(pTd->directory[i][0] == numBloque){
                    pTd->directory[i][1] = M;
                    switch(idCPU){
                    case CPU0:
                        pTd->directory[i][2] = 1;
                        pTd->directory[i][3] = 0;
                        pTd->directory[i][4] = 0;
                        break;
                    case CPU1:
                        pTd->directory[i][2] = 0;
                        pTd->directory[i][3] = 1;
                        pTd->directory[i][4] = 0;
                        break;
                    case CPU2:
                        pTd->directory[i][2] = 0;
                        pTd->directory[i][3] = 0;
                        pTd->directory[i][4] = 1;
                        break;
                    }
                    continuar = false;
                }
            }
            pthread_mutex_unlock(&mutDir);
        }else{
            return false;
        }
    }

    if(numBloque > 7 && numBloque < 16 ){                            					    // Bloque a reemplazar en el directorio externo X
        resultBlockDirectX = pthread_mutex_trylock(&mutDir1);
        bool continuar = true;
        if(resultBlockDirectX == 0){
            esperaCiclos(4, idCPU);                                                         // Directorio remoto tarda 4 ciclos
            for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio X */
                if(pTdX->directory[i][0] == numBloque){
                    pTdX->directory[i][1] = M;
                    switch(idCPU){
                    case CPU0:
                        pTdX->directory[i][2] = 1;
                        pTdX->directory[i][3] = 0;
                        pTdX->directory[i][4] = 0;
                        break;
                    case CPU1:
                        pTdX->directory[i][2] = 0;
                        pTdX->directory[i][3] = 1;
                        pTdX->directory[i][4] = 0;
                        break;
                    case CPU2:
                        pTdX->directory[i][2] = 0;
                        pTdX->directory[i][3] = 0;
                        pTdX->directory[i][4] = 1;
                        break;
                    }
                    continuar = false;
                }
            }
            pthread_mutex_unlock(&mutDir1);
        }else{
            return false;
        }
    }

    if(numBloque > 15){                                                            // Bloque a reemplazar en el directorio externo Y
        resultBlockDirectY = pthread_mutex_trylock(&mutDir2);
        bool continuar = true;
        if(resultBlockDirectY == 0){
            esperaCiclos(4, idCPU);                                                // Directorio remoto tarda 4 ciclos
            for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio Y */
                if(pTdY->directory[i][0] == numBloque){
                    pTdY->directory[i][1] = M;
                    switch(idCPU){
                    case CPU0:
                        pTdY->directory[i][2] = 1;
                        pTdY->directory[i][3] = 0;
                        pTdY->directory[i][4] = 0;
                        break;
                    case CPU1:
                        pTdY->directory[i][2] = 0;
                        pTdY->directory[i][3] = 1;
                        pTdY->directory[i][4] = 0;
                        break;
                    case CPU2:
                        pTdY->directory[i][2] = 0;
                        pTdY->directory[i][3] = 0;
                        pTdY->directory[i][4] = 1;
                        break;
                    }
                    continuar = false;
                }
            }
            pthread_mutex_unlock(&mutDir2);
        }else{
            return false;
        }
    }


    /* CASO # 3 BLOQUE EN CACHE EXTERNA */

    bool recorrer = true;
    bool encontrado = false;

    switch(idCPU){                                                                              /* Switch encargado de verificar el bloque en las caches externas,
                                                                                                          invalidar el bloque en estas, y copiarlo a memoria si fue modificado */
    case CPU0:
        resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
        if(resultBlockCacheX == 0){
            for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                    // El bloque esta compartido en los CPU externos, se invalida
                    esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                    pTcX->cache[5][j] = I;
                    encontrado = true;
                    recorrer = false;
                }else{
                    if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == M){                 // El bloque esta compartido en el CPU externo X , el bloque esta modificado
                        esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                        esperaCiclos(32, idCPU);                                                 // Escritura de bloque a memoria remota, tarda 32 ciclos
                        copiarAmemoria(pTcX, j, pTm, pTmX, pTmY);                                // Se almacena en memoria
                        pTcX->cache[5][j] = I;
                        encontrado = true;
                        recorrer = false;
                    }
                }
            }

            pthread_mutex_unlock(&mutCache1);

        }else{
            return false;    // No se logra bloquear la cache en CPU 1
        }
        resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
        if(resultBlockCacheY == 0){
            for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                    esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                    pTcY->cache[5][j] = I;
                    encontrado = true;
                    recorrer = false;
                }else{
                    if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == M){                // El bloque esta compartido en el CPU externo Y, el bloque esta modificado
                        esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                        esperaCiclos(32, idCPU);                                                 // Escritura de bloque a memoria remota, tarda 32 ciclos
                        copiarAmemoria(pTcY, j, pTm, pTmX, pTmY);                               // Se almacena en memoria
                        pTcY->cache[5][j] = I;
                        encontrado = true;
                        recorrer = false;
                    }
                }
            }

            pthread_mutex_unlock(&mutCache2);

        }else{
            return false;    // No se logra bloquear la cache en CPU 2
        }
        break;

    case CPU1:
        resultBlockCache = pthread_mutex_trylock(&mutCache);
        if(resultBlockCache == 0){
            for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                    esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                    pTcX->cache[5][j] = I;
                    encontrado = true;
                    recorrer = false;
                }else{
                    if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == M){                 // El bloque esta compartido en el CPU externo X , el bloque esta modificado
                        esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                        esperaCiclos(32, idCPU);                                                 // Escritura de bloque a memoria remota, tarda 32 ciclos
                        copiarAmemoria(pTcX, j, pTm, pTmX, pTmY);                                // Se almacena en memoria
                        pTcX->cache[5][j] = I;
                        encontrado = true;
                        recorrer = false;
                    }
                }
            }

            pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

        }else{
            return false;    // No se logra bloquear la cache en CPU 0
        }
        resultBlockCacheY = pthread_mutex_trylock(&mutCache2);
        if(resultBlockCacheY == 0){
            for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                    esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                    pTcY->cache[5][j] = I;
                    encontrado = true;
                    recorrer = false;
                }else{
                    if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == M){                // El bloque esta compartido en el CPU externo Y, el bloque esta modificado
                        esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                        esperaCiclos(32, idCPU);                                                 // Escritura de bloque a memoria remota, tarda 32 ciclos
                        copiarAmemoria(pTcY, j, pTm, pTmX, pTmY);                               // Se almacena en memoria
                        pTcY->cache[5][j] = I;
                        encontrado = true;
                        recorrer = false;
                    }
                }
            }

            pthread_mutex_unlock(&mutCache2);

        }else{
            return false;    // No se logra bloquear la cache en CPU 2
        }
        break;

    case CPU2:
        resultBlockCache = pthread_mutex_trylock(&mutCache);
        if(resultBlockCache == 0){
            for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                    esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                    pTcX->cache[5][j] = I;
                    encontrado = true;
                    recorrer = false;
                }else{
                    if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] == M){                 // El bloque esta compartido en el CPU externo X , el bloque esta modificado
                        esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                        esperaCiclos(32, idCPU);                                                 // Escritura de bloque a memoria remota, tarda 32 ciclos
                        copiarAmemoria(pTcX, j, pTm, pTmX, pTmY);                                // Se almacena en memoria
                        pTcX->cache[5][j] = I;
                        encontrado = true;
                        recorrer = false;
                    }
                }
            }

            pthread_mutex_unlock(&mutCache);                                                     // Se liberan las caches

        }else{
            return false;    // No se logra bloquear la cache en CPU 0
        }
        resultBlockCacheX = pthread_mutex_trylock(&mutCache1);
        if(resultBlockCacheX == 0){
            for(int j = 0; j < 4 && recorrer; ++j){                                              // Se modifica el estado en las caches
                if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == C){                     // El bloque esta compartido en los CPU externos, se invalida
                    esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                    pTcY->cache[5][j] = I;
                    encontrado = true;
                    recorrer = false;
                }else{
                    if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] == M){                // El bloque esta compartido en el CPU externo Y, el bloque esta modificado
                        esperaCiclos(1, idCPU);                                                      // Invalidacion de bloque, tarda 1 ciclo
                        esperaCiclos(32, idCPU);                                                 // Escritura de bloque a memoria remota, tarda 32 ciclos
                        copiarAmemoria(pTcY, j, pTm, pTmX, pTmY);                               // Se almacena en memoria
                        pTcY->cache[5][j] = I;
                        recorrer = false;
                        encontrado = true;
                    }
                }
            }

            pthread_mutex_unlock(&mutCache1);

        }else{
            return false;    // No se logra bloquear la cache en CPU 1
        }
        break;
    }

    if(encontrado){                                                                                 /* El bloque se encontraba en una cache externa
                                                                                                              y fue subido a cache local de memoria */
        esperaCiclos(32, idCPU);                                                                    // Escritura de memoria remota a cache local, tarda 32 ciclos
        copiarAcache(pTc, bloqueCache, numBloque, pTm, pTmX, pTmY);                                 // Se sube el bloque a cache local
        pTc->cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];
        pTc->cache[5][bloqueCache] = M;

        switch(idCPU){                                                                          // Libera la cache local
        case CPU0:
            pthread_mutex_unlock(&mutCache);
            break;
        case CPU1:
            pthread_mutex_unlock(&mutCache1);
            break;
        case CPU2:
            pthread_mutex_unlock(&mutCache2);
            break;
        }

        return true;
    }




    /* LLegado a este punto, el bloque no se encuentra en ninguna cache, ya el directorio donde esta el bloque fue modificado */



    /* CASO #4 BLOQUE EN NINGUNA CACHE */

    if(numBloque < 8){                                                                          // Revision de directorios
        resultBlockDirect = pthread_mutex_trylock(&mutDir);
        if(resultBlockDirect == 0){                                                             // Debe bloquearse el directorio del bloque para utilizar la memoria
            esperaCiclos(2, idCPU);                                                             // Directorio local tarda 2 ciclos
            esperaCiclos(16, idCPU);                                                             // Escritura de bloque en memoria local a cache local, tarda 16 ciclos
            copiarAcache(pTc, bloqueCache, numBloque, pTm, pTmX, pTmY);
            pTc->cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];
            pTc->cache[5][bloqueCache] = M;

            switch(idCPU){                                                                          // Libera la cache local
            case CPU0:
                pthread_mutex_unlock(&mutCache);
                break;
            case CPU1:
                pthread_mutex_unlock(&mutCache1);
                break;
            case CPU2:
                pthread_mutex_unlock(&mutCache2);
                break;
            }
            pthread_mutex_unlock(&mutDir);

            return true;
        }else{
            return false;
        }
    }
    if(numBloque > 7 && numBloque < 16){
        resultBlockDirectX = pthread_mutex_trylock(&mutDir1);
        if(resultBlockDirectX == 0){                                                             // Debe bloquearse el directorio del bloque para utilizar la memoria
            esperaCiclos(4, idCPU);                                                             // Directorio remoto tarda 4 ciclos
            esperaCiclos(32, idCPU);                                                             // Escritura de bloque en memoria remota a cache local, tarda 32 ciclos
            copiarAcache(pTc, bloqueCache, numBloque, pTm, pTmX, pTmY);
            pTc->cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];
            pTc->cache[5][bloqueCache] = M;

            switch(idCPU){                                                                          // Libera la cache local
            case CPU0:
                pthread_mutex_unlock(&mutCache);
                break;
            case CPU1:
                pthread_mutex_unlock(&mutCache1);
                break;
            case CPU2:
                pthread_mutex_unlock(&mutCache2);
                break;
            }
            pthread_mutex_unlock(&mutDir1);

            return true;
        }else{
            return false;
        }
    }
    if(numBloque > 15){
        resultBlockDirectY = pthread_mutex_trylock(&mutDir2);
        if(resultBlockDirectY == 0){                                                             // Debe bloquearse el directorio del bloque para utilizar la memoria
            esperaCiclos(4, idCPU);                                                             // Directorio remoto tarda 4 ciclos
            esperaCiclos(32, idCPU);                                                             // Escritura de bloque en memoria remota a cache local, tarda 32 ciclos
            copiarAcache(pTc, bloqueCache, numBloque, pTm, pTmX, pTmY);
            pTc->cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];
            pTc->cache[5][bloqueCache] = M;

            switch(idCPU){                                                                          // Libera la cache local
            case CPU0:
                pthread_mutex_unlock(&mutCache);
                break;
            case CPU1:
                pthread_mutex_unlock(&mutCache1);
                break;
            case CPU2:
                pthread_mutex_unlock(&mutCache2);
                break;
            }
            pthread_mutex_unlock(&mutDir2);

            return true;
        }else{
            return false;
        }
    }
}

void principalThread::copiarAcache(sCach *pointerC, int bloqueCache, int numBloque, sMemory *pointerM, sMemory *pointerMX, sMemory *pointerMY){      /* Se recibe un puntero a cache y a memoria, se copia el bloque a cache */

    int columMemoria;
    int idMemoria;

    columMemoria = numBloque % 8;                                                         // Columna del bloque en memoria

    if(numBloque < 8){                                                                    /* Se ubica la memoria donde se encuentra el bloque */
        idMemoria = 0;
    }else{
        if(numBloque > 7 && numBloque < 16){
            idMemoria = 1;
        }else{
            idMemoria = 2;
        }
    }

    switch (idMemoria) {
    case 0:
        pointerC->cache[0][bloqueCache] = pointerM->memory[0][columMemoria];            // Se copia el bloque en cache de memoria local
        pointerC->cache[1][bloqueCache] = pointerM->memory[1][columMemoria];
        pointerC->cache[2][bloqueCache] = pointerM->memory[2][columMemoria];
        pointerC->cache[3][bloqueCache] = pointerM->memory[3][columMemoria];
        break;
    case 1:
        pointerC->cache[0][bloqueCache] = pointerMX->memory[0][columMemoria];            // Se copia el bloque en cache de memoria X
        pointerC->cache[1][bloqueCache] = pointerMX->memory[1][columMemoria];
        pointerC->cache[2][bloqueCache] = pointerMX->memory[2][columMemoria];
        pointerC->cache[3][bloqueCache] = pointerMX->memory[3][columMemoria];
        break;
    case 2:
        pointerC->cache[0][bloqueCache] = pointerMY->memory[0][columMemoria];            // Se copia el bloque en cache de memoria Y
        pointerC->cache[1][bloqueCache] = pointerMY->memory[1][columMemoria];
        pointerC->cache[2][bloqueCache] = pointerMY->memory[2][columMemoria];
        pointerC->cache[3][bloqueCache] = pointerMY->memory[3][columMemoria];
        break;
    }
}

void principalThread::copiarAmemoria(sCach *pointerC, int bloqueCache, sMemory *pointerM, sMemory *pointerMX, sMemory *pointerMY){                      /* Se recibe un puntero a cache y se copia el bloque en memoria */

    int numeroBloque = pointerC->cache[4][bloqueCache];
    int columMemoria = numeroBloque % 8;
    int idMemoria;

    if(numeroBloque < 8){                                                            /* Se ubica la memoria donde se encuentra el bloque */
        idMemoria = 0;
    }else{
        if(numeroBloque > 7 && numeroBloque < 16){
            idMemoria = 1;
        }else{
            idMemoria = 2;
        }
    }

    switch (idMemoria) {
    case 0:
        pointerM->memory[0][columMemoria] = pointerC->cache[0][bloqueCache];               // Se copia el bloque de cache a la memoria local
        pointerM->memory[1][columMemoria] = pointerC->cache[1][bloqueCache];
        pointerM->memory[2][columMemoria] = pointerC->cache[2][bloqueCache];
        pointerM->memory[3][columMemoria] = pointerC->cache[3][bloqueCache];
        break;
    case 1:
        pointerMX->memory[0][columMemoria] = pointerC->cache[0][bloqueCache];               // Se copia el bloque de cache a la memoria externa X
        pointerMX->memory[1][columMemoria] = pointerC->cache[1][bloqueCache];
        pointerMX->memory[2][columMemoria] = pointerC->cache[2][bloqueCache];
        pointerMX->memory[3][columMemoria] = pointerC->cache[3][bloqueCache];
        break;
    case 2:
        pointerMY->memory[0][columMemoria] = pointerC->cache[0][bloqueCache];               // Se copia el bloque de cache a la memoria externa Y
        pointerMY->memory[1][columMemoria] = pointerC->cache[1][bloqueCache];
        pointerMY->memory[2][columMemoria] = pointerC->cache[2][bloqueCache];
        pointerMY->memory[3][columMemoria] = pointerC->cache[3][bloqueCache];
        break;
    }
}


void principalThread::fin(int idThread, int *registros, int idCPU, int cicloInicio)
{
    pthread_mutex_lock(&mutEstadisticas);
    estadisticas += " ---------- Datos del hilo "+QString::number(idThread)+" ----------\n";
    estadisticas += " * Se ejecuto en el procesador CPU"+QString::number(idCPU)+".\n";
    estadisticas += " * Empezó a correr en el ciclo "+QString::number(cicloInicio)+".\n";
    estadisticas += " * Duró corriendo "+QString::number(reloj-cicloInicio)+" ciclos de reloj.\n";
    estadisticas += " * Los registros quedaron como:\n";
    for(int i=0; i<32; ++i){
        estadisticas += "    R["+QString::number(i)+"] = "+QString::number(registros[i])+'\n';
    }
    estadisticas += " * La cache de datos al finalizar quedó como:\n";
    switch(idCPU){
    case CPU0:
        while(pthread_mutex_trylock(&mutCache) != 0){
            //sigue intentando hasta obtener el bloqueo de la cache
        }
        for(int i=0; i<4; ++i){
            QChar estado;
            switch(sCache.cache[5][i]){
            case M:
                estado = 'M';
                break;
            case I:
                estado = 'I';
                break;
            case C:
                estado = 'C';
                break;
            }
            estadisticas += "Bloque de cache numero "+QString::number(i)+" estado "+estado+" etiq: "+QString::number(sCache.cache[4][i])+'\n';
            estadisticas += "| ";
            for(int j=0; j<4; ++j){
                estadisticas += QString::number(sCache.cache[j][i]) + " | ";
            }
        }
        pthread_mutex_unlock(&mutCache);
        break;
    case CPU1:
        while(pthread_mutex_trylock(&mutCache1) != 0){
            //sigue intentando hasta obtener el bloqueo de la cache
        }
        for(int i=0; i<4; ++i){
            QChar estado;
            switch(sCache1.cache[5][i]){
            case M:
                estado = 'M';
                break;
            case I:
                estado = 'I';
                break;
            case C:
                estado = 'C';
                break;
            }
            estadisticas += "Bloque de cache numero "+QString::number(i)+" estado "+estado+" etiq: "+QString::number(sCache1.cache[4][i])+'\n';
            estadisticas += "| ";
            for(int j=0; j<4; ++j){
                estadisticas += QString::number(sCache1.cache[j][i]) + " | ";
            }
        }
        pthread_mutex_unlock(&mutCache1);
        break;
    case CPU2:
        while(pthread_mutex_trylock(&mutCache2) != 0){
            //sigue intentando hasta obtener el bloqueo de la cache
        }
        for(int i=0; i<4; ++i){
            QChar estado;
            switch(sCache2.cache[5][i]){
            case M:
                estado = 'M';
                break;
            case I:
                estado = 'I';
                break;
            case C:
                estado = 'C';
                break;
            }
            estadisticas += "Bloque de cache numero "+QString::number(i)+" estado "+estado+" etiq: "+QString::number(sCache2.cache[4][i])+'\n';
            estadisticas += "| ";
            for(int j=0; j<4; ++j){
                estadisticas += QString::number(sCache2.cache[j][i]) + " | ";
            }
        }
        pthread_mutex_unlock(&mutCache2);
        break;
    }
    pthread_mutex_unlock(&mutEstadisticas);

    if(colaPCs.empty()){                /* Verificacion sobre las instrucciones restantes por ejecutar */
        pthread_exit(NULL);             //termina la ejecucion de esa CPU
    }else{
        procesador(idThread+1, getCurrentPC(), idCPU, reloj);                                 /* El identificador del hilo a correr aumenta ya que es nuevo */
        /* Ejecucion de un nuevo hilo, sobre el CPU que finalizó */
    }

}

int principalThread::getCurrentPC()
{
    int retornar;
    if(colaPCs.empty() == false){
        retornar = colaPCs.front();
        colaPCs.pop();
    }else{
        retornar = -1;
    }
    return retornar;
}
