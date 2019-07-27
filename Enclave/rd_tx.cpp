

#include"util.hpp"
#define TICK_HI 50000
#define TICK_LO 10000

#define DELAY 200000
#define LEN0 800
#define LEN1 4000
#include <cstring>
#include <sgx_tcrypto.h>


void send_preamble();
void send_byte(uint8_t byte);
void tick();
void test_timing();

/* sending flag indicates  that a message is being sent and all received
 * messages should be ignored
 *
 * It is only ever set locally */
/* volatile bool sending; */

/* receiving flag indicates  that a message is being received and all send
 * requests should be delayed until sending has completed.
 *
 * It only ever set externally */
/* volatile bool receiving; */

void send_string(const char* str)
{
    
    if(use_encryption == 1)
    {
		uint8_t counter[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		uint32_t len = strlen(str);
		uint32_t blocks = (len+15)/16;
		uint8_t * buf = (uint8_t*)malloc(16*blocks);

		strncpy((char*)buf, str, blocks*16);

		sgx_aes_ctr_encrypt(&message_key, buf, blocks*16, counter, 8, buf);

		/* spin until any messages being received are completely received. */
		while(receiving) {};
		sending = true;
		for(int i=0; i < blocks*16; i++)
			send_packet(buf[i]);
		//for(int i=0; str[i] != NULL; i++)
		//{
			//send_packet(str[i]);
		//}
		send_packet((uint8_t)'d');
//		send_packet('e');
		sending = false;
		return;
	}
	else
	{
		/* spin until any messages being received are completely received. */
		while(receiving) {};
		sending = true;
		for(int i=0; str[i] != NULL; i++)
		{
		    send_packet(str[i]);
		}
		sending = false;
		return;
	}
		
}

void send_packet(packet_t p)
{
    send_preamble();
    /* send_byte(0xCC); */
    send_byte(p);
    send_byte(0xCC);
    /* ack bytes */
    /* send_byte(0x41); */
    /* send_byte(0x41); */
}


void tick()
{
    nops(DELAY);
}

void send_preamble()
{
    int preamble_len = 10;

    for(int i=0; i < preamble_len; i++)
    {
        if(i%2)
            do_n_rdseed<LEN0>();
        //measure_n_rdseed_time(LEN0);
        else
            do_n_rdseed<LEN1>();
        //measure_n_rdseed_time(LEN1);
        tick();
    }
}

void send_byte(uint8_t byte)
{
    uint8_t pairity = 0;

    for(int i=0; i < 8; i++)
    {
        pairity ^= (byte&0x80)>>7;
        if(byte & 0x80)
            //measure_n_rdseed_time(LEN0);
            do_n_rdseed<LEN0>();
        else
            //measure_n_rdseed_time(LEN1);
            do_n_rdseed<LEN1>();
        byte = byte<<1;
        tick();
    }

    if(pairity == 0x01)
    {
        do_n_rdseed<LEN0>();
    }
    else
    {
        do_n_rdseed<LEN1>();
    }
    tick();
    tick();
    tick();
}


/*
   void test_timing()
   {
   int iterations = 0;
   while(1)
   {
   printf("Set iteration value: ");
   scanf("%d",&iterations);
   printf("\n");

   measure_n_rdseed_time(iterations);
   printf("Did %d rdseeds.\n", iterations);
   }

   }
   */




