#include <stdlib.h>
#include <time.h>
#include "simulation.h"

void init_myrand()
{
	srand((unsigned int)time(NULL));
}

void rand_step(int n, double x[], double y[])
{
	double r = -1.0 + 2.0 * ((double)rand() / (RAND_MAX + 1.0));

	x[n] = n * 0.1;
	if(n!=0) {
		y[n] = y[n-1] + r;
	}
}
