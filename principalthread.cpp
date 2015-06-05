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

struct sMemory{
    int memory[5][8];       // Memoria CPU
};
struct sCach{
    int cache[6][4];        // Cache CPU
};
struct sDirectory{
    int directory[8][5];    // Directory CPU
};

/* --------------------------------------------- */


sMemory sMem;                // Creacion de instancias
sCach sCache;
sDirectory sDirect;

sMemory sMem1;
sCach sCache1;
sDirectory sDirect1;

sMemory sMem2;
sCach sCache2;
sDirectory sDirect2;

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


int* vecPrograma;

QString estadisticas;
int reloj;
int contCicCPU1 = 0;                         /* Encargado de llevar el conteo de cada ciclo del reloj en el CPU 1 */
int contCicTotales = 0;                      /* Permite sincronizar que cada CPU vaya por el mismo ciclo de reloj */
/* En la segunda parte se utilizará la variable contCicTotales totalmente */

/* Mutex para los recursos críticos */
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
/*---------------------------------------------------*/

principalThread::principalThread(QString programa, int numHilos)
{
    reloj = 0;
    numThreads = numHilos;
    vecPCs = new int[numThreads];
    QString::iterator it;
    QString strTemp;
    int tamVec = 1;
    int indiceVecPCs = 1;
    vecPCs[0] = 0;
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
            vecPCs[indiceVecPCs] = j;
            ++indiceVecPCs;
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

            if( j=0 ){                                      // Llenado de columna cero y su respectivo numero de bloke
                sDirect.directory[i][j] = i;
                sDirect1.directory[i][j] = i+8;
                sDirect2.directory[i][j] = i+16;
            }else{
                if( j=1 ){                                  // LLenado de columna uno y su respectiva etiqueta de bloke
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

    /*       Directorio

          B   E   P0  P1  P2
        ---------------------
        | 0 | C | 1 |   |   |
        ---------------------
        | 1 | M | 1 | 0 | 0 |
        ---------------------
        | 2 | U |   |   |   |
        ---------------------
        | 3 |   |   |   |   |
        ---------------------
        | 4 | C | 1 | 0 | 1 |
        ---------------------
        | 5 |   |   |   |   |
        ---------------------
        | 6 |   |   |   |   |
        ---------------------
        | 7 |   |   |   |   |
        ---------------------

     */
}


principalThread::~principalThread()
{
    delete[] vecPCs;
}

bool principalThread::lw(int regX, int regY, int n, int *vecRegs, sMemory *pTm, sCach *pTc, sDirectory *pTd, sMemory *pTmX, sCach *pTcX, sDirectory *pTdX, sMemory *pTmY, sCach *pTcY, sDirectory *pTdY, int idCPU)
{    
    int dirPrev = n + vecRegs[regY];
    int numBloque = dirPrev/16;
    int bloqueCache = numBloque%4;  /* Numero del bloque a buscar en el cache*/
    int filaCache = (dirPrev%16)/4;
    
    // **************** Busco en cache local ****************

    int resultLockCache;
    switch(idCPU){
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
        if(pTc->cache[4][bloqueCache] == numBloque && pTc->cache[5][bloqueCache] != I){
            vecRegs[regX] = pTc->cache[filaCache][bloqueCache];
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
            return true;
        }else{
            if(pTc->cache[5][bloqueCache] == M){    //donde va a poner el bloque esta modificado
                // ***************** Saco el bloque de mi cache y lo pongo en memoria (la que corresponda) *****************
                int bloqueReemplazar = pTc->cache[4][bloqueCache];
                if(bloqueReemplazar <= pTd->directory[7][0]){  //esta en mi memoria
                    int resultLockDir;
                    switch(idCPU){
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
                    if(resultLockCache == 0){
                        for(int i=0; i<4; ++i){
                            pTm->memory[i][bloqueReemplazar] = pTc->cache[i][bloqueCache];  //copia el bloque de cache a memoria local
                        }
                        switch(idCPU){
                        case CPU0:
                            pthread_mutex_unlock(&mutDir);
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
                if(bloqueReemplazar >= pTdX->directory[0][0] && bloqueReemplazar <= pTdX->directory[7][0]){ //esta en memoria remota 1
                    if(pthread_mutex_trylock(&mutMem1) == 0){
                        int indiceMem = 0;
                        bool continua = true;
                        while(indiceMem<4 && continua){
                            if(pTmX->memory[4][indiceMem] == bloqueReemplazar){
                                for(int i=0; i<4; ++i){
                                    pTmX->memory[i][indiceMem] = pTc->cache[i][bloqueCache];    //copia el bloque de cache a memoria remota 1
                                }
                                continua = false;
                            }
                            ++indiceMem;
                        }
                        pthread_mutex_unlock(&mutMem1);
                    }else{
                        pthread_mutex_unlock(&mutCache);
                        return false;
                    }
                }
                if(bloqueReemplazar >= pTdY->directory[0][0] && bloqueReemplazar <= pTdY->directory[7][0]){ //esta en memoria remota 2
                    if(pthread_mutex_trylock(&mutMem2) == 0){
                        int indiceMem = 0;
                        bool continua = true;
                        while(indiceMem<4 && continua){
                            if(pTmY->memory[4][indiceMem] == bloqueReemplazar){
                                for(int i=0; i<4; ++i){
                                    pTmY->memory[i][indiceMem] = pTc->cache[i][bloqueCache];    //copia el bloque de cache a memoria remota 2
                                }
                                continua = false;
                            }
                            ++indiceMem;
                        }
                        pthread_mutex_unlock(&mutMem2);
                    }else{
                        pthread_mutex_unlock(&mutCache);
                        return false;
                    }
                }
                //==========================================================================================================
            }

            /*
        if(numBloque <= pTd->directory[7][0]){  //pertenece al directorio local?
            if(pthread_mutex_trylock(&mutDirLocal) == 0){    //obtiene el recurso
                int indiceDir = 0;
                while(indiceDir < 8){
                    if(pTd->directory[indiceDir][0] == numBloque){   //lo encuentra
                        if(pTd->directory[indiceDir][1] == U){  //nadie tiene el bloque en cache
                            //lo subo de memoria
                            if(pthread_mutex_trylock()==0){ //intenta obtener la memoria local
                                int indiceMem = 0;
                                while(indiceMem < 4){
                                    if(pTm->memory[4][indiceMem] == numBloque){

                                    }
                                    ++indiceMem;
                                }
                            }else{
                                pthread_mutex_unlock(&mutDirLocal);
                                pthread_mutex_unlock(&mutCacheLocal);
                                return false;
                            }
                        }
                    }
                    ++indiceDir;
                }
            }else{
                pthread_mutex_unlock(&mutCacheLocal);
                return false;
            }
        }
        if(numBloque >= pTdX->directory[0][0] && numBloque <= pTdX->directory[7][0]){   //pertenece al directorio remoto 1?
            if(pthread_mutex_trylock(&mutDirRemoto1) == 0){    //obtiene el recurso
                int indiceDir = 0;
                while(indiceDir < 8){
                    if(pTdX->directory[indiceDir][0] == numBloque){ //lo encuentra

                    }
                    ++indiceDir;
                }
            }else{
                pthread_mutex_unlock(&mutCacheLocal);
                return false;
            }
        }
        if(numBloque >= pTdY->directory[0][0] && numBloque <= pTdY->directory[7][0]){   //pertenece al directorio remoto 2?
            if(pthread_mutex_trylock(&mutDirRemoto2) == 0){    //obtiene el recurso
                int indiceDir = 0;
                while(indiceDir < 8){
                    if(pTdY->directory[indiceDir][0] == numBloque){ //lo encuentra

                    }
                    ++indiceDir;
                }
            }else{
                pthread_mutex_unlock(&mutCacheLocal);
                return false;
            }
        }*/
        }
    }else{
        return false;
    }



    /*
            Codigo viejo

    while(indiceCache < 4){
        if(cache[4][indiceCache] == numBloque && cache[5][bloqueCache] != I){   //Encontre el bloque en mi cache

            //--------------------------------------------------------------------------------
            //| * Tengo que revisar el estado del bloque (no se implementa en esta parte).   |
            //| * Dura 2 ciclos de reloj haciendo esto.                                      |
            //--------------------------------------------------------------------------------

            vecRegs[regX] = cache[(dirPrev%16)/4][bloqueCache]; // Pone la palabra en el registro.
            //Libero el recurso critico (la cache)
            ++contCicCPU1;                              // Corre un ciclo de reloj
            ++contCicTotales;
            return true;                        //Retorna true ya que tuvo exito con el lw.
        }
        ++indiceCache;
    }
    if(indiceCache >= 4){   //Significa que no esta el bloque a buscar en el cache

        //-------------------------------------------------------------------------------------------------------
        //| * Va a tardar 16 ciclos de reloj en hacer lo que viene abajo.                                       |
        //| * Busco en el directorio a ver quien es el dueño del bloque (no se implementa en esta parte)        |
        //| * Intento bloquear el recurso critico (mi memoria). Si no puedo entonces libero la cache tambien.   |
        //-------------------------------------------------------------------------------------------------------

        if(cache[5][bloqueCache] == M){ // El bloque al que le voy a caer encima en cache esta modificado, tengo que pasarlo a memoria. (Write back)
            int bloqueMem = cache[4][bloqueCache];
            for(int i=0; i<4; ++i){
                memory[i][bloqueMem] = cache[i][bloqueCache];   // Hago la copia del bloque modificado en cache a memoria.
            }
            contCicCPU1 += 16;              // En cada escritura se tardan 16 ciclos de reloj
            contCicTotales += 16;
        }
        for(int i=0; i<4; ++i){ //Write allocate
            cache[i][bloqueCache] = memory[i][numBloque]; // Subo el bloque de memoria a cache.
        }
        contCicCPU1 += 16;              //* 16 ciclos de reloj
        contCicTotales += 16;
        // Libero el semaforo de la memoria
        cache[4][bloqueCache] = numBloque;    // Le pone el identificador al bloque en cache.
        cache[5][bloqueCache] = C;              // Pone el estado del bloque como compartido.
        vecRegs[regX] = cache[(dirPrev%16)/4][bloqueCache];     // Pone la palabra en el registro.
        //Libero el recurso critico (la cache)
        return true;
    }*/
    //Libero el recurso critico (la cache)
    return false;
}

void* principalThread::procesador(int id, int pc, int idCPU)
{
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

        int IR[4];  //IR = instruction register
        IR[0] = vecPrograma[IP];       //Codigo de instruccion
        IR[1] = vecPrograma[IP+1];     //Primer parametro
        IR[2] = vecPrograma[IP+2];     //Segundo parametro
        IR[3] = vecPrograma[IP+3];     //Tercer parametro

        IP += 4;    //Salta a la siguiente instruccion.

        switch(IR[0]){
        case DADDI:
            registros[IR[2]] = registros[IR[1]] + IR[3];                //Rx <- Ry + n       |  Las tres instrucciones
            ++contCicCPU1;
            ++contCicTotales;
            break;
        case DADD:
            registros[IR[3]] = registros[IR[1]] + registros[IR[2]];     //Rx <- Ry + Rz      |  tardan un ciclo de reloj
            ++contCicCPU1;
            ++contCicTotales;
            break;
        case DSUB:
            registros[IR[3]] = registros[IR[1]] - registros[IR[2]];     //Rx <- Ry - Rz      |  cada una.
            ++contCicCPU1;
            ++contCicTotales;
            break;
        case LW:
            while(lw(IR[2], IR[1], IR[3], registros, pMemory, pCache, pDirect, pMemoryX, pCacheX, pDirectX, pMemoryY, pCacheY, pDirectY) == false) {

            }         //Rx <- M(n + (Ry))
            break;
        case SW:
            sw(IR[2], IR[1], IR[3], registros, pMemory, pCache, pDirect, pMemoryX, pCacheX, pDirectX, pMemoryY, pCacheY, pDirectY);           //M(n + (Ry)) <- Rx
            break;
        case BEQZ:
            if(registros[IR[1]] == 0){                                  //Rx = 0, salta
                IP += (IR[3])*4;
            }
            ++contCicCPU1;
            ++contCicTotales;
            break;
        case BNEZ:
            if(registros[IR[1]] != 0){                                  //Rx != 0, salta
                IP += (IR[3])*4;
            }
            ++contCicCPU1;
            ++contCicTotales;
            break;
        }
    }
    if(vecPrograma[IP] == FIN){
        fin(idHilo, registros);
    }

    pthread_exit(NULL);
}

void *principalThread::procesadorHelper(void *threadStruct)
{
    struct threadData *td;
    td = (struct threadData*)threadStruct;
    return static_cast<principalThread*>(td->ptr)->procesador(td->idThread, td->numPC, td->idCPU);
}

QString principalThread::controlador()
{
    int thread_0 = 0;
    int thread_1 = 1;
    int thread_2 = 2;
    struct threadData tD0;
    struct threadData tD1;
    struct threadData tD2;
    pthread_t vecThreads[3];    //vector de threads para los procesadores
    QString hiloActual = "";

    int iPCs = 0;
    int idThread = 0;
    while(iPCs<numThreads){
        if(iPCs == 0){  //es la primera vez
            tD0.numPC = vecPCs[iPCs];
            tD0.idThread = idThread;
            tD0.idCPU = iPCs;
            ++iPCs;
            ++idThread;
            tD1.numPC = vecPCs[iPCs];
            tD1.idThread = idThread;
            tD1.idCPU = iPCs;
            ++iPCs;
            ++idThread;
            tD2.numPC = vecPCs[iPCs];
            tD2.idThread = idThread;
            tD2.idCPU = iPCs;
        }

        ++iPCs;
        ++idThread;
    }
    for(int indicePCs = 0; indicePCs < numThreads; ++indicePCs){
        tD.idThread = idThread;
        tD.numPC = vecPCs[indicePCs];
        //tD.idCPU = i%3;

        hiloActual = QString::number(idThread)+" en ejecucion.";

        pthread_create(&hilo[0], NULL, procesadorHelper, (void*) &tD);

        pthread_join(hilo, NULL);
        hiloActual = QString::number(idThread)+" terminado.";

        ++idThread;
    }

    estadisticas += "===================================================\n";
    estadisticas += "*** La memoria del procesador quedo como:\n";
    for(int i=0; i<numBloquesMem; ++i){
        estadisticas += "Bloque de memoria "+QString::number(i)+'\n';
        estadisticas += "| ";
        for(int j=0; j<4; ++j){
            estadisticas += QString::number(memory[j][i])+" | ";
        }
        estadisticas += '\n';
    }
    estadisticas += "===================================================\n";

    estadisticas += "\n Cantidad total de ciclos de reloj ejecutados: "+QString::number(contCicTotales);


    return estadisticas;
}


bool principalThread::sw(int regX, int regY, int n, int *vecRegs, sMemory *pTm, sCach *pTc, sDirectory *pTd, sMemory *pTmX, sCach *pTcX, sDirectory *pTdX, sMemory *pTmY, sCach *pTcY, sDirectory *pTdY, int idCPU){         /* Funcion que realiza el store */
    
    int dirPrev = n + vecRegs[regY];
    int numBloque = dirPrev / 16;                                  /* Se obtiene el numero del bloque a buscar en cache */
    int bloqueCache = numBloque % 4;                                /* Posicion en cache que debe tomar el bloque */
    bool vacio = true;
    bool continuar = false;
    int contador = 0;


    while(vacio && contador < 4){

        /* CASO #1 BLOQUE EN CACHE LOCAL EN ESTADO: M */

        if(pTc->cache[4][contador] == numBloque && pTc->cache[5][contador] == M){                   // Se verifica si se encuentra en cache local M o C
            vacio = false;
            pTc->cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];   /* Se almacena el contenido del registro en la posicion de la cache */
            pTc->cache[5][bloqueCache] = M;
            for(int i = 0; i < 8 && continuar; ++i){                   /* Verificacion de directorio local */
                if(pTd->directory[i][0] == numBloque){
                    pTd->directory[i][1] = M;
                    pTd->directory[i][idCPU + 2] = 1;
                    continuar = false;
                }
            }
            for(int i = 0; i < 8 && continuar; ++i){                   /* Verificacion de directorio externo X */
                if(pTdX->directory[i][0] == numBloque){
                    pTdX->directory[i][1] = M;
                    pTdX->directory[i][idCPU + 2] = 1;
                    continuar = false;
                }
            }
            for(int i = 0; i < 8 && continuar; ++i){                   /* Verificacion de directorio externo Y */
                if(pTdY->directory[i][0] == numBloque){
                    pTdY->directory[i][1] = M;
                    pTdY->directory[i][idCPU + 2] = 1;
                    continuar = false;
                }
            }
            return true;
        }


        /* CASO #2 BLOQUE EN CACHE LOCAL EN ESTADO: C  */

        if(pTc->cache[4][contador] == numBloque && pTc->cache[5][contador] == C){
            vacio = false;
            bool continuar = true;
            for(int i = 0; i < 8 && continuar; ++i){                                                                  /* Busqueda en directorio local */
                if(pTd->directory[i][0] == numBloque){

                    if(pTd->directory[i][2] == 1 && ( idCPU + 2 != 2 ) ){                               // CPU 0 tiene el bloque y no es el CPU local



                    }

                    if(pTd->directory[i][3] == 1 && (idCPU + 2 != 3 ) ){                                // CPU 1 tiene el bloque y no es el CPU local

                    }

                    if(pTd->directory[i][4] == 1 && (idCPU + 2 != 4) ){                                 // CPU 2 tiene el bloque y no es el CPU local


                    }
                    continuar = false;
                }
            }



            return true;
        }



        ++contador;
    }










    // meter los casos: bloque en cache local estado compartido, bloque en cache local estado invalido



    /* CASO #4 BLOQUE EN NINGUNA CACHE EN ESTADO: U */

    continuar = true;

    for(int i = 0; i < 8 && continuar; ++i){                                                    // Busca en el directorio de CPU local el bloque
        if(pTd->directory[i][0] == numBloque && pTd->directory[i][1] == U){

            pTd->directory[i][1] = M;                                                           // Cambia su estado a modificado

            if(idCPU == 0){
                pTd->directory[i][2] = 1;                                                       // El CPU 0 lo esta utilizando
            }else{
                if(idCPU == 1){
                    pTd->directory[i][3] = 1;                                                   // El CPU 1 lo esta utilizando
                }else{
                    pTd->directory[i][4] = 1;                                                   // El CPU 2 lo esta utilizando
                }
            }
            for(int j = 0; j < 8; ++i){                                                         // Se busca el valor en memoria
                if(pTm->memory[4][j] == numBloque){
                    copiarAcache(pTc, pTm, bloqueCache, j, pTmX, pTmY);
                }
            }
            continuar = false;
            return true;
        }
    }
    for(int i = 0; i < 8 && continuar; ++i){                                                    // Busca en el directorio de CPU externo X el bloque
        if(pTdX->directory[i][0] == numBloque && pTdX->directory[i][1] == U){

            pTdX->directory[i][1] == M;

            if(idCPU == 0){
                pTdX->directory[i][2] = 1;                                                       // El CPU 0 lo esta utilizando
            }else{
                if(idCPU == 1){
                    pTdX->directory[i][3] = 1;                                                   // El CPU 1 lo esta utilizando
                }else{
                    pTdX->directory[i][4] = 1;                                                   // El CPU 2 lo esta utilizando
                }
            }
            for(int j = 0; j < 8; ++i){                                                          // Se busca el valor en memoria
                if(pTm->memory[4][j] == numBloque){
                    copiarAcache(pTc, pTmX, bloqueCache, j, pTm, pTmY);
                }
            }
            continuar = false;
            return true;
        }
    }
    for(int i = 0; i < 8 && continuar; ++i){

        if(pTdY->directory[i][0] == numBloque && pTdY->directory[i][1] == U) ){

            pTdY->directory[i][1] = M;

            if(idCPU == 0){
                pTdY->directory[i][2] = 1;                                                       // El CPU 0 lo esta utilizando
            }else{
                if(idCPU == 1){
                    pTdY->directory[i][3] = 1;                                                   // El CPU 1 lo esta utilizando
                }else{
                    pTdY->directory[i][4] = 1;                                                   // El CPU 2 lo esta utilizando
                }
            }
            for(int j = 0; j < 8; ++i){                                                          // Se busca el valor en memoria
                if(pTm->memory[4][j] == numBloque){
                    copiarAcache(pTc, pTmY, bloqueCache, j, pTm, pTmX);
                }
            }
            continuar = false;
            return true;
        }
    }


    // Faltan caso: bloque en cache externa estado compartido, bloque en cache externa estado modificado

}


void principalThread::copiarAcache(sCach *pointerC, sMemory *pointerM, int bloqueCache, int columMemoria, sMemory *pointerMX, sMemory *pointerMY){

    if(pointerC->cache[5][bloqueCache] == M){                                       /* Bloque a reemplazar
                                                                                       en estado: modificado */
        copiarAmemoria(pointerC, bloqueCache, pointerM, pointerMX, pointerMY);
    }
    pointerC->cache[0][bloqueCache] = pointerM->memory[0][columMemoria];            // Se copia el bloque en cache
    pointerC->cache[1][bloqueCache] = pointerM->memory[1][columMemoria];
    pointerC->cache[2][bloqueCache] = pointerM->memory[2][columMemoria];
    pointerC->cache[3][bloqueCache] = pointerM->memory[3][columMemoria];
}

void principalThread::copiarAmemoria(sCach *pointerC, int bloqueCache, sMemory *pointerM, sMemory *pointerMX, sMemory *pointerMY){             /* Se recibe un bloque de cache y se copia en memoria */
    int numeroBloque = pointerC->cache[4][bloqueCache];
    int continuar = true;

    for(int i = 0; i < 8 && continuar; ++i){                                        // Busqueda en la primera memoria
        if(pointerM->memory[4][i] == numeroBloque){
            pointerM->memory[0][i] = pointerC->cache[0][bloqueCache];               // Se copia el bloque de cache a la memoria
            pointerM->memory[1][i] = pointerC->cache[1][bloqueCache];
            pointerM->memory[2][i] = pointerC->cache[2][bloqueCache];
            pointerM->memory[3][i] = pointerC->cache[3][bloqueCache];
            continuar = false;
        }
    }
    for(int i = 0; i < 8 && continuar; ++i){                                        // Busqueda en la segunda memoria
        if(pointerMX->memory[4][i] == numeroBloque){
            pointerMX->memory[0][i] = pointerC->cache[0][bloqueCache];               // Se copia el bloque de cache a la memoria
            pointerMX->memory[1][i] = pointerC->cache[1][bloqueCache];
            pointerMX->memory[2][i] = pointerC->cache[2][bloqueCache];
            pointerMX->memory[3][i] = pointerC->cache[3][bloqueCache];
            continuar = false;
        }
    }
    for(int i = 0; i < 8 && continuar; ++i){                                        // Busqueda en la tercera memoria
        if(pointerMY->memory[4][i] == numeroBloque){
            pointerMY->memory[0][i] = pointerC->cache[0][bloqueCache];               // Se copia el bloque de cache a la memoria
            pointerMY->memory[1][i] = pointerC->cache[1][bloqueCache];
            pointerMY->memory[2][i] = pointerC->cache[2][bloqueCache];
            pointerMY->memory[3][i] = pointerC->cache[3][bloqueCache];
            continuar = false;
        }
    }
}


void principalThread::fin(int idThread, int *registros)
{
    int tmp = contCicTotales - contCicCPU1;
    estadisticas += "------ Datos del hilo "+QString::number(idThread)+" ------\n";
    estadisticas += "\nCPU que lo ejecuta: 1";            // Segunda parte de agregará un indicar para cada CPU
    estadisticas += "\n\n Ciclos de reloj utilizados en el hilo: "+QString::number(contCicCPU1)+"\n\n";
    contCicCPU1 = 0;
    estadisticas += " Estado del reloj al inicio del hilo: "+QString::number(tmp)+"\n\n";
    estadisticas += "*** Los registros quedaron como:\n";
    for(int i=0; i<32; ++i){
        estadisticas += "R["+QString::number(i)+"] = "+QString::number(registros[i])+'\n';
    }
    estadisticas += "*** La cache de datos del procesador quedo asi:\n";
    for(int i=0; i<4; ++i){
        QChar estado;
        switch(cache[5][i]){
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
        estadisticas += "Bloque de cache numero "+QString::number(i)+" estado "+estado+" etiq: "+QString::number(cache[4][i])+'\n';
        estadisticas += "| ";
        for(int j=0; j<4; ++j){
            estadisticas += QString::number(cache[j][i]) + " | ";
        }
        estadisticas += '\n';
    }
}

