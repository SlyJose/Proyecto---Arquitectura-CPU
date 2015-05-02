#include "principalthread.h"

principalThread::principalThread(QString programa)
{

    QString::iterator it;
    QString strTemp;
    int tamVec = 1;

    for(it = programa.begin(); it!=programa.end(); ++it){ /* Se convierten todas las instrucciones a forma de vector */
        if(*it==' ' || *it=='\n'){
            ++it;
            strTemp.append('|');
            ++tamVec;
        }
        strTemp.append(*it);
    }    
    //qDebug()<<strTemp;
    vecInstrucciones = new int[tamVec];
    QString numTemp;
    int j = 0;
    int contador = 0;

    for(it = strTemp.begin(); it!=strTemp.end(); ++it){   /* Ciclo que lee cada digito y lo convierte a entero
                                                            almacenandolo en el vector de instrucciones: vecInstrucciones */
        if(*it == '|'){
            ++j;
            contador = 0;
        }else{
            if(contador == 0){
                vecInstrucciones[j] = it->digitValue();
                ++contador;
            }else{
                vecInstrucciones[j] = (vecInstrucciones[j] * 10) + it->digitValue(); /* Si el numero es de dos digitos, se sobreescribe el valor */
                contador = 0;
            }
        }
    }

    /*qDebug()<<"El vector quedo como:";
    for(int p=0; p<tamVec; ++p){
        qDebug()<<'-'<<vecInstrucciones[p];
    } */

}

principalThread::~principalThread()
{

}

