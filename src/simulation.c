#include "simulation.h"

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

void rand_step(int n, double t[], double x[])
{
    t[n] = n * 0.1;
    if (n == 0)
        x[0] = 0.0;
    else
        x[n] = x[n - 1] + ((next_u64() >> 63) ? 1.0 : -1.0);
}

void comp_mean(int n, double x[], double m[])
{
    if (n == 0)
        m[0] = 0.0;
    else
        m[n] = (m[n - 1] * (n - 1) + x[n - 1]) / n;
}
