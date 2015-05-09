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

/*---- Variables globales -----*/
int memory[4][32];
int cache[6][4];
int* vecPrograma;
QString estadisticas;
pthread_mutex_t mutClock = PTHREAD_MUTEX_INITIALIZER;
/*-----------------------------*/

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
            vecPCs[indiceVecPCs] = j+1;
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
    for(int i=0; i<4; ++i){
        for(int j=0; j<32; ++j){
            memory[i][j] = 0;
        }
    }

    for(int i=0; i<6; ++i){
        for(int j=0; j<4; ++j){
            if(i==4){
                cache[i][j] = -1;
            }else{
                if(i==5){
                    cache[i][j] = I;
                }else{
                    cache[i][j] = 0;
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
        }
        for(int i=0; i<4; ++i){ //Write allocate
            cache[i][bloqueCache] = memory[i][numBloque]; // Subo el bloque de memoria a cache.
        }
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
            registros[IR[2]] = registros[IR[1]] + IR[3];                //Rx <- Ry + n
            break;
        case DADD:
            registros[IR[3]] = registros[IR[1]] + registros[IR[2]];     //Rx <- Ry + Rz
            break;
        case DSUB:
            registros[IR[3]] = registros[IR[1]] - registros[IR[2]];     //Rx <- Ry - Rz
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
            break;
        case BNEZ:
            if(registros[IR[1]] != 0){                                  //Rx != 0, salta
                IP += (IR[3])*4;
            }
            break;
        }
    }
    if(vecPrograma[IP] == FIN){
        fin(idHilo, registros);
    }
    //pthread_mutex_unlock(&mutClock);

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

    /*---------------Para desplegar los hilos que estan corriendo.--------------*/
    QPlainTextEdit *textEdit = new QPlainTextEdit;
    QPushButton *quitButton = new QPushButton("&Aceptar");
    QObject::connect(quitButton, SIGNAL(clicked()), qApp, SLOT(quit()));
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(textEdit);
    layout->addWidget(quitButton);
    QWidget window;
    window.setLayout(layout);
    window.show();
    /*---------------------------------------------------------------------------*/

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

        hiloActual += "Empezo la ejecucion del hilo: "+QString::number(tD.idThread)+'\n';


        textEdit->setPlainText(hiloActual);

        pthread_create(&hilo, NULL, procesadorHelper, (void*) &tD);

        pthread_join(hilo, NULL);
        hiloActual += "Hilo actual: " + QString::number(tD.idThread);
        hiloActual += "  Estado: Finalizado\n";

        ++indicePCs;
        ++idThread;

        textEdit->setPlainText(hiloActual);
    }

    estadisticas += "La memoria del procesador quedo como:\n";
    for(int i=0; i<32; ++i){
        estadisticas += "Bloque de memoria "+QString::number(i)+'\n';
        for(int j=0; j<4; ++j){
            estadisticas += '['+QString::number(memory[j][i])+"]-";
        }
        estadisticas += '\n';
    }

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
            return true;
        }
        ++contador;
    }
    
    if(vacio){                                               /* El bloque no se encuentra en cache y debe cargarse de memoria */

        if(cache[5][bloqueCache] == M){                 /* El bloque esta en estado M y debe guardarse en memoria */
            for(int i = 0; i < 4; ++i){
                memory[i][cache[4][bloqueCache]] = cache[i][bloqueCache];     /* Se copia cada estado del bloque en cache a memoria */
            }
        }
        for(int j = 0; j < 4; ++j){
            cache[j][bloqueCache] = memory[j][numBloque];       /* Se copia el bloque requerido de memoria a cache */
        }
        cache[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];                 /* Una vez cargado el bloque, se modifica el registro */
        cache[5][bloqueCache] = M;                                          /* Se modifica el estado del bloque */
        cache[4][bloqueCache] = numBloque;                                  /* Nuevo bloque en cache */

        return true;
    }
    return false;
}

void principalThread::fin(int idThread, int *registros)
{
    //--------------------------------------------------------------------------------
    //| Lo primero es pasar los bloques que quedaron en cache como 'M' a la memoria. |
    //--------------------------------------------------------------------------------

    //Bloqueo mi cache.
    for(int i=0; i<4; ++i){
        if(cache[5][i] == M){   //el bloque esta modificado
            //Intento bloquear la memoria, si no puedo entonces suelta tambien la cache.
            for(int j=0; j<4; ++j){
                memory[j][cache[4][i]] = cache[j][i];
            }
            //Libero la memoria.
        }
    }
    //Libero la cache.

    estadisticas += "Datos del hilo "+QString::number(idThread)+'\n';
    estadisticas += "Los registros quedaron como:\n";
    for(int i=0; i<32; ++i){
        estadisticas += "R["+QString::number(i)+"] = "+QString::number(registros[i])+'\n';
    }
    estadisticas += "La cache de datos del procesador quedo asi:\n";
    for(int i=0; i<6; ++i){
        QChar estado;
        switch(cache[5][i]){
        case I:
            estado = 'I';
            break;
        case M:
            estado = 'M';
            break;
        case C:
            estado = 'C';
        }

        estadisticas += "Bloque "+QString::number(i)+" estado "+estado+":\n";
        for(int j=0; j<4; ++j){
            estadisticas += QString::number(cache[i][j]) + '-';
        }
        estadisticas += '\n';
    }
}
