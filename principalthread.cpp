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
    QString pcsTmp;
    int numCifras = 1;
    bool signo = false;
    QString numTemp;
    bool ok;

    for(it = strTemp.begin(); it!=strTemp.end(); ++it){   /* Ciclo que lee cada digito y lo convierte a entero
                                                            almacenandolo en el vector de instrucciones: vecInstrucciones */

        if(*it == '@'){
            int tmp = j+1;
            pcsTmp.append(tmp);
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

    qDebug()<<"El vector de pcs tiene los indices: "<<pcsTmp;
    int tamVecPc = pcsTmp.length();
    vecPCs = new int[tamVecPc];
    j = 0;

    for(it = pcsTmp.begin(); it != pcsTmp.end(); ++it){
        vecPCs[j] = it->digitValue();
        ++j;
    }

    qDebug()<<"El vector de PCs es:";
    for(int i=0; i<tamVecPc; ++i){
        qDebug()<<vecPCs[i];
    }

}


principalThread::~principalThread()
{

}

