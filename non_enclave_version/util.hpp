
// You may only use fgets() to pull input from stdin
// You may use any print function to stdout to print 
// out chat messages
#include <stdio.h>

// You may use memory allocators and helper functions 
// (e.g., rand()).  You may not use system().
#include <stdlib.h>

#include <inttypes.h>
#include <time.h>

#ifndef UTIL_H_
#define UTIL_H_

#define ADDR_PTR uint64_t 
#define CYCLES uint32_t

typedef uint8_t packet_t;
CYCLES measure_one_block_access_time(ADDR_PTR addr);
CYCLES measure_one_rdseed_time();
CYCLES measure_n_rdseed_time(unsigned int n);
uint32_t num_valid_rdseed();
uint32_t test_n_rdseed(uint32_t n);
void nops(uint32_t n);

// RDSEED Recursive Template
// recoursive step
template
    <
      size_t   count
    >
void do_n_rdseed() {
    asm volatile(
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    : 
    : 
    : "edx"); 
    do_n_rdseed<(count - 4)/2>();
    do_n_rdseed<(count-4) - ((count - 4)/2)>();
}

// base of recursion
template<>
inline void do_n_rdseed<0>() {
}
template<>
inline void do_n_rdseed<1>() {

    asm volatile(
    "rdseed %%edx\n\t"
    : 
    : 
    : "edx"); 

}
template<>
inline void do_n_rdseed<2>() {

    asm volatile(
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    : 
    : 
    : "edx"); 
}
template<>
inline void do_n_rdseed<3>() {

    asm volatile(
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    : 
    : 
    : "edx"); 
}

#endif
