#ifndef SIMULATION_H
#define SIMULATION_H


#include <stdint.h>

void init_myrand();

void rand_step(int n, double t[], double x[]);

void comp_mean(int n, double x[], double m[]);



#endif
