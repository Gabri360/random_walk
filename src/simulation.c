#include "simulation.h"
#include <math.h>


static uint64_t rng_state = 88172645463325252ULL;

void init_myrand(uint64_t seed)
{
    rng_state = seed ? seed : 88172645463325252ULL;
}

static uint64_t next_u64(void)
{
    uint64_t s = rng_state;
    s ^= s >> 12;
    s ^= s << 25;
    s ^= s >> 27;
    rng_state = s;
    return s * 2685821657736338717ULL;
}

void rand_step(int n, double t[], double x[][MAX_N])
{

	t[n] = n * 0.1;

    for (int i = 0; i < DIM; i++)
        x[i][n] = (n == 0) ? 0.0 : x[i][n - 1];

    if (n > 0) {
        uint64_t r = next_u64() % (2 * DIM);
        int axis = (int)(r / 2);
        x[axis][n] += (r % 2) ? 1.0 : -1.0;
    }
}

void comp_dist(int n, double x[][MAX_N], double d[])
{
	d[n] = 0;
	for(int i=0;i<DIM;i++) {
		d[n] += x[i][n]*x[i][n];
	}
	d[n] = sqrt(d[n]);
}


void comp_mean(int n, double x[], double m[])
{
    if (n == 0)
        m[0] = 0.0;
    else
        m[n] = (m[n - 1] * (n - 1) + x[n - 1]) / n;
}
