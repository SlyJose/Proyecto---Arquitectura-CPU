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

bool principalThread::lw(int regX, int regY, int n)
{

}

void *principalThread::procesador()
{

    int registros[32];   /* Los registros de cada procesador.*/
    registros[0] = 0;   //en el registro 0 siempre hay un 0
}


bool sw(int regX, int regY, int n){         /* Funcion que realiza el store */

    int dirPrev = n + regY;
    int numBloque = dirPrev / 16;
    int bloqueCache = numBloque % 4;        /* Se obtiene el numero del bloque a buscar en cache */

    bool vacio = true;

    int contador = 0;
    while(vacio && contador < 4){                           /* Se da lectura en la fila 4 del cache para buscar la etiqueta del bloque*/

        if(cacheCPU1[4][contador] == bloqueCache){          /* El bloque si se encuentra en cache */
            vacio = false;

            //palabraEnBloque = bloqueCache / 4;
            //modifica la palabra con el contenido de RX

            for(int i = 0; i < 4; ++i){                              /* Se modifica el estado del bloque en el directorio, estado M: modificado */
                if(directCPU1[i][0] == bloqueCache){                 /* Se busca la etiqueta del bloque en el directorio */
                        directCPU1[i][1] = M;                        /* Se cambia el estado y se le indica al CPU 1 */
                        directCPU1[i][2] = 1;
                }
            }
        }
        ++contador;
    }

    if(vacio){                                               /* El bloque no se encuentra en cache */

        int estadoActual;

        for(int i = 0; i < 4; ++i){                              /* Verifica el estado del bloque en el directorio */
            if(directCPU1[i][0] == bloqueCache){                 /* Encuentra la etiqueta del bloque en el directorio */

                if(directCPU1[i][1] == M){

                    // copia el bloque en ram
                }

                // copia de ram el numbloque
                // le cae encima al bloque en cache
                // palabraEnBloque = bloqueCache / 4
                // modifica palabraEnBloque en cache con Rx
                // marca en el directorio M para ese bloque
            }
        }
    }


}











