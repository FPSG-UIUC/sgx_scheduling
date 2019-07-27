
#include"util.hpp" 
#define DELAY 1000
#define SAMP_BUF_SZ 30
#define FILTER 0.2
#define ITER_MAX 1<<30


#define PREAMBLE_LEN 10
#define SUFFIX_LEN 1
#define DATA_LEN 8
#define PACKET_LEN (PREAMBLE_LEN+SUFFIX_LEN+DATA_LEN)
#define PREAMBLE 0x2AA

void print_array(int32_t array[], int size);
void shift_in(int32_t val, int32_t array[], int size);
bool search_packet(int32_t array[], int size, packet_t * packet);
double lpf(double v_old, double v_new, double weight);
void* listener_thread(void* in);

int main(int argc, char **argv)
{
  
  listener_thread(NULL);
  printf("Receiver finished.\n");
  return 0;
}

void shift_in(int32_t val, int32_t array[], int size)
{
  for(int i=size-1; i > 0; i--)
  {
    array[i] = array[i-1];
  }
  array[0] = val;
}

void print_array(int32_t array[], int size)
{
  for(int i=0; i < size; i++)
  {
    printf("%.0d ", array[i]);
  }
}


bool search_packet(int32_t array[], int size, packet_t * packet)
{
  // check preamble
  uint32_t preamble = PREAMBLE;
  packet_t data = 0;
  uint8_t pairity = 0, pairity_expected;
  int32_t i = 0;

  if(size < PACKET_LEN) 
    return false;

  //check preamble
  for(i=PACKET_LEN-1; i >= PACKET_LEN-PREAMBLE_LEN; i--, preamble>>=1)
  {
    if(array[i] != (preamble & 0x1))
      return false;
  }

  //debug aray
  //for(int b=PACKET_LEN-1; b >= 0; b--)
  //  printf("%1d",array[b]);
  //printf("\n");

  // grab data
  for(;i >= SUFFIX_LEN; i--)
  {
    data <<= 1;
    data |= (0x01 & array[i]);
    pairity ^= (array[i] == 0x1);
  }

  // grab pairity
  pairity_expected = array[i];
  // zero out packet so we don't see it again
  for(i=PACKET_LEN-1; i>=0; i--)
  {
    array[i] = 0;
  }

  *packet = data;

  if(pairity_expected != pairity)
    return false;
  else
    return true;
}






void* listener_thread(void* in)
{


  // edge detection thresholds, with hysteresis
  double threshold_hi = 0.1;  // low to high transition
  double threshold_lo = 0.01; // high to low transistion

  // bool, measured contention on the RDRAND HW
  int contention = 0;
  // filtered contention value
  double contention_f = 0;
  double contention_filter = FILTER;

  // the current state in hysteresis, representing the last sampled edge
  int32_t state = 0; //0-> last sampled falling edge

  // pulse length threshold for determining a 1 or a 0
  // tuned at runtime by finding pseudo average pulse width
  double pw_thresh = 80;
  double pw_thresh_filter = 0.0001; // pseudo average filter

  // array of the last N pulse legths, in # of iterations
  int32_t pulse_widths[SAMP_BUF_SZ]; // Number of iterations between falling edges

  int32_t bits[SAMP_BUF_SZ];   // buffer of received bits
  int32_t bit = 0;             // bit sample;
  int32_t iteration = 0;       // Counter for loop iterations 

  // container for received packet, if one is found.
  packet_t packet;

  // Initialize the buffer of pulse widths
  for (int i=0; i < SAMP_BUF_SZ; i++)
    pulse_widths[i] = 0;

  bool listening = true;
  while (listening) 
  {
    // sample contention and filter
    contention = test_n_rdseed(3);
    contention_f = lpf(contention_f, contention, contention_filter);

    switch(state)
    {
      // in low state (last bit was 0, last transition was high to low
      case 0:
        // check for low to high transition
        // if so, reset iteration count to measure pulse width
        if(contention_f > threshold_hi)
        {
          iteration = 0;
          state = 1;
        }
        break;

      // in high state (last bit was 1, last transition was low to high
      case 1:
        // check for high to low transition, marking the end of a pulse.
        // if so: measure pulse and associated bit, and reset to low state
        if(contention_f < threshold_lo)
        {
          
          // add pulse width to pulse_widths buffer
          shift_in(-iteration, pulse_widths, SAMP_BUF_SZ);

          // determine bit for associated pulse by comparing the measured
          // width against the pw_thresh. Add it to bit buffer.
          bit = (iteration<pw_thresh) ? 1:0;
          shift_in(bit, bits, SAMP_BUF_SZ);

          // search bit buffer for packets, and print them if found.
          if(search_packet(bits, PACKET_LEN, &packet))
          {
            printf("%c",packet);
          }

          // fold this pulse width into the pulse width threshold, if it 
          // wasn't too long.
          if(iteration < 1000)
            pw_thresh = lpf(pw_thresh, iteration, pw_thresh_filter);
  
          // reset iteration counter and go to low state
          iteration = 0;
          state = 0;
        }
        break;
    }
    
    // wait until next iteration
    nops(DELAY);
    iteration++;
  }
            
  return 0;
}

double lpf(double v_old, double v_new, double weight)
{
//  assert(w >= 0.0 && w <= 1.0); 
  return v_old*(1.0-weight) + v_new*(weight);
}
