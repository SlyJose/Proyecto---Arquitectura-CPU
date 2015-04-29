#ifndef PRINCIPALTHREAD_H
#define PRINCIPALTHREAD_H

#include <QString>
#include <QDebug>
#include <iterator>

//Los tipos de instrucciones que puede simular el procesador
#define DADDI   8
#define DADD    32
#define DSUB    34
#define LW      35
#define SW      43
#define BEQZ    4
#define BNEZ    5
#define FIN     63

class principalThread
{
public:
    principalThread(QString programa);
    ~principalThread();

private:
    int* vecInstrucciones;
};

#endif // PRINCIPALTHREAD_H
