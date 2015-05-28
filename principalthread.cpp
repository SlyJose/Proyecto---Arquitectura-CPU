/**
  * Universidad de Costa Rica
  * Escuela de Ciencias de la Computación e Informática
  * Arquitectura de Computadores
  * Proyecto Programado Parte 1 - Simulacion procesador MIPS
  * @author Fabian Rodriguez
  * @author Jose Pablo Ureña
  * I Semestre 2015
  */


#include "principalthread.h"

/*---- Variables globales (memoria compartida) -----*/
int memory[4][32];  //cache y memoria del cpu0
int cache[6][4];
int memory1[4][32]; //cache y memoria del cpu1
int chache1[6][4];
int memory2[4][32]; //cache y memoria del cpu2
int cache2[6][4];
int directory[8][5];    // Directorios para cada procesador
int directory1[8][5];
int directory2[8][5];

int* vecPrograma;
QString estadisticas;
int contCicCPU1 = 0;                         /* Encargado de llevar el conteo de cada ciclo del reloj en el CPU 1 */
int contCicTotales = 0;                      /* Permite sincronizar que cada CPU vaya por el mismo ciclo de reloj */
/* En la segunda parte se utilizará la variable contCicTotales totalmente */


pthread_mutex_t mutClock = PTHREAD_MUTEX_INITIALIZER;
/*---------------------------------------------------*/

principalThread::principalThread(QString programa, int numHilos)
{

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

    //Inicializa los valores de la memoria y la cache.

    for(int i=0; i<4; ++i){                     /* La memoria se mantiene como un solo bloque */
        for(int j=0; j<32; ++j){
            memory[i][j] = 0;
        }
    }

    for(int i=0; i<6; ++i){                     /* Se inicializa cada cache de CPU */
        for(int j=0; j<4; ++j){
            if(i==4){
                cache[i][j] = -1;
                cache1[i][j] = -1;
                cache2[i][j] = -1;
            }else{
                if(i==5){
                    cache[i][j] = I;
                    cache1[i][j] = I;
                    cache2[i][j] = I;
                }else{
                    cache[i][j] = 0;
                    cache1[i][j] = 0;
                    cache2[i][j] = 0;
                }
            }
        }
    }

    for(int i=0; i<8; ++i){                     /* Se inicializa cada directorio de CPU */
        for(int j=0; j<5; ++j){

            if( j=0 ){                  // Llenado de columna cero y su respectivo numero de bloke
                directory[i][j] = i;
                directory1[i][j] = i+8;
                directory2[i][j] = i+16;
            }else{
                if( j=1 ){              // LLenado de columna uno y su respectiva etiqueta de bloke
                   directory[i][j] = U;
                   directory1[i][j] = U;
                   directory2[i][j] = U;
                }else{
                    directory[i][j] = 0;
                    directory1[i][j] = 0;
                    directory2[i][j] = 0;

                }
            }
        }
    }



}


principalThread::~principalThread()
{
    delete[] vecPCs;
}

bool principalThread::lw(int regX, int regY, int n, int *vecRegs)
{
    int dirPrev = n + vecRegs[regY];
    int numBloque = dirPrev/16;
    int bloqueCache = numBloque%4;  /* Numero del bloque a buscar en el cache*/
    
    int indiceCache = 0;
    //Hay que bloquear el recurso critico (la cache)
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
            contCicCPU1 += 16;              /* En cada escritura se tardan 16 ciclos de reloj */
            contCicTotales += 16;
        }
        for(int i=0; i<4; ++i){ //Write allocate
            cache[i][bloqueCache] = memory[i][numBloque]; // Subo el bloque de memoria a cache.
        }
        contCicCPU1 += 16;              /* 16 ciclos de reloj */
        contCicTotales += 16;
        // Libero el semaforo de la memoria
        cache[4][bloqueCache] = numBloque;    // Le pone el identificador al bloque en cache.
        cache[5][bloqueCache] = C;              // Pone el estado del bloque como compartido.
        vecRegs[regX] = cache[(dirPrev%16)/4][bloqueCache];     // Pone la palabra en el registro.
        //Libero el recurso critico (la cache)
        return true;
    }
    //Libero el recurso critico (la cache)
    return false;
}

