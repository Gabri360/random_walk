#ifndef SIMULATION_H
#define SIMULATION_H


#include <stdint.h>
#include "const.h"

void init_myrand();

void rand_step(int n, double t[], double x[][MAX_N]);

void comp_dist(int n, double x[][MAX_N], double d[]);

void comp_mean(int n, double x[], double m[]);



#endif
