
// You may only use fgets() to pull input from stdin
// You may use any print function to stdout to print
// out chat messages
#include <stdio.h>
#include <sgx_tcrypto.h>

// You may use memory allocators and helper functions
// (e.g., rand()).  You may not use system().
#include <stdlib.h>

#include <inttypes.h>
#include <time.h>

#include "Enclave_t.h"

#ifndef UTIL_H_
#define UTIL_H_

#define ADDR_PTR uint64_t
#define CYCLES uint32_t

extern sgx_aes_ctr_128bit_key_t message_key;
extern uint32_t use_encryption;

typedef uint8_t packet_t;
CYCLES measure_one_block_access_time(ADDR_PTR addr);
CYCLES measure_one_rdseed_time();
CYCLES measure_n_rdseed_time(unsigned int n);
void listener_thread(void);
void send_packet(packet_t p);
void send_string(const char* str);
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

/* flag, which indicates that a preamble has been seen and a message is being
 * received. The sender should not send anything until this flag is cleared */
extern volatile bool receiving;

/* flag, which indicates that a message is being sent. The listener should
 * ignore all messages until this flag is cleared. */
extern volatile bool sending;


#endif