void* principalThread::procesador(int id, int pc)
{
    int registros[32];   /* Los registros de cada procesador.*/
    for(int i=0; i<32; ++i){
        registros[i] = 0;
    }
    
    int IP = pc;   //IP = Instruction pointer
    int idHilo = id;

    while(vecPrograma[IP] != FIN){ //mientras no encuentre una instruccion de finalizacion

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
            lw(IR[2], IR[1], IR[3], registros);          //Rx <- M(n + (Ry))            
            break;
        case SW:
            sw(IR[2], IR[1], IR[3], registros);          //M(n + (Ry)) <- Rx
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
    return static_cast<principalThread*>(td->ptr)->procesador(td->idThread, td->numPC);
}

QString principalThread::controlador()
{
    int idThread = 1;
    struct threadData tD;
    pthread_t hilo;
    QString hiloActual = "";

    //-------------------------------------------------------------
    //| Para la segunda parte se debe hacer un vector de threads. |
    //-------------------------------------------------------------

    for(int indicePCs = 0; indicePCs < numThreads; ++indicePCs){
        tD.idThread = idThread;
        tD.numPC = vecPCs[indicePCs];

        hiloActual = QString::number(idThread)+" en ejecucion.";

        pthread_create(&hilo, NULL, procesadorHelper, (void*) &tD);

        pthread_join(hilo, NULL);
        hiloActual = QString::number(idThread)+" terminado.";

        ++idThread;
    }

    estadisticas += "===================================================\n";
    estadisticas += "*** La memoria del procesador quedo como:\n";
    for(int i=0; i<32; ++i){
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


bool principalThread::sw(int regX, int regY, int n, int *vecRegs){         /* Funcion que realiza el store */
    
    int dirPrev = n + vecRegs[regY];
    int numBloque = dirPrev / 16;
    int bloqueCache = numBloque % 4;        /* Se obtiene el numero del bloque a buscar en cache */
    
    bool vacio = true;
    
    int contador = 0;
    while(vacio && contador < 4){                           /* Se da lectura en la fila 4 del cache para buscar la etiqueta del bloque*/
        
        if(cache[4][contador] == numBloque && cache[5][bloqueCache] != I){          /* El bloque si se encuentra en cache */
            vacio = false;
            
            cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];   /* Se almacena el contenido del registro en la posicion de la cache */
            cache[5][bloqueCache] = M;                            /* Se modifica el estado del bloque */
            ++contCicCPU1;
            ++contCicTotales;
            return true;
        }
        ++contador;
    }
    
    if(vacio){                                                   /* El bloque no se encuentra en cache y debe cargarse de memoria */

        if(cache[5][bloqueCache] == M){                           /* El bloque esta en estado M y debe guardarse en memoria */
            for(int i = 0; i < 4; ++i){
                memory[i][cache[4][bloqueCache]] = cache[i][bloqueCache];     /* Se copia cada estado del bloque en cache a memoria */
            }
            contCicCPU1 += 16;                                   /* 16 ciclos de reloj requeridos en la escritura */
            contCicTotales += 16;
        }
        for(int j = 0; j < 4; ++j){
            cache[j][bloqueCache] = memory[j][numBloque];        /* Se copia el bloque requerido de memoria a cache */
        }
        contCicCPU1 += 16;                                       /* 16 ciclos de reloj requeridos en la escritura */
        contCicTotales += 16;
        cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];                 /* Una vez cargado el bloque, se modifica el registro */
        cache[5][bloqueCache] = M;                                          /* Se modifica el estado del bloque */
        cache[4][bloqueCache] = numBloque;                                  /* Nuevo bloque en cache */

        return true;
    }
    return false;
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
