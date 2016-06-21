#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <inttypes.h>

#pragma GCC push_options
#pragma GCC optimize ("O0")

#ifndef MCC_INSTRUMENT_MAX_POINTS
#define MCC_INSTRUMENT_MAX_POINTS 3000
#endif

#ifndef MCC_INSTRUMENT_MAX_RECURSION
#define MCC_INSTRUMENT_MAX_RECURSION 50
#endif

struct time_info {
  void *this_fn;
  void *call_site;
  uint64_t cycles;
};

static struct time_info data[MCC_INSTRUMENT_MAX_POINTS];
static struct time_info stack[MCC_INSTRUMENT_MAX_RECURSION];

// total count of current elements stored in data
static unsigned num_of_elements = 0;
// current recursion level
static unsigned num_of_recursions = 0;
// dump to file
static FILE *file;

static inline uint64_t __attribute__((no_instrument_function)) rdtsc() {
  uint32_t lo, hi;
  __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
  return (uint64_t) hi << 32 | lo;
}

static void __attribute__((no_instrument_function)) __attribute__((constructor)) instrument_init() {
    file = fopen("mprof.out", "w");
    if (file == NULL)
      fprintf(stderr, "failed to create mprof.out!\n");
}

static void __attribute__((no_instrument_function)) __attribute__((destructor)) instrument_fini() {
  if (file == NULL) return;

  for(unsigned i = 0; i < num_of_elements; ++i)
    fprintf(file, "%p %p %" PRId64 "\n", data[i].this_fn, data[i].call_site, data[i].cycles);
  fclose(file);
}

void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void *this_fn, void *call_site) {
  if (this_fn == &fprintf) return;
  if (num_of_elements >= MCC_INSTRUMENT_MAX_POINTS) return;
  if (num_of_recursions >= MCC_INSTRUMENT_MAX_RECURSION) return;

  struct time_info *ti = &stack[num_of_recursions++];
  ti->this_fn = this_fn;
  ti->call_site = call_site;
  ti->cycles = rdtsc();
}

void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void *this_fn, void *call_site) {
  uint64_t cycles = rdtsc();

  if (this_fn == &fprintf) return;
  if (num_of_elements >= MCC_INSTRUMENT_MAX_POINTS) return;
  if (num_of_recursions >= MCC_INSTRUMENT_MAX_RECURSION) return;

  struct time_info *ti = &stack[--num_of_recursions];
  ti->cycles = cycles - ti->cycles;

  // copy the final trace point to the result array
  data[num_of_elements++] = *ti;
}

#pragma GCC pop_options
