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
    
    // ----------------------------------------------
    // |            Busco en cache local            |
    // ----------------------------------------------

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
            vecRegs[regX] = pTc->cache[filaCache][bloqueCache];
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
            int bloqueReemplazar = pTc->cache[4][bloqueCache];

            // -------------------------------------------------------------------------------------
            // |        Quito el bloque modificado , que le voy a caer encima, de la cache      |
            // -------------------------------------------------------------------------------------

            // Saco el bloque de mi cache y lo guardo en la memoria que corresponda
            // pero tengo que ir a ponerlo como "uncached" en el directorio que corresponda.
            if(pTc->cache[5][bloqueCache] == M){    //donde va a poner el bloque esta modificado?

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
                        int indiceDirect = 0;
                        bool encontrado = false;
                        while(indiceDirect<8 && encontrado == false){
                            if(pTd->directory[indiceDirect][0] == bloqueReemplazar){    //lo ubique en mi directorio
                                pTd->directory[indiceDirect][1] = U;    //lo pongo uncached en mi directorio
                                encontrado = true;
                            }
                            ++indiceDirect;
                        }
                        // ***** AQUI DEBERIA DE BLOQUEAR LA MEMORIA PERO LA PROFE DIJO QUE CON SOLO BLOQUEAR EL DIRECTORIO YA PROTEJO LA MEMORIA *****
                        for(int i=0; i<4; ++i){
                            pTm->memory[i][bloqueReemplazar] = pTc->cache[i][bloqueCache];  //copia el bloque de cache local a memoria local
                        }
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
                        int indiceDirect = 0;
                        bool encontrado = false;
                        while(indiceDirect < 8 && encontrado == false){
                            if(pTdX->directory[indiceDirect][0] == bloqueReemplazar){   //lo ubique en el directorio remoto 1
                                pTdX->directory[indiceDirect][1] = U;   // lo pongo "uncached"
                                encontrado == true;
                            }
                            ++indiceDirect;
                        }
                        for(int i=0; i<4; ++i){
                            pTmX->memory[i][bloqueReemplazar] = pTc->cache[i][bloqueCache];  //copia el bloque de cache local a memoria remota 1
                        }
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
                        int indiceDirect = 0;
                        bool encontrado = false;
                        while(indiceDirect < 8 && encontrado == false){
                            if(pTdY->directory[indiceDirect][0] == bloqueReemplazar){   //lo ubique en el directorio remoto 1
                                pTdY->directory[indiceDirect][1] = U;   // lo pongo "uncached"
                                encontrado == true;
                            }
                            ++indiceDirect;
                        }
                        for(int i=0; i<4; ++i){
                            pTmY->memory[i][bloqueReemplazar] = pTc->cache[i][bloqueCache];  //copia el bloque de cache local a memoria remota 1
                        }
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

            // Tengo que ver si alguien lo tiene modificado en su cache primero
            // y ademas se que no lo tengo en mi cache local, pero puede que el
            // bloque si me pertenezca -> lo tengo en mi directorio
            if(pTd->directory[0][0] <= numBloque && numBLoque <= pTd->directory[7][0]){ //el bloque que tengo que subir me pertenece a mi
                int resultBloqueDirect;
                switch(idCPU){  //bloqueo directorio local
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
                if(resultBloqueDirect == 0){

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
            if(){

            }
            if(){

            }

        }
    }else{
        return false;
    }
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
            for(int i = 0; i < 8 && continuar; ++i){                                                    /* Busqueda en directorio local */
                if(pTd->directory[i][0] == numBloque){
                    if(idCPU == 0){                                                                     /* Se coloca el estado del CPU que lo usa en 1 */
                            pTd->directory[i][2] = 1;
                            pTd->directory[i][3] = 0;
                            pTd->directory[i][4] = 0;
                    }else{
                        if(idCPU == 1){
                            pTd->directory[i][3] = 1;
                            pTd->directory[i][2] = 0;
                            pTd->directory[i][4] = 0;
                        }else{
                            pTd->directory[i][4] = 1;
                            pTd->directory[i][2] = 0;
                            pTd->directory[i][3] = 0;
                        }
                    }

                    bool recorrer = true;
                    for(int j = 0; j < 4 && recorrer; ++j){                                  // Se modifica el estado en las caches
                        if(pTcX->cache[4][j] == numBloque && pTcX->cache[5][j] = C){
                            pTcX->cache[5][j] = I;
                            recorrer = false;
                        }
                    }
                    for(int j = 0; j < 4 && recorrer; ++j){
                        if(pTcY->cache[4][j] == numBloque && pTcY->cache[5][j] = C){
                            pTcY->cache[5][j] = I;
                            recorrer = false;
                        }
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

