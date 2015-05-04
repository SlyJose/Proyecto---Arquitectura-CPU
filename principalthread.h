/**
  * Universidad de Costa Rica
  * Escuela de Ciencias de la Computación e Informática
  * Arquitectura de Computadores
  * Proyecto Programado Parte 1 - Simulacion procesador MIPS
  * @author Fabian Rodriguez
  * @author Jose Pablo Ureña
  * I Semestre 2015
  */

#ifndef PRINCIPALTHREAD_H
#define PRINCIPALTHREAD_H

#include <QString>
#include <QDebug>
#include <iterator>
#include <pthread.h>

//Los tipos de instrucciones que puede simular el procesador
#define DADDI   8
#define DADD    32
#define DSUB    34
#define LW      35
#define SW      43
#define BEQZ    4
#define BNEZ    5
#define FIN     63

//Los tipos de estado del directorio

#define U   0
#define C   1
#define M   2

class principalThread
{
public:
    principalThread(QString programa, int numProgramas);
    ~principalThread();

    /**
     * Se encarga de hacer una suma de un inmediato \var n con el registro \var regY y lo almacena
     * en el registro identificado por \var regX.
     * @brief daddi instruccion propia del procesador MIPS.
     * @param regX registro donde se va a guardar el resultado.
     * @param regY registro con el valor a sumar.
     * @param n inmediato a sumar con \var regY.
     * @param vecRegs vector con los registros del procesador.
     */
    void daddi(int regX, int regY, int n, int* vecRegs);

    /**
     * Se encarga de hacer la suma del valor almacenado en \var regz con el valor almacenado
     * en \var regY y guardar la suma en el registro indicado por \var regX.
     * @brief dadd instruccion propia del procesador MIPS.
     * @param regX registro donde se va a guardar el resultado.
     * @param regY registro donde va a estar el valor a usar para la suma.
     * @param regZ registro donde va a estar el otro valor a usar por la suma.
     * @param vecRegs vector con los registros del procesador.
     */
    void dadd(int regX, int regY, int regZ, int* vecRegs);

    /**
     * Se encarga de hacer la resta de lo que esta guardado en \var regY menos el valor guardado
     * en \var regZ y guarda el resultado en el registro identificado por \var regX.
     * @brief dsub instruccion propia del procesador MIPS
     * @param regX registro donde se va a guardar el resultado de la operacion.
     * @param regY registro al que se le va a hacer la resta.
     * @param regZ registro con el valor que se va a restar.
     * @param vecRegs vecto con los registros del procesador.
     */
    void dsub(int regX, int regY, int regZ, int* vecRegs);

    bool lw(int regX, int regY, int n, int* vecRegs);
    bool sw(int regX, int regY, int n);

    void beqz(int regX, int etiq);
    void bnez(int regX, int etiq);

    void fin();

    void* procesador();

private:
    int* vecInstrucciones;  /*!< Es el vector que va a tener las instrucciones de todos los programas.*/
    int* vecPCs;            /*!< Vector con los indices donde inicia cada programa en el vector.*/


    int memoryCPU1[5][4];   /* Estructuras de datos para cada procesador */
    int cacheCPU1[6][4];
    int directCPU1[4][4];

};

#endif // PRINCIPALTHREAD_H
