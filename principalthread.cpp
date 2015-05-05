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

principalThread::principalThread(QString programa, int numProgramas)
{
    
    QString::iterator it;
    QString strTemp;
    int tamVec = 1;
    
    int indiceVecPCs = 1;
    vecPCs = new int[numProgramas];
    vecPCs[0] = 0;      //Siempre el primer PC es la posicion 0.
    
    for(it = programa.begin(); it!=programa.end(); ++it){ /* Se convierten todas las instrucciones a forma de vector */
        if(*it==' ' || *it=='\n'){
            ++it;
            strTemp.append('|');
            ++tamVec;
        }
        if(*it=='@'){
            strTemp.append('@');
            ++it;
        }
        strTemp.append(*it);
    }
    //qDebug()<<strTemp;
    
    
    qDebug()<<strTemp;
    qDebug()<<"El tamano del vector seria de "<<tamVec;
    
    vecInstrucciones = new int[tamVec];
    int j = 0;
    int unaCifra = true;
    int numCifras = 1;
    bool signo = false;
    QString numTemp;
    bool ok;
    
    for(it = strTemp.begin(); it!=strTemp.end(); ++it){   /* Ciclo que lee cada digito y lo convierte a entero
                                                            almacenandolo en el vector de instrucciones: vecInstrucciones */
        
        if(*it == '@'){
            vecPCs[indiceVecPCs] = j+1;
            ++indiceVecPCs;
        }else{
            if(*it == '|'){
                vecInstrucciones[j] = numTemp.toInt(&ok, 10);
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
    vecInstrucciones[j] = numTemp.toInt(&ok, 10); //agrega el ultimo
    qDebug()<<"El vector quedo como:";
    for(int p=0; p<tamVec; ++p){
        qDebug()<<'-'<<vecInstrucciones[p];
    }
    \
    
    qDebug()<<"El vector de PCs es:";
    for(int i=0; i<numProgramas; ++i){
        qDebug()<<vecPCs[i];
    }
    
}


principalThread::~principalThread()
{
    delete[] vecInstrucciones;
    delete[] vecPCs;
}

bool principalThread::lw(int regX, int regY, int n, int *vecRegs)
{
    int dirPrev = n + regY;
    int numBloque = dirPrev/16;
    int bloqueCache = numBloque%4;  /* Numero del bloque a buscar en el cache*/
    
    int indiceCache = 0;
    //Hay que bloquear el recurso critico (la cache)
    while(indiceCache < 4){
        if(cacheCPU1[4][indiceCache] == bloqueCache){   //Encontre el bloque en mi cache
            //Tengo que revisar el estado del bloque (no se implementa en esta parte).
            vecRegs[regX] = cacheCPU1[(dirPrev%16)/4][bloqueCache]; // Pone la palabra en el registro.
            //Libero el recurso critico (la cache)
            return true;                        //Retorna true ya que tuvo exito con el lw.
        }
        ++indiceCache;
    }
    if(indiceCache >= 4){   //Significa que no esta el bloque a buscar en el cache

        // Busco en el directorio a ver quien es el dueño del bloque (no se implementa en esta parte)
        // Lo tengo que traer de memoria.
        // Intento bloquear el recurso critico (mi memoria). Si no puedo entonces libero la cache tambien.
        for(int i=0; i<4; ++i){
            cacheCPU1[i][bloqueCache] = memoryCPU1[numBloque][i]; //***** Hay que revisar el tamano de la memoria.
        }
        // Libero el semaforo de la memoria
        cacheCPU1[4][bloqueCache] = bloqueCache;    // Le pone el identificador al bloque en cache.
        cacheCPU1[5][bloqueCache] = C;              // Pone el estado del bloque como compartido.
        vecRegs[regX] = cacheCPU1[(dirPrev%16)/4][bloqueCache];     // Pone la palabra en el registro.
        //Libero el recurso critico (la cache)
        return true;
    }
    //Libero el recurso critico (la cache)
    return false;
}

void *principalThread::procesador(void* PC)
{
    int registros[32];   /* Los registros de cada procesador.*/
    registros[0] = 0;   //en el registro 0 siempre hay un 0
    

    long IP = (long)PC;    //IP = instruction pointer

    while(vecInstrucciones[IP] != FIN){ //mientras no encuentre una instruccion de finalizacion

        int IR[4];  //IR = instruction register
        IR[0] = vecInstrucciones[IP];       //Codigo de instruccion
        IR[1] = vecInstrucciones[IP+1];     //Primer parametro
        IR[2] = vecInstrucciones[IP+2];     //Segundo parametro
        IR[3] = vecInstrucciones[IP+3];     //Tercer parametro
        IP += 4;    //Salta a la siguiente instruccion.
        switch(IR[0]){
        case DADDI:
            registros[IR[1]] = registros[IR[2]] + IR[3];                //Rx <- Ry + n
            break;
        case DADD:
            registros[IR[1]] = registros[IR[2]] + registros[IR[3]];     //Rx <- Ry + Rz
            break;
        case DSUB:
            registros[IR[1]] = registros[IR[2]] - registros[IR[3]];     //Rx <- Ry - Rz
            break;
        case LW:
            break;
        case SW:
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
    if(vecInstrucciones[IP] == FIN){
        fin();

        pthread_exit(NULL);
    }
}


bool principalThread::sw(int regX, int regY, int n, int *vecRegs){         /* Funcion que realiza el store */
    
    int dirPrev = n + regY;
    int numBloque = dirPrev / 16;
    int bloqueCache = numBloque % 4;        /* Se obtiene el numero del bloque a buscar en cache */
    
    bool vacio = true;
    
    int contador = 0;
    while(vacio && contador < 4){                           /* Se da lectura en la fila 4 del cache para buscar la etiqueta del bloque*/
        
        if(cacheCPU1[4][contador] == numBloque){          /* El bloque si se encuentra en cache */
            vacio = false;
            
            cacheCPU1[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];   /* Se almacena el contenido del registro en la posicion de la cache */
            cacheCPU1[5][bloqueCache] = M;                            /* Se modifica el estado del bloque */

            for(int i = 0; i < 4; ++i){                              /* Se modifica el estado del bloque en el directorio, estado M: modificado */
                if(directCPU1[i][0] == bloqueCache){                 /* Se busca la etiqueta del bloque en el directorio */
                    directCPU1[i][1] = M;                            /* Se cambia el estado y se le indica al CPU 1 */
                    directCPU1[i][2] = 1;
                }
            }
        }
        ++contador;
    }
    
    if(vacio){                                               /* El bloque no se encuentra en cache y debe cargarse de memoria */

        if(cacheCPU1[5][bloqueCache] == M){                 /* El bloque esta en estado M y debe guardarse en memoria */
            for(int i = 0; i < 4; ++i){
                memoryCPU1[i][cacheCPU1[4][bloqueCache]] = cacheCPU1[i][bloqueCache];     /* Se copia cada estado del bloque en cache a memoria */
            }
            for(int j = 0; j < 4; ++j){
                cacheCPU1[j][bloqueCache] = memoryCPU1[j][numBloque];       /* Se copia el bloque requerido de memoria a cache */
            }
        }else{
            if(cacheCPU1[5][bloqueCache] == C){                             /* El bloque esta compartido */
                for(int j = 0; j < 4; ++j){
                    cacheCPU1[j][bloqueCache] = memoryCPU1[j][numBloque];       /* Se copia el bloque requerido de memoria a cache */
                }
            }
        }
        cacheCPU1[(dirPrev%16)/4][bloqueCache] = vecRegs[regX];                 /* Una vez cargado el bloque, se modifica el registro */
        cacheCPU1[5][bloqueCache] = M;                                          /* Se modifica el estado del bloque */
        cacheCPU1[4][bloqueCache] = numBloque;                                  /* Nuevo bloque en cache */
    }
}

void principalThread::fin()
{

}
