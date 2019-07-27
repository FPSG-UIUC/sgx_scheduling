
#include "util.hpp"

/* Measure the time it takes to access a block with virtual address addr. */
CYCLES measure_one_block_access_time(ADDR_PTR addr)
{
    CYCLES cycles;

    asm volatile("mov %1, %%r8\n\t"
    "lfence\n\t"
    "rdtsc\n\t"
    "mov %%eax, %%edi\n\t"
    "mov (%%r8), %%r8\n\t"
    "lfence\n\t"
    "rdtsc\n\t"
    "sub %%edi, %%eax\n\t"
    : "=a"(cycles) /*output*/
    : "r"(addr)
    : "r8", "edi"); 

    return cycles;
}


CYCLES measure_one_rdseed_time()
{
    CYCLES cycles;

    asm volatile(
    "rdtsc\n\t"
    "mov %%eax, %%edi\n\t"
    "try%=: rdseed %%edx\n\t"
    "jnc try%=\n\t"
    "rdtsc\n\t"
    "sub %%edi, %%eax\n\t"
    : "=a"(cycles) 
    : 
    : "edx", "edi");

    return cycles;
}

CYCLES measure_n_rdseed_time(unsigned int n)
{
    CYCLES cycles;

    if (n == 0) return 0;

    asm volatile(
    "movl %1, %%ecx\n\t"
    "rdtsc\n\t"
    "mov %%eax, %%edi\n\t"
    "loop%=: \n\t"
    "try%=: rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    "rdseed %%edx\n\t"
    "jnc try%=\n\t"
    "decl %%ecx\n\t"
    "jne loop%= \n\t"
    "rdtsc\n\t"
    "sub %%edi, %%eax\n\t"
    : "=a"(cycles) 
    : "r"(n)
    : "cc", "ecx", "edx", "edi"); 

    return cycles;
}

uint32_t num_valid_rdseed()
{
    uint32_t num_calls;
    uint32_t max = 1<<20;
    asm volatile(
    "movl %1, %%eax\n\t"
    "incl %%eax\n\t"
    "loop%=: \n\t"
    "decl %%eax\n\t"
    "je done%=\n\t"
    "rdseed %%edx\n\t"
    "jc loop%=\n\t"
    "done%=:\n\t"
    : "=a"(num_calls) 
    : "r"(max) 
    : "cc", "edx"); 

    return max-num_calls;
}

uint32_t test_n_rdseed(uint32_t n)
{
    uint32_t contention;

    if(n == 0) return 0;

    asm volatile(
    "loop%=: \n\t"
    "rdseed %%edx\n\t"
    "jnc contention%=\n\t"
    "decl %%eax\n\t"
    "jne loop%=\n\t"
    "movl $0, %1\n\t"
    "jmp done%=\n\t"
    "contention%=:\n\t"
    "movl $1, %1\n\t"
    "done%=:\n\t"
    : "=r"(contention) 
    : "a"(n) 
    : "cc", "edx"); 

    return contention;
}

void nops(uint32_t n)
{
    asm volatile(
    "loop%=: \n\t"
    "loop loop%=\n\t"
    :  
    : "c"(n) 
    : ); 
}


