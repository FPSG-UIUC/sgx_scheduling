
#include"util.hpp"

#define MAX_CALLS 40
#define SAMPLES 100
#define DELAY 1000

int main(int argc, char **argv)
{
    // Put your covert channel setup code here

    CYCLES test[100];
    printf("Please press enter.\n");

    char text_buf[2];
    int dummy;
    fgets(text_buf, sizeof(text_buf), stdin);

    CYCLES averages[MAX_CALLS];
    for (int n = 1; n < MAX_CALLS; n++) {
      averages[n] = 0;
      for (int s = 0; s < SAMPLES; s++) {
        averages[n] += measure_n_rdseed_time(n);
        for (int d = 0; d < DELAY; d++)
          dummy += d;
      }
      averages[n] /= SAMPLES;
    }

    printf("Average latency of n calls to rdseed (cycles)\n");
    printf("\tn\tlatency\tper_call\n");
    for (int n = 1; n < MAX_CALLS; n++) {
      printf("\t%3d: %10d\t%10d\n", n, averages[n], averages[n]/n);
    }

#define TRIES 40
    uint32_t tries[TRIES];
    for (int n = 0; n < TRIES; n++) {
      tries[n] = num_valid_rdseed();
        for (int d = 0; d < DELAY; d++)
          dummy += d;
    }

    printf("Number of calls required to run out of random bits:\n") ;
    printf("\tn\tcalls\n");
    for (int n = 0; n < TRIES; n++) {
      printf("\t%3d: %10d\n", n, tries[n]);
    }
    
    printf("Receiver finished.\n");

    return 0;
}


