#ifndef METHODS_H
#define METHODS_H


class methods
{
public:
    methods();
    ~methods();

    int daddi(int regX, int regY, int n);
    int dadd(int regX, int regY, int regZ);
    int dsub(int regX, int regY, int regZ);

    /**
     * @brief lw
     * @param regX
     * @param regY
     * @param n
     * @return true si hubo exito, false si fallo
     */
    bool lw(int regX, int regY, int n);

    /**
     * @brief sw
     * @param regX
     * @param regY
     * @param n
     * @return true si hubo exito, false si fallo
     */
    bool sw(int regX, int regY, int n);

    void beqz(int regX, int etiq);
    void bnez(int regX, int etiq);
    void fin();

};

#endif // METHODS_H
