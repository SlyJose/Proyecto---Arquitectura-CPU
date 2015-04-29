#include "principalthread.h"

principalThread::principalThread(QString programa)
{

    int fileSize = programa.size();

    qDebug()<<"El tamano original es "<<fileSize;
    QString::iterator it;
    QString strTemp;
    int tamVec = 0;
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
    bool ok;
    for(it = strTemp.begin(); it!=strTemp.end(); ++it){
       if(*it=='|'){
           ++it;
           vecInstrucciones[j] = numTemp.toInt(&ok, 10);
           ++j;
           numTemp.clear();
       }
           numTemp.append(*it);

    }

    qDebug()<<"El vector quedo como:";
    for(int p=0; p<tamVec; ++p){
        qDebug()<<'-'<<vecInstrucciones[p];
    }

    //**HAY UN PROBLEMA, OMITE EL ULTIMO ELEMENTO DEL STRING Y NO SE PORQUE**

}

principalThread::~principalThread()
{

}

