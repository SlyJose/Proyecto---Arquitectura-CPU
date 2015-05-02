#include "principalthread.h"

principalThread::principalThread(QString programa)
{

    int fileSize = programa.size();

    qDebug()<<"El tamano original es "<<fileSize;
    QString::iterator it;
    QString strTemp;
    int tamVec = 1;
    //REVISAR!!
    for(it = programa.begin(); it!=programa.end(); ++it){
        if(*it==' ' || *it=='\n'){
            ++it;
            strTemp.append('|');
            ++tamVec;
        }
        strTemp.append(*it);
    }    
    qDebug()<<strTemp;
    qDebug()<<"El tamano del vector seria de "<<tamVec;

    vecInstrucciones = new int[tamVec];
    QString numTemp;
    int j = 0;
    int contador = 0;

    for(it = strTemp.begin(); it!=strTemp.end(); ++it){
        if(*it == '|'){
            ++j;
            contador = 0;
        }else{
            if(contador == 0){
                vecInstrucciones[j] = it->digitValue();
                ++contador;
            }else{
                vecInstrucciones[j] = (vecInstrucciones[j] * 10) + it->digitValue();
                contador = 0;
            }
        }
    }

    qDebug()<<"El vector quedo como:";
    for(int p=0; p<tamVec; ++p){
        qDebug()<<'-'<<vecInstrucciones[p];
    }

}

principalThread::~principalThread()
{

}

