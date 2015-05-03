#ifndef METHODS_H
#define METHODS_H


class methods
{
public:
    methods();
    ~methods();

    int daddi(int regY, int n);
    int dadd(int regY, int regZ);
    int dsub(int regY, int regZ);
    bool lw(int regX, int regY, int n);
    bool sw(int regX, int regY, int n);
    void beqz(int regX, int etiq);
    void bnez(int regX, int etiq);
    void fin();

};

#endif // METHODS_H
