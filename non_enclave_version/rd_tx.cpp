#include"util.hpp"
#define TICK_HI 50000
#define TICK_LO 10000

#define DELAY 200000
#define LEN0 800
#define LEN1 4000



void send_preamble();
void send_byte(uint8_t byte);
void send_packet(packet_t p);
void tick();
void test_timing();

int main(int argc, char **argv)
{
	// Put your covert channel setup code here

	printf("Please type a message.\n");

	bool sending = true;
	while (sending) {
		char text_buf[128];
		fgets(text_buf, sizeof(text_buf), stdin);

		char count;
		for(count = 'a'; text_buf[count] != '\0'; count++) { ; }
		count++;
		printf("%d\n",count);
		send_preamble();
		send_byte((char)count);
		send_byte(0xCC);

		for(int c=0; c < 128; c++)
		{
			if(text_buf[c] == '\0')
				break;
			//test_timing();
			send_preamble();
			send_byte(text_buf[c]);
			send_byte(0xCC);
			//for(int i=0; i < DELAY; i++)
			//{
			//  measure_n_rdseed_time(8);
			//}
		}
	}

	printf("Sender finished.\n");

	return 0;
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




